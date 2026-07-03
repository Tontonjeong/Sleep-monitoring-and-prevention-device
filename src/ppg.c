// ppg.c — MCP3204 CH0, Fs=200 Hz, HPF+LPF, adaptive peak detection, rail/outlier rejection
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringSerial.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include "config.h"

#define FS_HZ              200.0
#define SAMPLE_NS          5000000L
#define UART_DEV           "/dev/serial0"
#define UART_BAUD          115200
#define FILTER_MODE        0  /* 0: 1st order HPF+LPF, 1: biquad HPF0.5 + LPF8 */

typedef enum { ST_IDLE = 0, ST_RUN = 1, ST_QUIT = 2 } SystemState;
static volatile SystemState g_state = ST_IDLE;
static volatile int g_running = 0;
static volatile unsigned int g_debounce_ms = 200;
static volatile unsigned int g_last_start_ms = 0;
static volatile unsigned int g_last_stop_ms = 0;
static int g_uart_fd = -1;

static void filter_reset_all(void);
static void peak_reset_all(void);

static inline unsigned int now_ms_u(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned int)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static inline unsigned long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000000ul + ts.tv_nsec / 1000ul);
}

static void on_sigint(int sig) {
    (void)sig;
    g_state = ST_QUIT;
}

static void start_isr(void) {
    unsigned int t = now_ms_u();
    if (t - g_last_start_ms < g_debounce_ms) return;
    g_last_start_ms = t;
    if (g_state != ST_QUIT && g_state != ST_RUN) {
        g_state = ST_RUN;
        g_running = 1;
        filter_reset_all();
        peak_reset_all();
        fprintf(stderr, "[START] RUNNING\n");
    }
}

static void stop_isr(void) {
    unsigned int t = now_ms_u();
    if (t - g_last_stop_ms < g_debounce_ms) return;
    g_last_stop_ms = t;
    if (g_state == ST_RUN) {
        g_running = 0;
        g_state = ST_IDLE;
        fprintf(stderr, "[STOP] IDLE\n");
    }
}

static int mcp3204_read_ch(uint8_t ch, uint16_t *out12) {
    if (!out12 || ch > 3) return -1;
    unsigned char tx[3] = {0};
    /* MCP3204 single-ended command: start=1, sgl=1, channel bits. */
    tx[0] = 0x06 | ((ch & 0x04) >> 2);
    tx[1] = (unsigned char)((ch & 0x03) << 6);
    tx[2] = 0x00;
    if (wiringPiSPIDataRW(SPI_CH, tx, 3) < 0) return -1;
    *out12 = (uint16_t)(((tx[1] & 0x0F) << 8) | tx[2]);
    return 0;
}

static inline float adc12_to_volt(uint16_t v) {
    return (float)v * (MCP3204_VREF / 4095.0f);
}

/* ---------- Filtering ---------- */
typedef struct { float y_prev, x_prev, alpha; } hpf1_t;
typedef struct { float y_prev, beta; } lpf1_t;
static hpf1_t g_hpf = {0.0f, 0.0f, 0.995f};
static lpf1_t g_lpf = {0.0f, 0.075f};
static inline void hpf1_reset(hpf1_t *h) { h->y_prev = 0.0f; h->x_prev = 0.0f; }
static inline void lpf1_reset(lpf1_t *l) { l->y_prev = 0.0f; }
static inline float hpf1_step(hpf1_t *h, float x) {
    float y = h->alpha * (h->y_prev + x - h->x_prev);
    h->y_prev = y;
    h->x_prev = x;
    return y;
}
static inline float lpf1_step(lpf1_t *l, float x) {
    float y = l->y_prev + l->beta * (x - l->y_prev);
    l->y_prev = y;
    return y;
}

typedef struct { float b0, b1, b2, a1, a2, s1, s2; } biquad_t;
static inline void biquad_reset(biquad_t *q) { q->s1 = q->s2 = 0.0f; }
static inline float biquad_step(biquad_t *q, float x) {
    float y = q->b0 * x + q->s1;
    q->s1 = q->b1 * x + q->s2 - q->a1 * y;
    q->s2 = q->b2 * x - q->a2 * y;
    return y;
}
static biquad_t q_hpf = {0.9937550f, -1.9875101f, 0.9937550f, 1.9874523f, -0.9875679f, 0, 0};
static biquad_t q_lpf = {0.0674553f, 0.1349106f, 0.0674553f, 1.1429805f, -0.4128016f, 0, 0};

static void filter_reset_all(void) {
#if FILTER_MODE == 0
    hpf1_reset(&g_hpf);
    lpf1_reset(&g_lpf);
#else
    biquad_reset(&q_hpf);
    biquad_reset(&q_lpf);
#endif
}

static float filter_process(float xin) {
#if FILTER_MODE == 0
    float y = hpf1_step(&g_hpf, xin);
    y = lpf1_step(&g_lpf, y);
#else
    float y = biquad_step(&q_hpf, xin);
    y = biquad_step(&q_lpf, y);
#endif
    if (!isfinite(y)) y = 0.0f;
    if (y > 5.0f) y = 5.0f;
    if (y < -5.0f) y = -5.0f;
    return y;
}

/* ---------- Adaptive peak detection ---------- */
typedef enum { PK_IDLE, PK_RISING, PK_SMAXED } PeakState;
static PeakState pk = PK_IDLE;
static float env_amp = 0.0f;
static const float ENV_ATTACK = 0.40f;
static const float ENV_DECAY  = 0.02f;
static const float TH_K       = 0.10f;
static unsigned long last_peak_us = 0;
static const unsigned long REFRACTORY_US = 300000UL;
static float prev_y = 0.0f, prev_d1 = 0.0f, prev_d2 = 0.0f;

static void peak_reset_all(void) {
    pk = PK_IDLE;
    env_amp = 0.0f;
    last_peak_us = 0;
    prev_y = prev_d1 = prev_d2 = 0.0f;
}

static unsigned int peak_process(float y) {
    float xabs = fabsf(y);
    env_amp = (xabs > env_amp)
        ? (ENV_ATTACK * xabs + (1.0f - ENV_ATTACK) * env_amp)
        : (ENV_DECAY  * xabs + (1.0f - ENV_DECAY)  * env_amp);
    float thr = TH_K * env_amp;
    float d1 = y - prev_y;
    float d2 = d1 - prev_d1;
    unsigned long now = now_us();
    unsigned int ibi_ms = 0;

    switch (pk) {
        case PK_IDLE:
            if ((y < thr) && (prev_d1 < 0.0f && d1 >= 0.0f)) pk = PK_RISING;
            break;
        case PK_RISING:
            if ((y > thr) && (prev_d2 > 0.0f && d2 <= 0.0f)) pk = PK_SMAXED;
            break;
        case PK_SMAXED:
            if ((y > thr) && (prev_d1 > 0.0f && d1 <= 0.0f) &&
                ((last_peak_us == 0) || (now - last_peak_us > REFRACTORY_US))) {
                if (last_peak_us) {
                    unsigned long ibi_cur = (now - last_peak_us) / 1000UL;
                    if (ibi_cur >= 250 && ibi_cur <= 2000) ibi_ms = (unsigned int)ibi_cur;
                }
                last_peak_us = now;
                pk = PK_IDLE;
            }
            break;
    }

    prev_d2 = d2;
    prev_d1 = d1;
    prev_y = y;
    return ibi_ms;
}

static int is_rail(uint16_t v) { return v == 0 || v == 4095; }
static int is_big_jump(uint16_t a, uint16_t b) { return ((a > b ? a - b : b - a) > 800); }

int main(void) {
    if (wiringPiSetupGpio() != 0) {
        fprintf(stderr, "ERR: wiringPiSetupGpio failed\n");
        return 1;
    }
    pinMode(GPIO_START, INPUT);
    pinMode(GPIO_STOP, INPUT);
    pullUpDnControl(GPIO_START, PUD_UP);
    pullUpDnControl(GPIO_STOP, PUD_UP);
    if (wiringPiISR(GPIO_START, INT_EDGE_FALLING, &start_isr) < 0) return 1;
    if (wiringPiISR(GPIO_STOP, INT_EDGE_FALLING, &stop_isr) < 0) return 1;
    if (wiringPiSPISetupMode(SPI_CH, SPI_SPEED_HZ, 0) < 0) return 1;

    g_uart_fd = serialOpen(UART_DEV, UART_BAUD);
    if (g_uart_fd < 0) fprintf(stderr, "WARN: UART disabled\n");
    signal(SIGINT, on_sigint);
    filter_reset_all();
    peak_reset_all();

    fprintf(stderr, "READY: START=BCM%d, STOP=BCM%d, Fs=%.1f Hz, SPI CE%d\n", GPIO_START, GPIO_STOP, FS_HZ, SPI_CH);
    struct timespec ts = {0, SAMPLE_NS};
    unsigned int diag_cnt = 0;
    uint16_t last_raw = 0xFFFF;

    while (g_state != ST_QUIT) {
        if (!g_running) { nanosleep(&ts, NULL); continue; }
        uint16_t raw12 = 0;
        if (mcp3204_read_ch(MCP3204_CHANNEL, &raw12) != 0) {
            fprintf(stderr, "ERR: SPI read failed\n");
            nanosleep(&ts, NULL);
            continue;
        }
        if (is_rail(raw12) || (last_raw != 0xFFFF && is_big_jump(raw12, last_raw))) {
            static unsigned int clip_cnt = 0;
            if (++clip_cnt % 50 == 0) fprintf(stderr, "WARN: RAW rail/outlier=%u\n", raw12);
            last_raw = raw12;
            nanosleep(&ts, NULL);
            continue;
        }
        last_raw = raw12;
        float v = adc12_to_volt(raw12);
        float y = filter_process(v);
        if (++diag_cnt >= 200) {
            fprintf(stderr, "DIAG RAW=%4u V=%.3f FILT=%.4f\n", raw12, v, y);
            diag_cnt = 0;
        }
        unsigned int ibi_ms = peak_process(y);
        if (ibi_ms > 0) {
            float bpm = 60000.0f / (float)ibi_ms;
            printf("BPM: %.1f\n", bpm);
            fflush(stdout);
            if (g_uart_fd >= 0) {
                char line[32];
                int n = snprintf(line, sizeof(line), "BPM:%.1f\n", bpm);
                if (n > 0) serialPuts(g_uart_fd, line);
            }
        }
        nanosleep(&ts, NULL);
    }
    if (g_uart_fd >= 0) serialClose(g_uart_fd);
    fprintf(stderr, "EXIT\n");
    return 0;
}

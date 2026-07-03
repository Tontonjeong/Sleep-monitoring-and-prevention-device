// server.c — Raspberry Pi 4 sensing/server node: MCP3204 PPG sampling + TCP/IP CSV streaming
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "config.h"

#define SAMPLE_NS 5000000L
#define FS_HZ 200.0

static volatile int running_status = 0;
static volatile int quit_flag = 0;
static volatile unsigned int last_start_ms = 0;
static volatile unsigned int last_stop_ms = 0;
static const unsigned int debounce_ms = 200;
static int latest_bpm = 0;

static unsigned int now_ms_u(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned int)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static unsigned long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000000ul + ts.tv_nsec / 1000ul);
}

static void on_sigint(int sig) { (void)sig; quit_flag = 1; }

static void startSystem(void) {
    unsigned int t = now_ms_u();
    if (t - last_start_ms < debounce_ms) return;
    last_start_ms = t;
    running_status = 1;
    fprintf(stderr, "[EVENT] START\n");
}

static void stopSystem(void) {
    unsigned int t = now_ms_u();
    if (t - last_stop_ms < debounce_ms) return;
    last_stop_ms = t;
    running_status = 0;
    latest_bpm = 0;
    fprintf(stderr, "[EVENT] STOP\n");
}

static int mcp3204_read_ch(uint8_t ch, uint16_t *out12) {
    if (!out12 || ch > 3) return -1;
    unsigned char tx[3] = {0};
    tx[0] = 0x06 | ((ch & 0x04) >> 2);
    tx[1] = (unsigned char)((ch & 0x03) << 6);
    tx[2] = 0x00;
    if (wiringPiSPIDataRW(SPI_CH, tx, 3) < 0) return -1;
    *out12 = (uint16_t)(((tx[1] & 0x0F) << 8) | tx[2]);
    return 0;
}
static inline float adc12_to_volt(uint16_t v) { return (float)v * (MCP3204_VREF / 4095.0f); }

typedef struct { float y_prev, x_prev, alpha; } hpf1_t;
typedef struct { float y_prev, beta; } lpf1_t;
static hpf1_t hpf = {0.0f, 0.0f, 0.995f};
static lpf1_t lpf = {0.0f, 0.075f};
static float hpf_step(float x) { float y = hpf.alpha * (hpf.y_prev + x - hpf.x_prev); hpf.y_prev = y; hpf.x_prev = x; return y; }
static float lpf_step(float x) { float y = lpf.y_prev + lpf.beta * (x - lpf.y_prev); lpf.y_prev = y; return y; }
static float ppg_filter(float x) { float y = lpf_step(hpf_step(x)); if (!isfinite(y)) y = 0; return y; }

typedef enum { PK_IDLE, PK_RISING, PK_SMAXED } PeakState;
static PeakState pk = PK_IDLE;
static float env_amp = 0.0f, prev_y = 0.0f, prev_d1 = 0.0f, prev_d2 = 0.0f;
static unsigned long last_peak_us = 0;

static unsigned int peak_process(float y) {
    float xabs = fabsf(y);
    env_amp = (xabs > env_amp) ? (0.40f * xabs + 0.60f * env_amp) : (0.02f * xabs + 0.98f * env_amp);
    float thr = 0.10f * env_amp;
    float d1 = y - prev_y;
    float d2 = d1 - prev_d1;
    unsigned long t = now_us();
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
                ((last_peak_us == 0) || (t - last_peak_us > 300000UL))) {
                if (last_peak_us) {
                    unsigned long tmp = (t - last_peak_us) / 1000UL;
                    if (tmp >= 250 && tmp <= 2000) ibi_ms = (unsigned int)tmp;
                }
                last_peak_us = t;
                pk = PK_IDLE;
            }
            break;
    }
    prev_d2 = d2; prev_d1 = d1; prev_y = y;
    return ibi_ms;
}

static int is_rail(uint16_t v) { return v == 0 || v == 4095; }
static int is_big_jump(uint16_t a, uint16_t b) { return ((a > b ? a - b : b - a) > 800); }

static int setup_server_socket(void) {
    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) { perror("socket"); return -1; }
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERVER_PORT);
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind"); close(serv_sock); return -1;
    }
    if (listen(serv_sock, 5) == -1) {
        perror("listen"); close(serv_sock); return -1;
    }
    return serv_sock;
}

int main(void) {
    signal(SIGINT, on_sigint);
    if (wiringPiSetupGpio() == -1) return 1;
    if (wiringPiSPISetupMode(SPI_CH, SPI_SPEED_HZ, 0) == -1) return 1;
    pinMode(GPIO_START, INPUT); pullUpDnControl(GPIO_START, PUD_UP);
    pinMode(GPIO_STOP, INPUT); pullUpDnControl(GPIO_STOP, PUD_UP);
    wiringPiISR(GPIO_START, INT_EDGE_FALLING, &startSystem);
    wiringPiISR(GPIO_STOP, INT_EDGE_FALLING, &stopSystem);

    int serv_sock = setup_server_socket();
    if (serv_sock < 0) return 1;
    printf("Server listening on port %d...\n", SERVER_PORT);
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) { perror("accept"); close(serv_sock); return 1; }
    printf("Client connected: %s\n", inet_ntoa(clnt_addr.sin_addr));

    struct timespec sample_ts = {0, SAMPLE_NS};
    uint16_t last_raw = 0xFFFF;
    unsigned int tx_timer = now_ms_u();

    while (!quit_flag) {
        if (running_status) {
            uint16_t raw = 0;
            if (mcp3204_read_ch(MCP3204_CHANNEL, &raw) == 0 && !is_rail(raw) && !(last_raw != 0xFFFF && is_big_jump(raw, last_raw))) {
                float y = ppg_filter(adc12_to_volt(raw));
                unsigned int ibi = peak_process(y);
                if (ibi > 0) latest_bpm = (int)(60000.0f / (float)ibi + 0.5f);
                last_raw = raw;
            }
        }
        unsigned int now = now_ms_u();
        if (now - tx_timer >= 1000) {
            char message[100];
            int bpm_to_send = running_status ? latest_bpm : 0;
            snprintf(message, sizeof(message), "%s,%d,%d", SERVER_SERIAL, bpm_to_send, running_status);
            if (write(clnt_sock, message, strlen(message)) <= 0) break;
            tx_timer = now;
        }
        nanosleep(&sample_ts, NULL);
    }
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

// client.c — Raspberry Pi 4 analysis/client node: TCP receive + EAR IPC + LCD + LED/Buzzer alarm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "config.h"

#if LED_ACTIVE_LOW
  #define LED_ON_LEVEL  LOW
  #define LED_OFF_LEVEL HIGH
#else
  #define LED_ON_LEVEL  HIGH
  #define LED_OFF_LEVEL LOW
#endif

#define LCD_RS 0x01
#define LCD_RW 0x02
#define LCD_EN 0x04
#define LCD_BL 0x08

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int file_read_ear_state(const char *path, float *ear_out, int *drowsy_out) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    float ear;
    int d;
    int ok = (fscanf(fp, "%f %d", &ear, &d) == 2);
    fclose(fp);
    if (!ok) return 0;
    *ear_out = ear;
    *drowsy_out = d;
    return 1;
}

static int lcd_fd = -1;
static void lcd_write4(unsigned char data, unsigned char mode) {
    unsigned char out = 0;
    out |= (data & 0xF0);
    out |= LCD_BL;
    if (mode) out |= LCD_RS;
    wiringPiI2CWrite(lcd_fd, out | LCD_EN);
    delayMicroseconds(50);
    wiringPiI2CWrite(lcd_fd, out & ~LCD_EN);
    delayMicroseconds(50);
}
static void lcd_send(unsigned char value, unsigned char mode) {
    lcd_write4(value & 0xF0, mode);
    lcd_write4((value << 4) & 0xF0, mode);
}
static void lcd_cmd(unsigned char cmd) {
    lcd_send(cmd, 0);
    delayMicroseconds(2000);
}
static void lcd_data(unsigned char ch) { lcd_send(ch, 1); }
static void lcd_init(void) {
    lcd_fd = wiringPiI2CSetup(LCD_I2C_ADDR);
    if (lcd_fd < 0) { perror("wiringPiI2CSetup"); exit(1); }
    delay(50);
    lcd_write4(0x30, 0); delay(5);
    lcd_write4(0x30, 0); delay(5);
    lcd_write4(0x30, 0); delay(5);
    lcd_write4(0x20, 0); delay(5);
    lcd_cmd(0x28);  /* 4-bit, 2-line, 5x8 */
    lcd_cmd(0x0C);  /* display on, cursor off */
    lcd_cmd(0x06);  /* entry mode */
    lcd_cmd(0x01);  /* clear */
    delay(2);
}
static void lcd_set_cursor(int row, int col) {
    static const unsigned char row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row < 0) row = 0;
    if (row > 1) row = 1;
    if (col < 0) col = 0;
    if (col > 15) col = 15;
    lcd_cmd(0x80 | (row_offsets[row] + col));
}
static void lcd_clear(void) { lcd_cmd(0x01); delay(2); }
static void lcd_print_fixed(int row, const char *s) {
    char buf[LCD_COLS + 1];
    int n = (int)strlen(s);
    if (n > LCD_COLS) n = LCD_COLS;
    memset(buf, ' ', LCD_COLS);
    memcpy(buf, s, n);
    buf[LCD_COLS] = '\0';
    lcd_set_cursor(row, 0);
    for (int i = 0; i < LCD_COLS; i++) lcd_data((unsigned char)buf[i]);
}
static void lcd_draw_bpm_bar(int bpm) {
    char line[LCD_COLS + 1];
    memset(line, ' ', LCD_COLS);
    line[LCD_COLS] = '\0';
    if (bpm > 0) {
        int minB = 50, maxB = 120;
        if (bpm < minB) bpm = minB;
        if (bpm > maxB) bpm = maxB;
        float norm = (float)(bpm - minB) / (float)(maxB - minB);
        int bars = (int)(norm * LCD_COLS + 0.5f);
        if (bars < 0) bars = 0;
        if (bars > LCD_COLS) bars = LCD_COLS;
        for (int i = 0; i < bars; i++) line[i] = '#';
    }
    lcd_print_fixed(0, line);
}

static void alarm_off(void) {
    digitalWrite(LED_PIN, LED_OFF_LEVEL);
    digitalWrite(BUZZER_PIN, LOW);
}
static void alarm_on(void) {
    digitalWrite(LED_PIN, LED_ON_LEVEL);
    digitalWrite(BUZZER_PIN, HIGH);
}
static void start_ear_process(void) {
    system("pkill -f run_ear.sh >/dev/null 2>&1");
    system("pkill -f \"./ear\" >/dev/null 2>&1");
    system("bash -c '" EAR_SCRIPT " >/dev/null 2>&1 &'");
}
static void stop_ear_process(void) {
    system("pkill -f run_ear.sh >/dev/null 2>&1");
    system("pkill -f \"./ear\" >/dev/null 2>&1");
}

int main(void) {
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "wiringPiSetupGpio failed\n");
        return 1;
    }
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    alarm_off();
    lcd_init();
    lcd_print_fixed(0, "                ");
    lcd_print_fixed(1, "BPM:--- STOP    ");

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return -1; }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);
    printf("Connecting to server %s:%d...\n", SERVER_IP, SERVER_PORT);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }
    printf("Connected. Waiting for packets...\n");

    char message[128];
    int last_status = 0;
    float last_ear = -1.0f;
    long long ear_low_start_ms = -1;
    int drowsy = 0;
    long long alarm_next_toggle_ms = 0;
    int alarm_state = 0;

    while (1) {
        int str_len = (int)read(sock, message, sizeof(message) - 1);
        if (str_len <= 0) { printf("\nServer disconnected.\n"); break; }
        message[str_len] = '\0';
        char sn[32] = {0};
        int bpm = 0, status = 0;
        if (sscanf(message, "%31[^,],%d,%d", sn, &bpm, &status) < 3) continue;

        if (status == 1 && last_status == 0) {
            printf("\n[EVENT] START -> start EAR engine\n");
            start_ear_process();
            digitalWrite(LED_PIN, LED_ON_LEVEL);
            delay(150);
            digitalWrite(LED_PIN, LED_OFF_LEVEL);
            last_ear = -1.0f;
            ear_low_start_ms = -1;
            drowsy = 0;
            alarm_off();
            alarm_state = 0;
            alarm_next_toggle_ms = 0;
        } else if (status == 0 && last_status == 1) {
            printf("\n[EVENT] STOP -> stop EAR engine + alarm off\n");
            stop_ear_process();
            drowsy = 0;
            alarm_off();
            alarm_state = 0;
            alarm_next_toggle_ms = 0;
            lcd_print_fixed(0, "                ");
            lcd_print_fixed(1, "BPM:--- STOP    ");
        }
        last_status = status;
        if (status == 0) continue;

        lcd_draw_bpm_bar(bpm);
        char line2[32];
        if (bpm > 0) snprintf(line2, sizeof(line2), "BPM:%3d START   ", bpm);
        else snprintf(line2, sizeof(line2), "BPM:--- START   ");
        lcd_print_fixed(1, line2);

        float ear_tmp;
        int d_tmp;
        if (file_read_ear_state(EAR_STATE_FILE, &ear_tmp, &d_tmp)) {
            last_ear = ear_tmp;
            (void)d_tmp;
        }

        long long tms = now_ms();
        if (last_ear > 0.0f && last_ear < EAR_THR) {
            if (ear_low_start_ms < 0) ear_low_start_ms = tms;
            if ((tms - ear_low_start_ms) >= EAR_CLOSED_MS) drowsy = 1;
        } else {
            ear_low_start_ms = -1;
            drowsy = 0;
        }

        if (!drowsy) {
            alarm_off();
            alarm_state = 0;
            alarm_next_toggle_ms = 0;
        } else {
            if (alarm_next_toggle_ms == 0) {
                alarm_on();
                alarm_state = 1;
                alarm_next_toggle_ms = tms + ALARM_ON_MS;
            } else if (tms >= alarm_next_toggle_ms) {
                if (alarm_state) {
                    alarm_off();
                    alarm_state = 0;
                    alarm_next_toggle_ms = tms + ALARM_OFF_MS;
                } else {
                    alarm_on();
                    alarm_state = 1;
                    alarm_next_toggle_ms = tms + ALARM_ON_MS;
                }
            }
        }
    }
    stop_ear_process();
    alarm_off();
    lcd_clear();
    close(sock);
    return 0;
}

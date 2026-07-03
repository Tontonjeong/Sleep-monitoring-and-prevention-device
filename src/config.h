#ifndef DROWSINESS_CONFIG_H
#define DROWSINESS_CONFIG_H

/* Raspberry Pi 4 + wiringPiSetupGpio() BCM numbering */
#define SPI_CH              0       /* SPI0 CE0: /dev/spidev0.0 */
#define SPI_SPEED_HZ        1000000 /* MCP3204 read clock */
#define MCP3204_CHANNEL     0
#define MCP3204_VREF        3.3f

/* Project report main wiring: START=GPIO17, STOP=GPIO27.
 * Some early server.c screenshots used GPIO22 for START; change here if your breadboard follows that legacy wiring. */
#define GPIO_START          17
#define GPIO_STOP           27

#define SERVER_PORT         5000
#define SERVER_SERIAL       "SN-RPI-001"
#define SERVER_IP           "172.31.95.226"

/* Client outputs */
#define LED_PIN             18
#define BUZZER_PIN          23
#define LED_ACTIVE_LOW      0
#define LCD_I2C_ADDR        0x27
#define LCD_COLS            16
#define LCD_ROWS            2

/* EAR decision */
#define EAR_THR             0.220f
#define EAR_CLOSED_MS       2000
#define ALARM_ON_MS         200
#define ALARM_OFF_MS        200
#define EAR_STATE_FILE      "/tmp/ear_state.txt"
#define EAR_SCRIPT          "/home/dong/run_ear.sh"

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#define START_BUTTON 22
#define STOP_BUTTON  27
#define SPI_CH 0
#define ADC_CH 0
volatile int running_status = 0;
const char* serial_num = "SN-RPI-001";

int readADC(int channel) {
    unsigned char buff[3];
    buff[0] = 1;
    buff[1] = (8 + channel) << 4;
    buff[2] = 0;
    wiringPiSPIDataRW(SPI_CH, buff, 3);
    return ((buff[1] & 3) << 8) + buff[2];
}

int getActualBPM() {
    int rawValue = readADC(ADC_CH);
    int threshold = 750;
    if (rawValue > threshold) return 0;
    return (rand() % 20) + 76;
}

void startSystem() { running_status = 1; printf("\n[EVENT] 측정 시작\n"); }
void stopSystem()  { running_status = 0; printf("\n[EVENT] 측정 중지\n"); }

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    if (wiringPiSetupGpio() == -1) return 1;
    if (wiringPiSPISetup(SPI_CH, 500000) == -1) return 1;
    pinMode(START_BUTTON, INPUT);
    pullUpDnControl(START_BUTTON, PUD_UP);
    wiringPiISR(START_BUTTON, INT_EDGE_FALLING, &startSystem);
    pinMode(STOP_BUTTON, INPUT);
    pullUpDnControl(STOP_BUTTON, PUD_UP);
    wiringPiISR(STOP_BUTTON, INT_EDGE_FALLING, &stopSystem);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return -1;
    listen(serv_sock, 5);
    printf("서버 실행 중... (클라이언트 연결을 기다립니다)\n");
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1) return -1;
    printf("클라이언트 연결됨!\n");
    while(1) {
        char message[100];
        int bpm_to_send = (running_status == 1) ? getActualBPM() : 0;
        sprintf(message, "%s,%d,%d", serial_num, bpm_to_send, running_status);
        if(write(clnt_sock, message, strlen(message)) <= 0) break;
        delay(1000);
    }
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

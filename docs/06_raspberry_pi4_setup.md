# 06. Raspberry Pi 4 Setup and Run Guide

## 1. Enable interfaces

```bash
sudo raspi-config
```

활성화:

- SPI: MCP3204 ADC
- I2C: LCD1602
- Camera/USB webcam: `/dev/video0`

## 2. Install packages

```bash
sudo apt update
sudo apt install -y build-essential git i2c-tools libi2c-dev
sudo apt install -y libopencv-dev
```

`wiringPi`는 OS 버전에 따라 별도 설치가 필요할 수 있습니다.

## 3. Check I2C LCD

```bash
i2cdetect -y 1
```

보통 LCD backpack 주소는 `0x27` 또는 `0x3f`입니다. 본 코드 기본값은 `0x27`입니다.

## 4. Check SPI

```bash
ls /dev/spidev*
```

예상:

```text
/dev/spidev0.0
/dev/spidev0.1
```

## 5. Build

```bash
make clean
make
```

## 6. Run server

Server Raspberry Pi:

```bash
sudo ./build/server
```

또는 PPG 단독 진단:

```bash
sudo ./build/ppg
```

## 7. Run client

Client Raspberry Pi에서 server IP를 수정합니다.

```c
#define SERVER_IP "172.31.95.226"
```

실행:

```bash
sudo ./build/client
```

## 8. Run EAR script manually

```bash
bash scripts/run_ear.sh
cat /tmp/ear_state.txt
```

정상 state format:

```text
0.2778 0
0.1800 1
```

## 9. Debug checklist

| 증상 | 확인 |
|---|---|
| LCD 안 켜짐 | I2C address, VCC/GND, SDA/SCL |
| ADC 값 고정 | SPI enable, CE0 wiring, MCP3204 VREF |
| START 안 됨 | pull-up, falling edge, GPIO BCM 번호 |
| EAR 창 안 뜸 | DISPLAY, XAUTHORITY, `/dev/video0` 권한 |
| 알람 안 울림 | LED active-low 여부, buzzer polarity |
| client 접속 실패 | SERVER_IP, port 5000, same network |

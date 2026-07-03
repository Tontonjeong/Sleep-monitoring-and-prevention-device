# 06. Raspberry Pi 4 Setup

## 1. Hardware Assumption

- Raspberry Pi 4 Model B
- Raspberry Pi OS
- SPI enabled
- I2C enabled
- USB webcam 연결
- MCP3204 ADC 연결
- LCD1602 I2C backpack 연결
- LED + Active buzzer 출력 회로 연결

## 2. Basic Setup

```bash
sudo apt update
sudo apt install -y build-essential git pkg-config i2c-tools libopencv-dev opencv-data
sudo raspi-config nonint do_spi 0
sudo raspi-config nonint do_i2c 0
sudo reboot
```

## 3. Build

```bash
git clone https://github.com/Tontonjeong/Sleep-monitoring-and-prevention-device.git
cd Sleep-monitoring-and-prevention-device
make
chmod +x scripts/run_ear.sh
```

## 4. SPI Check

```bash
ls /dev/spidev*
```

Expected:

```text
/dev/spidev0.0  /dev/spidev0.1
```

## 5. I2C Check

```bash
i2cdetect -y 1
```

LCD1602 backpack 주소가 일반적으로 `0x27` 또는 `0x3f`로 표시된다. 다르면 `src/config.h`의 `LCD_I2C_ADDR`를 수정한다.

## 6. Camera Check

```bash
ls /dev/video*
v4l2-ctl --list-devices
```

`run_ear.sh`는 기본적으로 `/dev/video0`을 사용한다.

## 7. Execution

Server Node:

```bash
./server
```

Client Node:

```bash
./client
```

EAR engine standalone test:

```bash
./ear lbfmodel.yaml /usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml /dev/video0 0.22 3.0 1
```

## 8. Common Problems

| 증상 | 원인 후보 | 해결 |
|---|---|---|
| `/dev/spidev0.0` 없음 | SPI 비활성 | `raspi-config`에서 SPI enable |
| LCD 표시 안 됨 | I2C 주소 다름 | `i2cdetect -y 1`로 주소 확인 |
| START 눌러도 반응 없음 | GPIO 번호/풀업/배선 오류 | BCM 번호 확인, GND 연결 확인 |
| 카메라 창 안 뜸 | DISPLAY/XAUTHORITY 문제 | `echo $DISPLAY`, X11 session 확인 |
| EAR 값 -1 지속 | 얼굴 미검출/조도 문제 | 조도 확보, 카메라 각도 조정 |
| LED 항상 켜짐/꺼짐 | active-low 배선 | `LED_ACTIVE_LOW` 변경 |

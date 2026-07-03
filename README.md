# Real-Time Drowsiness Monitoring & Alert Device

Raspberry Pi 4 기반 실시간 졸음 모니터링 및 경고 웨어러블 프로젝트입니다.

## 개요

이 프로젝트는 두 대의 Raspberry Pi 4를 사용하여 생체신호와 영상 기반 졸음 지표를 분산 처리합니다.

- Server Node: PPG analog circuit, MCP3204 ADC, START/STOP interrupt, TCP transmission
- Client Node: USB webcam, EAR analysis, I2C LCD, LED, active buzzer
- Signal Processing: PPG filtering, peak detection, IBI, BPM calculation
- Vision: Eye Aspect Ratio threshold and closed-eye duration check
- Network: TCP/IP socket, default port 5000

## Architecture

```text
PPG Sensor -> Analog Front-End -> MCP3204 ADC -> Raspberry Pi Server
                                                     |
                                                     | TCP/IP
                                                     v
USB Webcam -> EAR Engine -> /tmp/ear_state.txt -> Raspberry Pi Client
                                                     |
                                                     +-- I2C LCD
                                                     +-- LED / Active Buzzer
```

## Core Logic

### PPG to BPM

```text
ADC sample -> HPF -> LPF -> adaptive peak detection -> IBI -> BPM
BPM = 60000 / IBI_ms
```

### EAR Decision

```text
EAR = ( ||p2-p6|| + ||p3-p5|| ) / ( 2 ||p1-p4|| )
```

When EAR is below 0.22 for about 2 seconds, the client triggers visual and audio feedback.

## Portfolio Package

The complete portfolio package prepared from the report, presentation, source-text files, and photos includes:

```text
src/ppg.c
src/server.c
src/client.c
src/ear.cpp
src/config.h
scripts/run_ear.sh
scripts/setup_rpi4.sh
docs/*.md
docs/code/*.md
docs/assets/diagrams/*.svg
docs/assets/images/*.svg
legacy/*.py
index.md
_config.yml
```

The uploaded `송신부 코드.txt` and `수신부코드.txt` files were MPU6050 AirMouse Python scripts, so they are preserved separately under `legacy/` in the prepared package.

## Build

```bash
make
```

## Run

Server Raspberry Pi:

```bash
sudo ./build/server
```

Client Raspberry Pi:

```bash
sudo ./build/client
```

## Note

For real hardware, confirm `src/config.h` before execution: server IP, GPIO BCM numbers, LCD I2C address, camera device, and OpenCV model paths must match the actual Raspberry Pi 4 setup.

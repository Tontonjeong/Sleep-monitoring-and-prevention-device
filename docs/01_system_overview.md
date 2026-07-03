# 01. System Overview

## 1. 설계 목적

본 프로젝트의 목적은 운전 또는 장시간 집중 작업 상황에서 발생 가능한 졸음 상태를 실시간으로 탐지하고, 졸음으로 판단되면 LED와 Active Buzzer를 통해 즉시 사용자에게 경고하는 웨어러블 임베디드 시스템을 구현하는 것입니다.

보고서에서는 졸음 판단을 **PPG 기반 BPM 산출**과 **영상 기반 EAR 분석**의 두 축으로 구성하되, 실제 졸음 판정의 주 지표는 EAR로 설정하고 BPM은 LCD 모니터링 보조 지표로 사용한다고 정리했습니다.

## 2. 전체 시스템 구조

![Full architecture](assets/diagrams/system_architecture_full.svg)

```mermaid
flowchart LR
    PPG[PPG sensor] --> AFE[Analog front-end]
    AFE --> ADC[MCP3204 ADC]
    ADC --> SERVER[Raspberry Pi Server]
    SERVER -->|TCP/IP: SN,BPM,status| CLIENT[Raspberry Pi Client]
    CAM[USB webcam] --> EAR[OpenCV EAR engine]
    EAR -->|stdout| SH[run_ear.sh]
    SH -->|File IPC| STATE[/tmp/ear_state.txt]
    STATE --> CLIENT
    CLIENT --> LCD[LCD1602 I2C]
    CLIENT --> LED[LED]
    CLIENT --> BUZZER[Active Buzzer]
```

## 3. Node 역할 분리

| Node | 주요 기능 | 이유 |
|---|---|---|
| Server Node A | PPG 샘플링, ADC 읽기, BPM 계산, START/STOP ISR, TCP 전송 | 200 Hz 샘플링 안정성 확보 |
| Client Node B | EAR 분석, LCD 표시, LED/Buzzer 제어, 최종 판단 | OpenCV 영상 처리 부하 분리 |

영상 처리를 하나의 라즈베리파이에 함께 올리면 ADC sampling과 BPM 계산이 지연될 수 있습니다. 본 설계는 이러한 병목을 줄이기 위해 하드웨어/연산 부하를 분산했습니다.

## 4. 데이터 흐름

1. PPG 센서가 혈액량 변화를 analog waveform으로 출력합니다.
2. Analog front-end가 포토다이오드 전류를 전압화하고 ADC 입력 가능한 형태로 가공합니다.
3. MCP3204가 PPG analog 신호를 12-bit digital sample로 변환합니다.
4. Server Raspberry Pi가 SPI로 ADC 값을 읽고 BPM/status를 생성합니다.
5. Server는 TCP/IP로 `SN-RPI-001,BPM,status` packet을 Client에 전송합니다.
6. Client는 USB webcam 기반 EAR engine을 실행합니다.
7. `run_ear.sh`는 EAR engine 출력을 파싱하여 `/tmp/ear_state.txt`에 저장합니다.
8. `client.c`는 TCP packet과 EAR state file을 읽어 LCD/LED/Buzzer를 제어합니다.

## 5. 왜 BPM은 보조 지표인가

단순 BPM은 개인차가 크고 졸음 상태에서의 변화 폭이 완만하여 실시간 트리거로 쓰기 어렵습니다. 따라서 최종 구현은 **EAR을 주 트리거**, BPM은 LCD 시각화/생체 모니터링 보조 지표로 설계했습니다.

## 6. 최종 출력

| 출력 | 인터페이스 | 동작 |
|---|---|---|
| LCD1602 | I2C | BPM bar, BPM number, START/STOP 표시 |
| LED | GPIO | 졸음 확정 시 점멸 |
| Active Buzzer | GPIO | 졸음 확정 시 beep |

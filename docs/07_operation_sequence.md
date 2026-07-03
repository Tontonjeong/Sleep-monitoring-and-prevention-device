# 07. Operation Sequence

## 1. 시연 전 준비

1. 웨어러블 밴드를 머리에 착용한다.
2. PPG 센서 클립을 귀에 부착한다.
3. Server Node와 Client Node가 같은 네트워크에 연결되어 있는지 확인한다.
4. Client의 `src/config.h`에서 `SERVER_IP`를 Server Node IP로 설정한다.

## 2. 실행 순서

```mermaid
sequenceDiagram
    participant User
    participant Server as Raspberry Pi 4 Server
    participant Client as Raspberry Pi 4 Client
    participant EAR as EAR Engine
    participant Alarm as LED/Buzzer/LCD

    Server->>Server: ./server 실행, port 5000 listen
    Client->>Server: ./client 실행, TCP connect
    Client->>Alarm: LCD "BPM:--- STOP"
    User->>Server: START button press
    Server->>Server: ISR → running_status=1
    Server->>Client: SN-RPI-001,BPM,1
    Client->>EAR: run_ear.sh start
    EAR->>Client: /tmp/ear_state.txt update
    Client->>Alarm: LCD BPM update
    User->>EAR: eyes closed
    EAR->>Client: EAR < threshold
    Client->>Alarm: LED/Buzzer toggle
    User->>Server: STOP button press
    Server->>Client: SN-RPI-001,0,0
    Client->>EAR: stop process
    Client->>Alarm: alarm off, LCD STOP
```

## 3. 상태별 동작

| 상태 | Server | Client | Output |
|---|---|---|---|
| STOP | TCP 대기/전송, BPM 0 | EAR engine 정지 | LCD `BPM:--- STOP` |
| START | PPG 샘플링/BPM 계산 | EAR engine 실행 | LCD `BPM:xxx START` |
| DROWSY | 계속 packet 전송 | EAR 지속시간 확인 | LED/Buzzer 토글 |
| STOP edge | `running_status=0` | 프로세스 종료 | Alarm off |

## 4. 실제 시연 이미지

| 착용 | 시스템 | 회로 |
|---|---|---|
| ![](assets/images/wearable_side.svg) | ![](assets/images/system_lcd_rpi.svg) | ![](assets/images/circuit_board.svg) |

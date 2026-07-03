# Legacy Uploaded Python Scripts

## 1. 왜 legacy인가

업로드된 `송신부 코드.txt`, `수신부코드.txt`는 파일명과 달리 본 졸음 모니터링 C 코드가 아니라 MPU6050 기반 AirMouse Python script였습니다. 따라서 본 프로젝트 핵심 코드로 섞지 않고 `legacy/`에 별도 보존했습니다.

## 2. 기능 요약

| 파일 | 기능 |
|---|---|
| `legacy/airmouse_sender_provided.py` | MPU6050 gyro 값을 읽고 socket으로 전송, button 상태를 LEFT/RIGHT/SCROLL로 전송 |
| `legacy/airmouse_receiver_provided.py` | 동일한 AirMouse 송신부 코드 변형 |

## 3. 포함 이유

- 사용자가 제공한 모든 파일을 보존하기 위함
- 포트폴리오 본문과 혼동되지 않도록 프로젝트 외부/legacy 자료로 분리
- 실제 졸음 모니터링 core는 PDF/PPT의 `ppg.c`, `server.c`, `client.c`, `run_ear.sh` 기준으로 문서화

## 4. 본 프로젝트와 다른 점

| 항목 | AirMouse script | Drowsiness device |
|---|---|---|
| 센서 | MPU6050 gyro | PPG sensor + webcam |
| 통신 | TCP client send | server/client status/BPM |
| 출력 | 7-segment, buzzer, LED | LCD1602, LED, buzzer |
| 알고리즘 | gyro movement command | PPG filtering + EAR drowsiness decision |

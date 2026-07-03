# Code Deep Dive — `legacy/` Python Files

## 1. 왜 `legacy/`로 분리했는가

업로드된 `송신부 코드.txt`, `수신부코드.txt`에는 MPU6050, 74HC595, socket, RPi.GPIO를 사용하는 Python 기반 에어마우스 코드가 들어 있었다. 이는 본 저장소의 최종 주제인 **PPG/EAR 기반 졸음 모니터링 C 시스템**과 직접 일치하지 않으므로, 포트폴리오 메인 구조에는 섞지 않고 `legacy/` 폴더에 원문 보존했다.

## 2. 파일

| 파일 | 내용 |
|---|---|
| `legacy/airmouse_sender_provided.py` | MPU6050 자이로값, 버튼 상태, sensitivity를 TCP로 전송 |
| `legacy/airmouse_receiver_provided.py` | 업로드된 수신부 텍스트 원문. 실제 내용은 송신부와 거의 동일한 구조 |

## 3. 사용된 요소

- TCP socket client
- I2C MPU6050 register read
- GPIO button input with pull-up
- 74HC595 shift register driving
- LED/Buzzer click feedback

## 4. 포트폴리오 정리 기준

졸음 모니터링 프로젝트의 핵심 코드는 `src/`와 `scripts/`에 두고, 업로드된 Python 파일은 제출 자료 추적성과 재사용 가능성을 위해 별도 보존했다.

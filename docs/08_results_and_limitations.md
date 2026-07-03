# 08. Results & Limitations

## 1. 구현 결과

- Server가 정상적으로 실행되고 Client 접속을 대기한다.
- Client가 Server IP로 접속하면 LCD에 `BPM:--- STOP`이 표시된다.
- START 버튼 입력 시 양쪽 노드에서 센싱/분석이 시작된다.
- BPM은 LCD1602 I2C에 숫자와 bar 형태로 표시된다.
- EAR engine은 HDMI 화면에서 얼굴/눈 상태를 모니터링한다.
- 눈 감음 상태가 지속되면 `DROWSY!` 상태로 판단하고 LED/Buzzer가 동작한다.
- STOP 버튼 입력 시 EAR 프로세스, PPG 센싱, 알람 출력이 중단된다.

## 2. 하드웨어 변경 사항

초기에는 진동모터를 경고 수단으로 고려했으나, 시연 동영상에서는 진동모터 동작 여부가 외부 관찰자에게 명확히 보이지 않는다. 따라서 최종 시연에서는 **LED + Active Buzzer** 중심으로 변경하여 시각/청각적으로 결과를 명확히 확인하도록 설계했다.

## 3. BPM 지표의 한계

BPM은 사람마다 baseline 차이가 크고, 졸음으로 인한 변화가 EAR보다 완만하다. 따라서 본 구현에서는 BPM을 최종 졸음 트리거로 사용하지 않고 **LCD 모니터링 지표**로 사용한다.

## 4. 차세대 개선 방향

| 개선 항목 | 이유 | 구현 방향 |
|---|---|---|
| HRV 분석 | BPM 단독보다 자율신경계 변화 반영 가능 | RMSSD, SDNN, R-R interval 분석 |
| 카메라 robustness | 조도/각도/가림에 민감 | IR camera, face alignment, tracking 적용 |
| 알람 모듈 분리 | 착용감/안전성 개선 | 독립 웨어러블 모듈 + 무선 제어 |
| 사용자 calibration | EAR threshold 개인차 | 초기 30초 baseline 기반 adaptive threshold |
| 데이터 로깅 | 실험 검증 강화 | CSV/SQLite 저장, timestamp 동기화 |

## 5. Demo Gallery

![Demo Session](assets/images/demo_session.svg)

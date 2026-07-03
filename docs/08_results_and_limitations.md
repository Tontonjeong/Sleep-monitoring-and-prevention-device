# 08. Results, Limitations, and Future Work

## 1. 구현 결과

| 항목 | 결과 |
|---|---|
| TCP 연결 | Server가 port 5000에서 대기하고 Client가 접속 |
| 대기 화면 | LCD에 `BPM:--- STOP` 표시 |
| START | 버튼 ISR 발생 후 status=1 전송, PPG/EAR 측정 시작 |
| BPM 표시 | I2C LCD에 BPM 숫자와 bar 표시 |
| EAR 분석 | USB webcam 영상에서 눈 개폐 상태 분석 |
| 졸음 판정 | EAR 0.22 미만 상태 지속 시 `DROWSY!` 경고 |
| 알람 | LED 점멸과 buzzer beep 발생 |
| STOP | 분석/센싱/알람 중지, LCD STOP 복귀 |

## 2. 설계 변경: 진동모터 → LED/Buzzer

초기에는 진동모터를 경고 수단으로 고려했으나, 시연 영상에서는 진동모터 작동 여부가 직관적으로 전달되기 어렵습니다. 따라서 결과 가독성과 전달력을 높이기 위해 **시각적/청각적 피드백 중심의 LED+Buzzer**로 변경했습니다.

## 3. BPM 한계

단순 BPM은 졸음 판정 trigger로 사용하기에 다음 한계가 있습니다.

| 한계 | 설명 |
|---|---|
| 개인차 | 평상시 BPM 범위가 사용자마다 다름 |
| baseline 필요 | 개인별 졸음 기준을 만들려면 긴 샘플링 필요 |
| 변화 폭 작음 | 졸음 시 BPM 변화가 EAR보다 완만함 |
| 임계값 모호 | 단일 threshold로 모든 사용자를 처리하기 어려움 |

따라서 본 프로젝트에서는 BPM을 LCD 모니터링 보조 지표로 재정의했습니다.

## 4. EAR 한계

| 한계 | 설명 |
|---|---|
| 조도 | 얼굴/눈 landmark 검출 성능이 빛에 영향 받음 |
| 얼굴 각도 | 고개 회전, 숙임, 가림이 있으면 EAR 신뢰도 저하 |
| 안경/머리카락 | 눈 영역 landmark occlusion 발생 가능 |
| 판단 지연 | 2초 이상 지속 조건 때문에 빠른 반응과 안정성 사이 trade-off 존재 |

## 5. 향후 개선: HRV 도입

단순 BPM 대신 R-R interval 변동을 분석하는 HRV 접근이 필요합니다.

### RMSSD

\[
RMSSD = \sqrt{\frac{1}{N-1}\sum_{i=1}^{N-1}(RR_{i+1}-RR_i)^2}
\]

### SDNN

\[
SDNN = \sqrt{\frac{1}{N-1}\sum_{i=1}^{N}(RR_i-\overline{RR})^2}
\]

HRV 지표는 자율신경계 변화에 민감하므로, EAR과 결합하면 개인차에 더 강한 졸음 판정으로 발전시킬 수 있습니다.

## 6. 개선 방향

- EAR threshold를 사용자별 calibration 방식으로 변경
- 단순 BPM 대신 HRV/RR interval feature 도입
- 조도 변화 대응을 위한 영상 전처리 강화
- threaded socket receive로 client responsiveness 개선
- LED/Buzzer 외에 진동모터를 독립형 wearable module로 재도입
- 오류 상황별 fail-safe state 추가

# Documentation Index

이 폴더는 PDF 보고서와 PPT 발표자료에 포함된 시스템 구조, 코드, 알고리즘, GPIO, 통신 프로토콜을 포트폴리오용 Markdown 문서로 재구성한 것입니다.

## Architecture

![Full architecture](assets/diagrams/system_architecture_full.svg)

## 문서 목록

| 문서 | 설명 |
|---|---|
| [01_system_overview.md](01_system_overview.md) | 프로젝트 목적, 시스템 구성, 설계 의도 |
| [02_hardware_gpio.md](02_hardware_gpio.md) | GPIO, SPI, I2C, USB, 입력/출력 조건 |
| [03_ppg_signal_processing.md](03_ppg_signal_processing.md) | PPG analog-to-digital pipeline, HPF/LPF, peak detection |
| [04_ear_algorithm.md](04_ear_algorithm.md) | EAR 공식, landmark, threshold, 졸음 판정 |
| [05_tcp_ip_protocol.md](05_tcp_ip_protocol.md) | TCP server/client, packet, socket flow |
| [06_raspberry_pi4_setup.md](06_raspberry_pi4_setup.md) | Raspberry Pi 4 build/run 환경 |
| [07_operation_sequence.md](07_operation_sequence.md) | 실제 시연 순서와 START/STOP 상태 전이 |
| [08_results_and_limitations.md](08_results_and_limitations.md) | 결과, 고찰, HRV 확장 방향 |
| [09_infographics.md](09_infographics.md) | 모든 구조도/파이프라인/인포그래픽 모음 |
| [10_formula_reference.md](10_formula_reference.md) | 코드와 연결되는 공식 전체 모음 |
| [code/README.md](code/README.md) | 코드별 상세 분석 index |

## 코드 분석 문서

- [code/ppg_c.md](code/ppg_c.md)
- [code/server_c.md](code/server_c.md)
- [code/client_c.md](code/client_c.md)
- [code/run_ear_sh.md](code/run_ear_sh.md)
- [code/ear_cpp.md](code/ear_cpp.md)
- [code/legacy_python.md](code/legacy_python.md)

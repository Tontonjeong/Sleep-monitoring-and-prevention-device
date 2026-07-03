# Code Deep Dive — `scripts/run_ear.sh`

## 1. 역할

`run_ear.sh`는 OpenCV EAR engine인 `./ear`를 실행하고, stdout 로그 중에서 유효한 CSV 데이터만 필터링하여 `/tmp/ear_state.txt`에 최신 EAR 상태를 저장한다.

## 2. 실행 구조

```mermaid
flowchart LR
    A[./ear] -->|stdout/stderr| B[run_ear.sh]
    B --> C{Regex match?}
    C -->|yes| D[parse ts, ear, eye_closed, closed_ms, drowsy]
    C -->|no| E[ignore debug log]
    D --> F[if EAR < 0 then drowsy=0]
    F --> G[/tmp/ear_state.txt]
```

## 3. 환경 변수

```bash
export DISPLAY="${DISPLAY:-:0}"
export XAUTHORITY="${XAUTHORITY:-/home/dong/.Xauthority}"
```

HDMI display에 OpenCV 창을 띄우기 위한 설정이다.

## 4. Regex Filter

```bash
^[0-9]+,[-+]?[0-9]*\.?[0-9]+,[01],[0-9]+,[01]$
```

허용되는 형식:

```text
timestamp,ear,eye_closed,closed_ms,drowsy
```

예:

```text
2365780,0.2778,0,0,0
```

## 5. IPC File Format

`client.c`가 읽는 최종 파일은 단순하다.

```text
ear drowsy
```

예:

```text
0.1842 1
```

## 6. 오동작 방지

얼굴 미검출 등으로 EAR이 음수가 되면 drowsy를 강제로 0으로 만든다.

```bash
if awk -v ear="$ear" 'BEGIN{exit !(ear < 0)}'; then
  drowsy=0
fi
```

## 7. 실행

`client.c`가 START packet을 받으면 자동으로 실행한다. 수동 테스트는 다음과 같다.

```bash
chmod +x scripts/run_ear.sh
./scripts/run_ear.sh
cat /tmp/ear_state.txt
```

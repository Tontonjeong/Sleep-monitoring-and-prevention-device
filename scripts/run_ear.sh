#!/bin/bash
set -u
cd /home/dong || exit 1
STATE=/tmp/ear_state.txt
LOG=/tmp/ear_run.log
export DISPLAY="${DISPLAY:-:0}"
export XAUTHORITY="${XAUTHORITY:-/home/dong/.Xauthority}"
rm -f "$STATE"
echo "[run_ear] started at $(date) (DISPLAY=$DISPLAY)" >> "$LOG"
printf "%.4f %d\n" 0.0000 0 > "$STATE"

stdbuf -oL -eL ./ear lbfmodel.yaml \
  /usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml \
  /dev/video0 0.22 3.0 1 2>&1 \
| while IFS= read -r line; do
    echo "$line" >> "$LOG"
    line="${line#\[ear\] }"
    if ! [[ "$line" =~ ^[0-9]+,[-+]?[0-9]*\.?[0-9]+,[01],[0-9]+,[01]$ ]]; then
      continue
    fi
    IFS=',' read -r ts ear eye_closed closed_ms drowsy <<< "$line"
    if awk -v ear="$ear" 'BEGIN{exit !(ear < 0)}'; then
      drowsy=0
    fi
    printf "%.4f %d\n" "$ear" "$drowsy" > "$STATE"
done
echo "[run_ear] exited at $(date)" >> "$LOG"

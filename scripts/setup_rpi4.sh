#!/bin/bash
set -euo pipefail
sudo apt update
sudo apt install -y build-essential git pkg-config i2c-tools raspi-config \
  libopencv-dev opencv-data

# Enable hardware peripherals used by this project.
sudo raspi-config nonint do_spi 0
sudo raspi-config nonint do_i2c 0

# wiringPi is not maintained in some recent Raspberry Pi OS releases.
# Install a compatible package/fork manually if `gpio -v` fails on your image.
if ! command -v gpio >/dev/null 2>&1; then
  echo "[WARN] wiringPi gpio command not found. Install wiringPi-compatible package for your OS."
fi

echo "Reboot is recommended after enabling SPI/I2C."

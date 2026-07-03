# Sleep Monitoring and Prevention Device

Raspberry Pi 4 based real-time drowsiness monitoring and prevention device.

## Overview

This project uses two Raspberry Pi 4 boards in a distributed architecture.

- Server node: PPG sensing, MCP3204 ADC sampling, START/STOP button interrupts, TCP data transmission
- Client node: USB webcam EAR analysis, 1602 I2C LCD display, LED and active buzzer alarm
- Network: TCP/IP socket, default port `5000`
- Drowsiness decision: EAR is the primary trigger. BPM is displayed on the LCD as an auxiliary monitoring value.

## Architecture

```text
PPG Sensor -> Analog Circuit -> MCP3204 ADC -> Raspberry Pi Server
                                                    |
                                                    | TCP/IP: SN,BPM,status
                                                    v
USB Webcam -> EAR Engine -> /tmp/ear_state.txt -> Raspberry Pi Client
                                                    |
                                                    v
                                      I2C LCD + LED + Active Buzzer
```

## Repository Structure

```text
.
├── README.md
├── Makefile
├── src/
│   ├── server.c
│   ├── client.c
│   └── ppg.c
├── scripts/
│   ├── run_ear.sh
│   └── test_gpio.sh
└── docs/
    ├── hardware.md
    ├── software.md
    └── demo_sequence.md
```

## Hardware Summary

### Server Node

| Function | Interface | Default BCM GPIO |
|---|---:|---:|
| START button | GPIO interrupt input | 22 |
| STOP button | GPIO interrupt input | 27 |
| MCP3204 ADC | SPI0 CE0 | 8 |
| PPG output | MCP3204 CH0 | ADC CH0 |

### Client Node

| Function | Interface | Default BCM GPIO / Address |
|---|---:|---:|
| USB webcam | USB | `/dev/video0` |
| 1602 LCD | I2C | `0x27` |
| LED | GPIO output | 24 |
| Active buzzer | GPIO output | 23 |

All GPIO numbers are BCM numbers.

## Build

```bash
sudo apt update
sudo apt install -y build-essential i2c-tools wiringpi
make
```

Enable SPI and I2C with `sudo raspi-config` before running.

## Run

Server Raspberry Pi:

```bash
./build/server
```

Client Raspberry Pi:

```bash
./build/client
```

The client starts `scripts/run_ear.sh` when it receives `status=1` from the server.

## EAR State File

`run_ear.sh` writes the latest EAR state to:

```text
/tmp/ear_state.txt
```

Format:

```text
<ear_value> <drowsy_flag>
```

Example:

```text
0.1871 1
```

## Decision Logic

```text
EAR < 0.22 for about 2 seconds -> DROWSY
DROWSY -> LED blink + buzzer beep + LCD mark '!'
STOP -> kill EAR process + alarm off + LCD returns to STOP state
```

## GPIO Test

```bash
chmod +x scripts/test_gpio.sh
./scripts/test_gpio.sh
```

Manual test:

```bash
pinctrl set 24 op dh
sleep 1
pinctrl set 24 op dl
pinctrl set 23 op dh
sleep 1
pinctrl set 23 op dl
```

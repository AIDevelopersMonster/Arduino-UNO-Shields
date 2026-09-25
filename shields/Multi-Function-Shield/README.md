# Shield 02 — Arduino Multi-Function Shield

Second shield in the **Arduino UNO & Shields** project.

The physical sample photographed for this project visually matches the classic
4-LED Multi-Function Shield family commonly sold for Arduino UNO.

## On-board hardware

- 4-digit 7-segment display
- two serial shift-register ICs used for the display
- four LEDs D1-D4
- three user buttons S1-S3 plus RESET
- 10 kOhm-style potentiometer position marked `VR-A0`
- buzzer position marked `LS1-3`
- IR receiver position marked `U4-IR-2`
- temperature-sensor position marked `U5-18B20-LM35-A4`
- serial expansion header marked for APC220 / Bluetooth / voice-recognition modules
- GND / +5 V / signal expansion headers

## Working pin-map hypothesis

This is the standard map for the classic 4-LED shield family and is the map used
by LAB-02. It still requires bench certification on our exact physical sample.

| Function | Arduino UNO pin |
| --- | ---: |
| APC220 / optional UART | D0 / D1 |
| Optional IR receiver | D2 |
| Buzzer | D3 |
| Display LATCH | D4 |
| Free expansion | D5 |
| Free expansion | D6 |
| Display CLOCK | D7 |
| Display DATA | D8 |
| Free expansion | D9 |
| LED D4 | D10 |
| LED D3 | D11 |
| LED D2 | D12 |
| LED D1 | D13 |
| Potentiometer | A0 |
| Button S1 | A1 |
| Button S2 | A2 |
| Button S3 | A3 |
| LM35 / DS18B20 header | A4 |
| Free expansion | A5 |

LEDs and buttons are expected to be active LOW on this family.

## Display clone note

The common display wiring uses:

- LATCH D4
- CLOCK D7
- DATA D8
- digit-select bytes `F1 F2 F4 F8`

There are known shield clones with different display or digit-select polarity.
LAB-02 therefore includes a runtime `STD / INV` digit-select diagnostic switch.
We will keep the setting at **STD** unless the physical sample proves otherwise.

## Buzzer — bench-verified active mode

For this physical sample we use the buzzer strictly as an **active /
self-oscillating buzzer**.

Bench-verified behavior:

- D3 HIGH -> strong sustained sound
- D3 LOW -> quiet
- frequency/PWM drive makes the buzzer much quieter

For the installed part the default is therefore **ACTIVE**.

LAB-02 v0.6 also supports an explicitly selected **PASSIVE** mode for a future
replacement part or another shield variant. The modes are mutually exclusive:

- ACTIVE: `BUZ,ON`, `BUZ,OFF`, `BEEP,<milliseconds>`
- PASSIVE: `TONE,<Hz>`, `TONE,OFF`

Select with `CFG,BUZZER,ACTIVE` or `CFG,BUZZER,PASSIVE`. Switching type first
forces the output OFF, so the two drive methods cannot overlap.

## Optional interfaces

The first LAB-02 stage intentionally leaves these disconnected:

- D2 IR receiver
- A4 LM35 / DS18B20
- D0 / D1 APC220, Bluetooth or voice module
- D5, D6, D9, A5 external hardware

They will be added only after the onboard hardware path is verified.

## External references used for the initial map

- Hackatronics — Arduino Multi-function Shield Projects:
  https://www.cohesivecomputing.co.uk/hackatronics/arduino-multi-function-shield/
- Jeronimus.Net — Arduino Multi Function Shield:
  https://blog.jeronimus.net/2017/04/arduino-multi-function-shield.html
- ArduinoGetStarted — Multi-Function Shield:
  https://arduinogetstarted.com/tutorials/arduino-multi-function-shield

External references establish the family-level map. Repository results establish
what is actually true for our physical sample.

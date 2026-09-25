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

## APC220 / Bluetooth / Voice Recognition header

The 7-pin header marked for APC220 / Bluetooth / Voice Recognition is not a
separate bus. On the classic Funduino-style shield it exposes the Arduino UNO
hardware UART together with power.

Common family-level pinout, numbered as documented for the Funduino version:

| Header pin | Signal |
| ---: | --- |
| 1 | GND |
| 2 | +5 V |
| 3 | NC |
| 4 | Arduino D1 / TX |
| 5 | Arduino D0 / RX |
| 6 | NC |
| 7 | NC |

For a generic TTL-UART module the signal connection is crossed in the usual
way: Arduino TX (D1) -> module RX, Arduino RX (D0) <- module TX, and GND must be
common.

Important: D0/D1 are the same hardware UART used by the UNO USB-serial path.
External devices on this header can therefore interfere with upload, Serial
Monitor, the LAB-02 GUI, or other USB serial diagnostics. Disconnect the
external UART module during upload unless its interface is known not to drive
the lines.

The exact physical orientation of pin 1 on a clone must be confirmed from
silkscreen/continuity before applying power.

## Jumpers J1 and J2

For the common Funduino-style schematic:

- **J1** connects a 10 kOhm pull-up resistor from +5 V to **A4**, the signal pin
  of the U5 `18B20 / LM35` header.
- **J2** connects 10 kOhm pull-up resistors to the three button inputs
  **A1 / A2 / A3**.

Practical use:

- **DS18B20 on A4:** J1 closed is useful because the 1-Wire data line needs a
  pull-up.
- **LM35 on A4:** J1 should normally be open/removed because the analog LM35
  output should not be biased by that pull-up.
- **Buttons S1-S3:** with J2 closed, the shield provides external pull-ups and a
  pressed button reads LOW.
- LAB-02 firmware uses Arduino `INPUT_PULLUP` for A1-A3, so the buttons can
  still work with J2 open; J2 is therefore not required by our firmware.

### Clone/revision warning

At least one published clone description swaps the **J1/J2 designators** while
describing the same two functions. Therefore the labels must not be treated as
universal across every Multi-Function Shield revision.

For this repository the common Funduino convention is used as the working
reference:

```text
J1 -> A4 10 kOhm pull-up (DS18B20 / LM35 header)
J2 -> A1/A2/A3 button pull-ups
```

The exact mapping on our physical sample should be promoted to bench-verified
only after continuity measurement.

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
- FyzKAB — Funduino Multi-function Shield description and corrected pin map:
  https://kabinet.fyzika.net/dilna/ARDUINO/funduino-popis.php

External references establish the family-level map. Repository results establish
what is actually true for our physical sample.

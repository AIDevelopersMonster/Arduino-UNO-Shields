# Arduino UNO & Shields

Hardware research, documentation, laboratory tests and practical projects
based on Arduino UNO compatible boards and expansion shields.

## Project goals

- identify and document Arduino UNO compatible hardware;
- verify power, USB, UART, GPIO and shield interfaces;
- build reproducible laboratory tests;
- publish working Arduino examples;
- document shield pin usage and compatibility;
- develop practical projects from tested hardware combinations.

## Current hardware

### Arduino UNO compatible board

- ATmega328P platform
- board identification and hardware verification
- power and USB checks
- UART and GPIO tests

See: [boards/Arduino-UNO-Clone](boards/Arduino-UNO-Clone/)

### Shield 01 — W5100 Ethernet + SD

- WIZnet W5100 Ethernet controller
- RJ45 Ethernet
- SD card interface
- network configuration tests
- TCP/UDP experiments
- web server and client examples
- SD read/write tests
- combined Ethernet + SD applications

See: [shields/W5100-Ethernet-SD](shields/W5100-Ethernet-SD/)

### Shield 02 — Multi-Function Shield

Classic 4-LED Arduino Multi-Function Shield family:

- 4-digit 7-segment display driven by two 74HC595 shift registers
- 4 LEDs
- 3 user buttons plus RESET
- A0 potentiometer
- buzzer on D3
- optional IR receiver header
- optional LM35 / DS18B20 header
- APC220 / Bluetooth / voice-module UART header
- free expansion pins

See: [shields/Multi-Function-Shield](shields/Multi-Function-Shield/)

### Shield 03 — MAR2406 2.4-inch TFT Touch

- 240 x 320 TFT
- package marking: ILI9341
- 8-bit parallel LCD bus
- resistive touch panel
- microSD slot

See: [shields/MAR2406-2.4-TFT](shields/MAR2406-2.4-TFT/)

## Laboratories

### LAB-01 — Arduino UNO Clone + W5100 + SD 4 GB

The first complete hardware stand.

Goal: first prove that the UNO, W5100 and SD card work separately and together,
then use the verified stand for practical network projects.

See: [labs/01-UNO-W5100-SD-4GB](labs/01-UNO-W5100-SD-4GB/)

### LAB-02 — Arduino UNO + Multi-Function Shield

Unified diagnostic firmware plus a PC GUI with tabs for live video-friendly
testing of the onboard LEDs, buttons, potentiometer, display and buzzer.

The firmware remains usable from a plain Serial Monitor; the GUI is an optional
visual front end.

See: [labs/02-UNO-MultiFunction-Shield](labs/02-UNO-MultiFunction-Shield/)

GUI: [tools/MFS-GUI](tools/MFS-GUI/)

### LAB-03 — Arduino UNO + MAR2406 2.4-inch TFT Touch Shield

New laboratory for the 240 x 320 parallel TFT shield: controller identification,
display graphics, resistive touch calibration, microSD testing and a small HMI.

Intro video: https://youtube.com/shorts/TexEwx2-TlQ

See: [labs/03-UNO-MAR2406-TFT](labs/03-UNO-MAR2406-TFT/)

## Project structure

- `boards/` — Arduino UNO boards and hardware notes
- `shields/` — individual shield projects
- `labs/` — complete reproducible hardware stands and their test series
- `docs/` — common project documentation
- `examples/` — reusable Arduino sketches and demonstrations
- `tools/` — utilities and test tools

## Status

- **LAB-01** — W5100 + SD hardware diagnostics remain documented.
- **LAB-02** — Multi-Function Shield Stage A PASS on the physical shield: LEDs, buttons, potentiometer, display, active buzzer and GUI/serial control verified.
- **LAB-03** — MAR2406 TFT Touch laboratory started; hardware documentation and initial test plan added.

Optional IR, temperature, UART and external-GPIO interfaces for LAB-02 remain for later stages.

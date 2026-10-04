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
- ILI9341 controller, bench-confirmed
- 8-bit parallel LCD bus
- calibrated resistive touch panel
- microSD slot
- completed LCD + Touch + SD integration test
- final touch-controlled Tic-Tac-Toe demonstrator with persistent SD history

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

Completed laboratory for the MAR2406 TFT shield:

- ILI9341 identified and verified;
- canonical ROT1 / 320x240 landscape established;
- resistive touch wiring and calibration certified;
- microSD read/write verified;
- LCD + Touch + SD integration passed;
- final Tic-Tac-Toe application demonstrator completed for the UNO memory envelope.

Intro video: https://youtube.com/shorts/TexEwx2-TlQ

See: [labs/03-UNO-MAR2406-TFT](labs/03-UNO-MAR2406-TFT/)

### LAB-05 — KonSol / KON-OS on Arduino UNO

Experimental resident cooperative operating environment for Arduino UNO / ATmega328P.

KonSol 0.4 now provides:

- resident cooperative kernel and five scheduled tasks;
- Serial shell and microSD filesystem services;
- direct 8-bit ILI9341 TFT driver;
- direct resistive-Touch service;
- TFT dashboard and Touch File Browser;
- streamed KAP1 VM for external applications stored on microSD;
- RUN -> resident services -> WAIT_TOUCH -> EXIT -> return-to-KonSol lifecycle without reflashing.

KonSol 0.4 physical certification:

- **FULL PHYSICAL PASS**;
- 25720 / 32256 bytes Flash (79%);
- 1228 / 2048 bytes SRAM globals (59%);
- measured free RAM: 812 B after boot, 750 B in the active shell.

Current Zenodo publication (KonSol 0.4): https://doi.org/10.5281/zenodo.23144922

Zenodo record: https://zenodo.org/records/23144922

Historical KonSol 0.1 publication: https://doi.org/10.5281/zenodo.23139756

KonSol 0.1 video: https://www.youtube.com/watch?v=N3PEQUbUmPM

See: [labs/05-UNO-KON-OS](labs/05-UNO-KON-OS/)

## Project structure

- `boards/` — Arduino UNO boards and hardware notes
- `shields/` — individual shield projects
- `labs/` — complete reproducible hardware stands and their test series
- `docs/` — common project documentation
- `examples/` — reusable Arduino sketches and demonstrations
- `tools/` — utilities and test tools

## Status

- **LAB-01** — W5100 + SD hardware diagnostics documented.
- **LAB-02** — Multi-Function Shield Stage A PASS on the physical shield: LEDs, buttons, potentiometer, display, active buzzer and GUI/serial control verified.
- **LAB-03** — **COMPLETE** on 2026-10-04: ILI9341, graphics, ROT1 geometry, resistive touch, microSD and integrated LCD + Touch + SD verified; TEST-08 final application builds on Arduino UNO at 30746 / 32256 bytes Flash (95%) and 1106 / 2048 bytes SRAM globals (54%).
- **LAB-05** — **KonSol 0.4 FULL PHYSICAL PASS** on physical Arduino UNO: resident kernel, Serial shell, microSD, direct TFT, direct Touch, File Browser and external streamed KAP1 application lifecycle verified; current Zenodo DOI 10.5281/zenodo.23144922 (historical 0.1 DOI 10.5281/zenodo.23139756).

Optional LAB-02 IR, temperature, UART and external-GPIO interfaces remain available for future work.

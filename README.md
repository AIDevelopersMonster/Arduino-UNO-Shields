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


### Shield 04 — HY-M302 Multi-Purpose 9-in-1 Shield

Multi-purpose Arduino UNO learning shield, cross-identified with the
Keyestudio KS0183 Multi-purpose Shield V1 family:

- DHT11 temperature/humidity sensor
- LM35 analog temperature sensor
- LDR light sensor and potentiometer
- IR receiver and passive buzzer
- RGB LED plus two indicator LEDs
- two user buttons
- D7/D8 digital, A3 analog, I2C and TTL UART expansion

The published family pin map is documented as a working reference. Exact pin
polarity, RGB channel order and clone-specific wiring remain to be certified on
our physical HY-M302 sample before they are marked PASS.

See: [shields/HY-M302-Multi-Purpose-Shield](shields/HY-M302-Multi-Purpose-Shield/)

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

The published KonSol 0.4 baseline provides:

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

Standalone KonSol 0.6 article: https://doi.org/10.5281/zenodo.23161379

This 0.6 record is a separate technical article for the TEST-08 multi-label result,
not a replacement/version update of the KonSol 0.5 publication.

KonSol 0.5 publication: https://doi.org/10.5281/zenodo.23149141

KonSol 0.5 video: https://youtu.be/HVHLsfV9dFY

Previous KonSol 0.4 publication: https://doi.org/10.5281/zenodo.23144922

Historical KonSol 0.1 publication: https://doi.org/10.5281/zenodo.23139756

KonSol 0.1 video: https://www.youtube.com/watch?v=N3PEQUbUmPM

KonSol 0.5 extends the resident VM with KAP2 registers, Touch-driven loops,
conditional branching and register rendering. TEST-06 physically passed with
the interactive COUNTER.KAP application while retaining KAP1 compatibility.

TEST-07 adds the host-side KASM assembler. Human-readable COUNTER.kasm was
compiled to a generated KAP2 file that compared exactly equal to the physically
certified COUNTER.KAP bytecode.

KonSol 0.6 / TEST-08 extends KAP2 with up to eight indexed labels and symbolic
LABEL/JMP/JZ/JNZ source-level control flow. The physical RED -> YELLOW -> GREEN
sequence, clean APP EXIT 0, 720 B post-run shell free RAM, readable SD filesystem
and unchanged legacy COUNTER.KAP execution were all verified on Arduino UNO.

See: [labs/05-UNO-KON-OS](labs/05-UNO-KON-OS/)

## Projects

### Project 01 — HY-M302 KonSol Operating Environment

Driver-based KonSol line for Arduino UNO + HY-M302:

- resident cooperative kernel;
- HY-M302 measurement and actuator services;
- USB Serial and Bluetooth transports using one protocol;
- IR event/input driver;
- optional SD storage driver;
- optional display driver;
- compact external measurement/control programs;
- future PC CLI/GUI and Android client.

External modules are treated as drivers with explicit pin/resource ownership.
The kernel must detect incompatible configurations instead of assuming every
Arduino library can own the same pins simultaneously.

See: [projects/HY-M302-KonSol-OS](projects/HY-M302-KonSol-OS/)

## Project structure

- `boards/` — Arduino UNO boards and hardware notes
- `shields/` — individual shield projects
- `labs/` — complete reproducible hardware stands and their test series
- `docs/` — common project documentation
- `examples/` — reusable Arduino sketches and demonstrations
- `tools/` — utilities and test tools
- `projects/` — higher-level systems built from certified boards, shields and drivers

## Status

- **LAB-01** — W5100 + SD hardware diagnostics documented.
- **LAB-02** — Multi-Function Shield Stage A PASS on the physical shield: LEDs, buttons, potentiometer, display, active buzzer and GUI/serial control verified.
- **LAB-03** — **COMPLETE** on 2026-10-04: ILI9341, graphics, ROT1 geometry, resistive touch, microSD and integrated LCD + Touch + SD verified; TEST-08 final application builds on Arduino UNO at 30746 / 32256 bytes Flash (95%) and 1106 / 2048 bytes SRAM globals (54%).
- **LAB-05** — **KonSol 0.6 TEST-08 FULL PHYSICAL PASS**. KAP2 now supports indexed multi-label control flow with LABEL/JMP/JZ/JNZ while retaining legacy KonSol 0.5 MARK/JNZ compatibility. MULTI.KAP physically completed RED -> YELLOW -> GREEN -> APP EXIT 0, the old COUNTER.KAP still ran unchanged, post-run shell free RAM remained 720 B, all five cooperative tasks stayed active, and DIR / remained operational. Standalone KonSol 0.6 article: DOI 10.5281/zenodo.23161379. KonSol 0.5 remains a separate publication at DOI 10.5281/zenodo.23149141 with video https://youtu.be/HVHLsfV9dFY.

Optional LAB-02 IR, temperature, UART and external-GPIO interfaces remain available for future work.

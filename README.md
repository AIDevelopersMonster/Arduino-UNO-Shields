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
- IR receiver and active/self-oscillating buzzer on the tested sample
- RGB LED plus two indicator LEDs
- two user buttons
- D7/D8 digital, A3 analog, I2C and TTL UART expansion

The primary onboard pin map, RGB channel order/polarity, buttons, DHT11,
potentiometer, LDR, discrete LEDs, RGB, active buzzer and asynchronous NEC IR
path are bench-certified on the tested HY-M302 sample. The LM35 fitted to this
individual sample failed the cooling-response test and is not treated as a
certified temperature source.

See: [shields/HY-M302-Multi-Purpose-Shield](shields/HY-M302-Multi-Purpose-Shield/)

Overview video: https://youtube.com/shorts/t4WG9GA9Qws

## Libraries

### HY_M302

Low-overhead Arduino library for Shield 04, currently v0.2.0:

- buttons;
- named red/blue indicator LEDs;
- bench-certified RGB control D9=R, D10=G, D11=B;
- active/self-oscillating buzzer control on D5;
- potentiometer, LDR, LM35 raw path and A3;
- compact dependency-free DHT11 reader;
- non-blocking NEC IR decoder with zero-drop counters;
- symbolic remote-key layer and D7/D8 GPIO helpers.

See: [libraries/HY_M302](libraries/HY_M302/)

## Laboratories

### LAB-01 — Arduino UNO Clone + W5100 + SD 4 GB

The first complete hardware stand.

Goal: first prove that the UNO, W5100 and SD card work separately and together,
then use the verified stand for practical network projects.

See: [labs/01-UNO-W5100-SD-4GB](labs/01-UNO-W5100-SD-4GB/)

### LAB-01B — Blue W5100 Ethernet Shield (no microSD)

Independent evidence-driven test series for the blue WIZnet W5100 Ethernet
Shield. This is separate from the original LAB-01 with SD 4GB.

- TEST-01 raw SPI register probe: **PASS** (read/write/restore).
- TEST-02 DHCP + host-side Ping: **PASS** (4/4 replies, 0% loss).
- TEST-03 minimal HTTP server: **FULL PASS** (browser HTML and HTTP 200).
- TEST-04 Ethernet browser GPIO control: **PENDING** (firmware published,
  actual hardware verification still required).
- TEST-05 TCP and TEST-06 UDP: **FULL PASS** for their bounded echo tests.
- [TEST-07 Network Robustness](labs/01-W5100-Ethernet/TEST-07_Network_Robustness/):
  DHCP retries, UDP/TCP recovery, bounded soak and PowerShell event logs;
  **BUILD VERIFIED / 6 hardware scenarios PASS / DhcpOutage DEFERRED**;
  [hardware result](labs/01-W5100-Ethernet/TEST-07_Network_Robustness/RESULT_2026-10-10.md).
- [TEST-08 microSD Hardware](labs/01-W5100-Ethernet/TEST-08_microSD_Hardware/):
  SPI/card/FAT diagnostics, exclusive test-file creation, exact binary readback,
  append, remount, cleanup and PowerShell logs; **HARDWARE PASS v0.2, 15/15 cold-start and operator-reported repeat; prior FAIL retained**;
  [hardware evidence](labs/01-W5100-Ethernet/TEST-08_microSD_Hardware/RESULT_2026-10-10.md).

The lab README includes full pin mapping, serial/Arduino CLI commands,
certification evidence and the TEST-01–10 programme.

See: [labs/01-W5100-Ethernet](labs/01-W5100-Ethernet/)

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

Frozen resident cooperative operating environment for Arduino UNO / ATmega328P
with MAR2406 TFT/Touch and microSD.

The completed KonSol 0.8 / TEST-11 line includes:

- resident cooperative kernel and scheduled services;
- Serial shell and microSD filesystem services;
- direct 8-bit ILI9341 TFT and resistive-Touch services;
- APPS launcher and general FILES browser;
- external KAP1/KAP2 applications stored on microSD;
- KASM host-side assembler/tooling;
- HOST1 machine protocol and verified host file transfer;
- Host Manager GUI plus PowerShell CLI workflows;
- SD-backed boot resource through `/BOOT.TXT`;
- TEST-11 FULL PHYSICAL PASS.

KonSol 0.8 is frozen as a completed reproducible result. New feature growth
requires a separate justified milestone rather than silently changing the
published baseline.

KonSol 0.8 / TEST-11 publication: https://doi.org/10.5281/zenodo.23223036

KonSol 0.8 / TEST-11 video: https://youtu.be/FmeGiIh2ii0

KonSol 0.7 article: https://doi.org/10.5281/zenodo.23197956

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

KonSol 0.7 / TEST-09 adds HOST1, a machine protocol over the existing
USB-TTL Serial link. It supports status inspection, verified file transfer and
remote application lifecycle control. TEST-10 adds the KonSol Host Manager GUI,
including KASM -> KAP build, Install, Download, Run, Stop and Delete without
consuming additional UNO firmware resources.

See: [labs/05-UNO-KON-OS](labs/05-UNO-KON-OS/)

## Projects

### Project 01 — HY-M302 KonSol Operating Environment (design branch)

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

### Project 02 — KSC / KonSol Commander

KSC is the active target-decoupled research line built around one navigable
VFS-like namespace for physical devices, runtime state, configuration and
remote PC files.

The completed Arduino UNO + HY-M302 phase physically validated:

- `/dev`, `/proc`, `/sys`, and remote `/host`;
- one-panel ANSI Commander and shared shell/VFS semantics;
- PC keyboard + HY-M302 IR as semantic CHAR/KEY sources;
- TTY + framed HOSTFS multiplexing on one Serial/COM link;
- streamed host files larger than AVR SRAM using <=32-byte reads;
- explicit-offset retry and BAD_HANDLE reopen/resume;
- 192-byte logical file-view windows without a 192-byte page buffer;
- streamed `.KSC` execution with PRINT / WRITE / WAIT / STOP;
- runtime-selected SW1 behavior loaded from PC without compile, upload or reset.

Final HY-M302 image:

```text
Flash       31024 / 32256 B = 96%
Global SRAM  1556 / 2048 B = 75%
Observed Commander free RAM ~380 B
```

Stage preprint DOI:
https://doi.org/10.5281/zenodo.23251546

Earlier KSC_Core two-target DOI:
https://doi.org/10.5281/zenodo.23232216

KSC-04 video:
https://youtu.be/pqV5DG1o-WA

See: [projects/KSC-KonSol-Commander](projects/KSC-KonSol-Commander/)

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
- **LAB-05** — **KonSol 0.8 / TEST-11 FULL PHYSICAL PASS and frozen**. The resident cooperative environment, external KAP1/KAP2 applications, KASM, TFT/Touch services, APPS/FILES navigation, HOST1, Host Manager and SD-backed boot resource are treated as a completed reproducible research result. Publication DOI: 10.5281/zenodo.23223036.

Optional LAB-02 IR, temperature, UART and external-GPIO interfaces remain available for future work.

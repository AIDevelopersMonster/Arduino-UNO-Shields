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

## Project structure

- `boards/` — Arduino UNO boards and hardware notes
- `shields/` — individual shield projects
- `docs/` — common project documentation
- `examples/` — reusable Arduino sketches and demonstrations
- `tools/` — utilities and test tools

## First laboratory target

**Arduino UNO + W5100 Ethernet Shield + SD Card**

The first stage is to identify the hardware, document the assembly,
verify Ethernet and SD separately, and then test them together in reproducible examples.

## Status

Project started. Shield 01: **W5100 Ethernet + SD**.

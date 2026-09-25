# LAB-02 — Arduino UNO + Multi-Function Shield

## Purpose

LAB-02 turns the classic Multi-Function Shield into one reproducible diagnostic
stand suitable both for hardware validation and for video demonstrations.

Unlike LAB-01, the first LAB-02 stage intentionally uses **one unified firmware**.
The same firmware can be controlled in two ways:

1. plain ASCII commands from Serial Monitor or another terminal;
2. the tabbed Python GUI in [tools/MFS-GUI](../../tools/MFS-GUI/).

The GUI is not required for the hardware to work. It is a visual front end over
a documented serial protocol.

## Hardware

- Arduino UNO compatible board
- classic 4-LED Multi-Function Shield
- USB cable

No optional IR, temperature or UART module is required for Stage A.

## Pin map used by this laboratory

| Function | Arduino UNO pin |
| --- | ---: |
| Buzzer | D3 |
| Display LATCH | D4 |
| Display CLOCK | D7 |
| Display DATA | D8 |
| LED D4 | D10 |
| LED D3 | D11 |
| LED D2 | D12 |
| LED D1 | D13 |
| Potentiometer | A0 |
| Button S1 | A1 |
| Button S2 | A2 |
| Button S3 | A3 |

Optional future interfaces:

| Function | Arduino UNO pin |
| --- | ---: |
| IR receiver | D2 |
| Free expansion | D5, D6, D9, A5 |
| LM35 / DS18B20 | A4 |
| APC220 / Bluetooth / voice UART | D0 / D1 |

## Stage A — unified onboard test

Firmware:

`sketches/01_MFS_Lab/01_MFS_Lab.ino`

GUI:

`../../tools/MFS-GUI/mfs_gui.py`

The first bench session checks:

- USB upload and 115200-baud serial link;
- display multiplexing and digit order;
- independent control of D1-D4;
- live state of S1-S3 with debounce and press counters;
- A0 potentiometer raw range and min/max;
- active-buzzer behavior;
- passive-buzzer tone behavior;
- repeatability of the automated visual test sequence.

## Why the auto test does not print PASS automatically

The Arduino can prove that it drove a pin or read a value, but it cannot by
itself prove that a physical LED illuminated, a segment is wired to the expected
digit, or that a buzzer was audible.

Therefore `TEST,ALL` is an **automated stimulus sequence**, not an automatic
hardware certification. The operator records PASS/FAIL in the results only
after observing the real shield.

## GUI tabs in v0.1

- OVERVIEW
- LEDs
- BUTTONS
- POT A0
- DISPLAY
- BUZZER
- SYSTEM

IR, TEMP, UART and external GPIO tabs are deferred until those interfaces are
physically added and tested.

## Stage B — optional interfaces

After Stage A is certified, add separate test steps for:

1. D2 IR receiver
2. A4 LM35
3. A4 DS18B20 with the appropriate jumper/pull-up configuration
4. D0/D1 serial-module header
5. D5/D6/D9/A5 external experiments

## Rule

A family-level Internet pin map is not treated as proof of this physical sample.
The canonical repository map is updated only from bench evidence.

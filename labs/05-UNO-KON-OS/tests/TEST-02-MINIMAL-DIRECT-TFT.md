# LAB-05 / TEST-02 — KonSol 0.2 Minimal Direct TFT Driver

## Purpose

Extend the physically verified KonSol 0.1 kernel with a system display while
keeping the architecture small enough for Arduino UNO / ATmega328P.

This test deliberately avoids MCUFRIEND_kbv and Adafruit_GFX. The display is
driven directly over the verified MAR2406 8-bit parallel bus.

## Firmware

`sketches/02_KonSol_Minimal_TFT/02_KonSol_Minimal_TFT.ino`

## Direct LCD wiring

```text
LCD D0..D7 = UNO D8,D9,D2,D3,D4,D5,D6,D7

RD  = A0
WR  = A1
RS  = A2
CS  = A3
RST = A4
```

The implementation preserves:

- D0/D1 for Serial;
- D10..D13 for SPI/microSD.

## New system service

KonSol now adds a third cooperative task:

```text
ID  TASK      PERIOD
0   SERIAL    1 ms
1   CLOCK     100 ms
2   DISPLAY   1000 ms
```

The DISPLAY task periodically refreshes the system dashboard.

## Dashboard

Expected screen:

```text
KONSOL 0.2
MINIMAL TFT SYSTEM DRIVER

KERNEL   RUN
SD       READY
RAM      ... B
UPTIME   ... S
SHELL    A:/>

DIRECT ILI9341 8-BIT
NO MCUFRIEND / NO GFX
```

## Build

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull

arduino-cli compile --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\02_KonSol_Minimal_TFT
```

Record Flash and SRAM usage.

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\02_KonSol_Minimal_TFT
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Expected boot messages:

```text
KonSol 0.2
Arduino UNO / ATmega328P / 16 MHz
Cooperative kernel + SD + direct ILI9341
No MCUFRIEND_kbv / No Adafruit_GFX

BOOT: TFT init
BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: ...
Type HELP
A:/>
```

## Shell checks

Run:

```text
INFO
MEM
PS
DIR /
TYPE /XOLOG.TXT
TFT
```

The `TFT` command reinitializes the direct display driver and redraws the
dashboard.

## PASS criteria

TEST-02 passes when:

1. firmware fits in Arduino UNO Flash;
2. runtime free RAM remains practical;
3. direct ILI9341 init produces a readable 320x240 landscape dashboard;
4. Serial shell remains usable;
5. SD mounts and DIR / still works;
6. DISPLAY task counter increases in PS;
7. no display corruption occurs during SD filesystem activity;
8. TFT command restores the dashboard;
9. no reset or filesystem corruption is observed.

## Non-goal

Touch is intentionally not enabled in TEST-02. The resistive touch interface
shares A1/A2/D6/D7 with the LCD bus. Touch will be introduced as a separate
controlled service after the direct display driver is certified.

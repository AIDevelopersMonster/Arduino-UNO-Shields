# LAB-03 — Arduino UNO + MAR2406 2.4-inch TFT Touch Shield

## Goal

Build a reproducible laboratory stand around an Arduino UNO and the MAR2406 2.4-inch TFT shield, progressing from hardware identification to a small touch-controlled HMI.

## Hardware

- Arduino UNO compatible board
- MAR2406 2.4-inch TFT LCD shield
- 240 x 320 display
- package marking: ILI9341
- 8-bit parallel LCD interface
- resistive touch panel
- microSD slot

Detailed shield notes and pin map: [MAR2406 shield documentation](../../shields/MAR2406-2.4-TFT/).

## Test plan

1. Fit the shield to the UNO and verify power/startup behavior.
2. Read/probe the LCD controller ID.
3. Run a minimal display test.
4. Verify colors, drawing primitives, text and screen rotations.
5. Identify and test the resistive touch interface.
6. Calibrate touch coordinates.
7. Test the microSD interface.
8. Combine display + touch + SD where UNO memory permits.
9. Build a small demonstrator HMI.

Continuity measurements will be made only if a particular test exposes an unresolved hardware connection.

## TEST-01 — LCD controller identification — PASS

Firmware:

`sketches/01_LCD_ID_Probe/01_LCD_ID_Probe.ino`

Procedure:

`tests/TEST-01-LCD-ID.md`

TEST-01 uses direct 8-bit bus access and requires no display library. It reads the raw identification/status registers and looks for the ILI9341 `0x9341` signature without initializing the graphics engine.

Bench result on 2026-10-03: `0xD3 -> 00 00 93 41`. The physical sample is therefore verified as **ILI9341**. Full capture: `results/TEST-01-LCD-ID.md`.

## Video

### Start of the laboratory — Arduino UNO + display shield

Short introduction showing the hardware set used to start LAB-03:

**YouTube Shorts:** https://youtube.com/shorts/TexEwx2-TlQ

The video introduces the Arduino UNO + 2.4-inch TFT Touch Shield combination before software diagnostics and functional testing.

## Status

**STARTED — 2026-10-03**

- hardware identified from the photographed unit;
- shield pin map documented;
- introductory video recorded;
- TEST-01 low-level LCD ID probe: **PASS**;
- physical controller ID: **ILI9341 confirmed (`0x9341`)**;
- TEST-02 direct graphics smoke test prepared and ready for bench.

The next certification event is the visible result from TEST-02.

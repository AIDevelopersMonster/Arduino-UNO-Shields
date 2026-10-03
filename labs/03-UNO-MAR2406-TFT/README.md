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

## Video

### Start of the laboratory — Arduino UNO + display shield

Short introduction showing the hardware set used to start LAB-03:

**YouTube Shorts:** https://youtube.com/shorts/TexEwx2-TlQ

The video introduces the Arduino UNO + 2.4-inch TFT Touch Shield combination before software diagnostics and functional testing.

## Status

**STARTED — 2026-10-03**

Hardware identified from the photographed unit. Initial documentation and test plan created. Controller identification is not yet counted as a laboratory PASS until confirmed on the physical unit.

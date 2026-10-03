# TEST-07 — integrated LCD + Touch + microSD

## Purpose

TEST-07 verifies that the three major functional blocks of the MAR2406 shield operate together on the Arduino UNO:

- ILI9341 display;
- calibrated resistive touch;
- microSD storage.

This is the integration test after the independent display, touch, and SD tests.

## Verified configuration used

Display:

```text
ILI9341
ROT1 / 320x240
```

Touch wiring:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified ROT0 touch calibration:

```text
LEFT   = 153
RIGHT  = 930
TOP    = 962
BOTTOM = 168
```

microSD SPI:

```text
CS   = D10
MOSI = D11
MISO = D12
SCK  = D13
```

## Test procedure

The firmware:

1. initializes the TFT;
2. initializes the microSD card;
3. recreates `T07LOG.TXT`;
4. displays five touch targets in canonical landscape orientation;
5. asks for the targets in order: TL, TR, BL, BR, CENTER;
6. appends one point record to the SD card for every accepted touch;
7. reopens the file after the fifth point;
8. counts the recorded point lines;
9. displays `TEST-07 PASS` only when all five records are present.

A missed touch is marked in red and does not advance the sequence.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\07_Display_Touch_SD
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\07_Display_Touch_SD
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

## PASS criterion

The TFT must finish with:

```text
LCD PASS
TOUCH PASS
SD LOG PASS
TEST-07 PASS
```

and Serial must report:

```text
POINT RECORDS=5
VERIFY PASS
TEST-07 PASS
```

This certifies simultaneous use of display, calibrated touch, and microSD on the tested UNO + MAR2406 combination.

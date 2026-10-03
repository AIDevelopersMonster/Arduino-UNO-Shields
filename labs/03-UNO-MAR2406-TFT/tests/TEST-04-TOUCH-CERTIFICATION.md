# TEST-04 — canonical resistive touch certification

## Basis

This test is derived directly from the user-provided working sketch `TFT_LCD_Break_game.ino`.

The working sketch establishes the following touch wiring for this physical shield:

| Signal | UNO pin |
| --- | --- |
| XP | D6 |
| XM | A2 |
| YP | A1 |
| YM | D7 |

It also provides these calibration constants:

```text
TS_LEFT = 270
TS_RT   = 887
TS_TOP  = 267
TS_BOT  = 879
MINPRESSURE = 40
MAXPRESSURE = 2000
```

The reference sketch uses `TouchScreen(XP, YP, XM, YM, 300)` and restores all four shared touch pins to OUTPUT after each `getPoint()` call.

## Purpose

Certify that the shield provides:

1. valid pressure detection;
2. changing raw X and Y coordinates;
3. usable mapping into display coordinates;
4. repeatable press/release behavior.

This is deliberately **not** a game or HMI test.

## Orientation

The known-working source uses `tft.setRotation(0)`.

TEST-04 keeps rotation 0 intentionally. Landscape ROT 1 transformation will be handled only after the underlying touch interface is certified.

## Required libraries

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "MCUFRIEND_kbv"
arduino-cli lib install "TouchScreen"
```

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04_Touch_Certification
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04_Touch_Certification
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Bench sequence

Touch and release at:

1. top-left;
2. top-right;
3. bottom-left;
4. bottom-right;
5. center.

Each press should draw a cross and emit one line:

```text
POINT#1 RAW X=... Y=... Z=... SCREEN X=... Y=...
READY
```

## PASS criteria

TEST-04 passes when:

- pressure is detected for all five touches;
- raw X changes significantly left-to-right;
- raw Y changes significantly top-to-bottom;
- mapped screen coordinates correspond to the physical touch positions;
- center maps near the center of the 240 x 320 rotation-0 frame.

After PASS, the next touch step is to transform the verified raw coordinates into the project canonical landscape ROT 1 / 320 x 240 coordinate system.

# TEST-03 — rotation, geometry and text

## Purpose

Move from the TEST-02 pixel-write proof to a stable display coordinate system for the rest of LAB-03.

Firmware:

`../sketches/03_Rotation_Geometry_Text/03_Rotation_Geometry_Text.ino`

TEST-03 requires **no external display library**.

## What the sketch tests

The firmware exercises all four ILI9341 MADCTL rotations:

| Rotation | MADCTL | Logical size |
| --- | --- | --- |
| 0 | `0x48` | 240 x 320 |
| 1 | `0x28` | 320 x 240 |
| 2 | `0x88` | 240 x 320 |
| 3 | `0xE8` | 320 x 240 |

Each screen contains:

- a white full-frame border;
- two inset rectangles;
- both diagonals;
- a center cross;
- four color-coded logical corner markers;
- corner labels `TL`, `TR`, `BL`, `BR`;
- `ROT N` text;
- logical width/height text.

This checks rotation, address limits, line/rectangle primitives and a small built-in 5x7 bitmap font in one test.

## Build and upload

From the repository root:

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\03_Rotation_Geometry_Text
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\03_Rotation_Geometry_Text
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Boot behavior

On boot the sketch automatically shows rotations 0, 1, 2 and 3 for about 1.8 seconds each, then returns to **ROT 1**.

The serial monitor reports the active MADCTL value and logical dimensions.

## Interactive commands

Send a single character in the serial monitor:

- `0` — rotation 0, 240 x 320;
- `1` — rotation 1, 320 x 240;
- `2` — rotation 2, 240 x 320;
- `3` — rotation 3, 320 x 240;
- `n` — next rotation;
- `a` — replay all four rotations;
- `?` — print help.

## Bench decision

The physical shield is normally viewed in landscape as in the TEST-02 photograph.

Compare **ROT 1** and **ROT 3**. Select the one where:

- `ROT N` is upright;
- `TL` is physically top-left;
- `TR` is physically top-right;
- `BL` is physically bottom-left;
- `BR` is physically bottom-right.

That rotation becomes the canonical **320 x 240 landscape orientation** for later HMI and touch calibration.

## PASS criteria

Record PASS if:

- all four rotations fill the complete screen;
- borders reach all expected edges;
- diagonals and center cross are not truncated;
- text is readable;
- logical corner labels are internally consistent;
- one of ROT 1 / ROT 3 is selected as the project landscape default.

Touch coordinates are **not** part of TEST-03. Touch calibration begins only after this coordinate system is fixed.

# TEST-05 — Touch Paint / full-screen HMI check

## Purpose

This test is inspired by the factory `Example_11_touch_pen` demo, but it is rewritten for the verified current MAR2406 batch and for the LAB-03 canonical landscape orientation.

The factory demo provides the useful interaction idea:

- select a color;
- select a pen size;
- clear the canvas;
- draw with the resistive touch panel.

TEST-05 keeps that concept but does **not** copy the factory touch pinout or factory calibration, because the current tested batch uses different shared pins.

## Verified current-batch touch

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified ROT0 calibration seed:

```text
LEFT   = 151
RIGHT  = 920
TOP    = 964
BOTTOM = 169
```

## Canonical HMI orientation

The display is run in:

```text
ROT1 / 320x240
```

Touch is mapped first into the certified ROT0 portrait geometry and then transformed to the ROT1 landscape geometry.

Expected transform:

```text
landscapeX = portraitY
landscapeY = 239 - portraitX
```

TEST-05 therefore also certifies that the calibrated touch coordinate system is aligned with the project HMI coordinate system.

## User interface

Top toolbar:

- six colors: red, yellow, green, cyan, blue, magenta;
- three pen sizes;
- CLEAR.

Canvas:

- black drawing area;
- subtle inner frame;
- five small reference crosses near the four corners and center.

The reference marks make it easy to detect:

- swapped axes;
- mirrored axes;
- dead zones near edges;
- strong non-linearity;
- large offset from the intended stylus position.

## PASS criteria

PASS if all of the following are true:

1. Drawing follows the stylus in the same direction.
2. No X/Y swap is visible.
3. No horizontal or vertical mirroring is visible.
4. Lines can be drawn close to all four canvas edges.
5. Center drawing is correctly aligned.
6. All six color buttons work.
7. All three pen sizes work.
8. CLEAR erases the canvas without breaking touch input.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\05_Touch_Paint
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\05_Touch_Paint
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

The serial monitor prints one `DOWN` record per new touch and one `UP` record on release.

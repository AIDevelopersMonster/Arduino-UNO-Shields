# TEST-04 — interactive resistive-touch calibration

## Purpose

TEST-04 is now an interactive calibration wizard for the current tested MAR2406 batch.

Verified touch wiring:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Initial seed calibration:

```text
LEFT   = 167
RIGHT  = 931
TOP    = 964
BOTTOM = 190
```

These seed values are only the starting point. The sketch recalculates calibration from the actual physical touches.

## How the wizard works

Five fixed targets are drawn safely inside the display:

1. TL
2. TR
3. BL
4. BR
5. CENTER

The target never moves.

A small green dot shows where the current calibration maps the physical touch.

### Phase 1 — collect

Touch each of the five targets once.

The sketch stores:

- expected screen X/Y;
- raw touch X/Y.

After the fifth point, it performs a least-squares linear fit independently for X and Y and derives new raw endpoints:

- LEFT / RIGHT;
- TOP / BOTTOM.

### Phase 2 — verify and refine

The same five targets are shown again.

For each target:

- if mapped error is within +/-4 pixels on both axes, the target passes;
- otherwise the new raw measurement is added to the data set;
- calibration is recomputed immediately;
- the same fixed target is presented again.

This prevents the target from "chasing" the measurement. The calibration converges toward the fixed geometry instead.

## Serial output

Example:

```text
TARGET=TL RAW X=224 Y=789 Z=777 SCREEN X=18 Y=72 EXPECT X=20 Y=70 ERR dx=-2 dy=2
PASS TL
READY
```

When a point needs refinement:

```text
TARGET=TR ...
REFINE TR
CAL LEFT=... RIGHT=... TOP=... BOTTOM=...
READY
```

At completion:

```text
CALIBRATION COMPLETE
CAL LEFT=...
RIGHT=...
TOP=...
BOTTOM=...
```

The final values are also shown on the TFT.

## Libraries

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

## PASS criterion

All five verification targets must pass within +/-4 pixels in both axes.

The resulting four raw calibration endpoints become the certified constants for this physical sample / batch.

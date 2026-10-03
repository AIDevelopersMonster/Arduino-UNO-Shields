# TEST-04C — guided touch point capture

## Why TEST-04C replaces the continuous stream

The continuous TEST-04A/04B diagnostics were too noisy for a human bench procedure because values were printed even when the panel was not being touched.

TEST-04C changes the method:

- no continuous output;
- the operator places the stylus at a specified point;
- one serial command captures one point;
- 15 samples are taken;
- the median X/Y/Z values are printed once.

## Touch mapping

Return to the standard shield-family mapping:

| Touch electrode | UNO pin |
| --- | --- |
| XP | D8 |
| XM | A2 |
| YP | A3 |
| YM | D9 |

This mapping is the common arrangement for 2.4-inch UNO ILI9341 resistive shields. The earlier TEST-04A failure is therefore treated as a diagnostic-method issue, not as sufficient evidence to reject the physical pinout.

## Dependency

```powershell
arduino-cli lib install "Adafruit TouchScreen"
```

## Build and upload

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04C_Guided_Touch_Capture
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04C_Guided_Touch_Capture
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Bench procedure

Hold the stylus at the requested point and send one digit:

- `1` = top-left
- `2` = top-right
- `3` = bottom-left
- `4` = bottom-right
- `5` = center

Expected format:

```text
POINT TOP-LEFT : X=... Y=... Z=...
```

Do not move the stylus until that line appears.

## What we need

Send exactly the five resulting POINT lines.

From those five records we can determine:

- whether both axes are real and independent;
- which raw axis maps to screen X;
- which raw axis maps to screen Y;
- whether either axis is reversed;
- approximate raw edge values;
- whether Z is usable as a press threshold.

Only after this five-point capture do we create the 320 x 240 calibration constants.

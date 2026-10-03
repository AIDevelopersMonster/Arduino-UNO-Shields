# TEST-04 — raw resistive touch diagnostics

## Purpose

Identify and bench-verify the real 4-wire resistive touch connection on the tested MAR2406 shield, then record raw ADC ranges before any coordinate calibration.

Firmware:

`../sketches/04_Raw_Touch/04_Raw_Touch.ino`

No external touch library is required.

## Candidate wiring under test

TEST-04 starts with the common pin assignment used by many 2.4-inch UNO parallel TFT shields:

| Touch electrode | Arduino UNO |
| --- | --- |
| XP | D8 |
| XM | A2 |
| YP | A3 |
| YM | D9 |

This is a **candidate**, not yet project-certified hardware data.

The four lines are shared with the TFT parallel interface, so the sketch keeps the LCD deselected while taking touch samples and restores the shared pins after each conversion.

## Build and upload

From the repository root:

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04_Raw_Touch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04_Raw_Touch
arduino-cli monitor -p COM4 -c baudrate=115200
```

## What to do on the bench

First leave the panel untouched for several samples.

Then press, one location at a time:

1. top-left;
2. top-right;
3. bottom-left;
4. bottom-right;
5. center.

For each position, hold the stylus/finger still for about one second and copy several consecutive serial lines.

The output format is:

```text
X=<raw> Y=<raw> Z1=<raw> Z2=<raw> P=<proxy> | X[min,max] Y[min,max] P[min,max]
```

Command `r` resets the observed min/max statistics.

## What counts as evidence that the pin mapping is correct

A useful mapping should show all three properties:

- X changes strongly when moving horizontally;
- Y changes strongly when moving vertically;
- pressure/contact values change clearly between released and pressed states.

The axis direction may be reversed. That is normal and belongs to the calibration stage.

## PASS criteria

Record **PASS / WIRING CONFIRMED** only if repeated touches produce stable, position-dependent X/Y readings and a usable press/release distinction.

If the values are stuck, random, or one axis does not respond, do **not** calibrate yet. We will then test the alternate shield-family pin arrangements or use targeted continuity measurements only on the unresolved touch lines.

## Next step after PASS

TEST-05 will map the verified raw ranges to the already-fixed display coordinate system:

- display: 320 x 240;
- orientation: ROT 1;
- MADCTL: `0x28`.

That stage will determine the actual raw edge values and axis inversion/swap needed for accurate touch coordinates.

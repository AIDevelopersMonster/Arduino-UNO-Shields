# TEST-04D — event-driven raw touch matrix

## Why this version exists

TEST-04A proved that the panel electrically reacts to direct GPIO/ADC probing.
TEST-04C, which switched to the Adafruit TouchScreen library, did not react on the tested sample.

Therefore TEST-04D returns to the direct raw method that worked, while removing the continuous idle stream.

## Behavior

The firmware stays silent while idle.

When a press is detected it prints exactly one block containing four direct measurements, then waits for release before allowing the next capture.

Suggested physical order:

1. top-left;
2. top-right;
3. bottom-left;
4. bottom-right;
5. center.

## Build and upload

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04D_Touch_Matrix
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04D_Touch_Matrix
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Expected output

One press produces one block:

```text
TOUCH #1
  M1 D8=H A2=L read A3 : ...
  M2 A3=H D9=L read A2 : ...
  M3 D9=H A2=L read A3 : ...
  M4 A3=H D8=L read A2 : ...
RELEASE
READY
```

There is no continuous stream.

## What to send back

Send the five TOUCH blocks in order TL, TR, BL, BR, CENTER.

From those 20 measurements we can determine which two drive/sense combinations actually encode the two independent panel axes on this physical shield.

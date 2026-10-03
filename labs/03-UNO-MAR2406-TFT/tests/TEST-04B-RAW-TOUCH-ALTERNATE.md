# TEST-04B — alternate raw touch mapping

## Why this retest exists

TEST-04A produced a usable change on only one axis:

- X stayed near ~800;
- Y swept over almost the full ADC range.

Therefore Candidate A is not suitable for calibration.

TEST-04B swaps to the alternate common shield mapping:

| Electrode | UNO pin |
| --- | --- |
| XP | D9 |
| YP | A2 |
| XM | A3 |
| YM | D8 |

This keeps the two ADC sense nodes on A2 and A3.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04B_Raw_Touch_Alternate
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04B_Raw_Touch_Alternate
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Bench sequence

1. no touch;
2. top-left;
3. top-right;
4. bottom-left;
5. bottom-right;
6. center.

Hold each point about one second.

## What we need to see

A good result will have:

- X changing substantially left-to-right;
- Y changing substantially top-to-bottom;
- repeatable values when holding a fixed point;
- a distinguishable pressure/contact response.

Only then do we proceed to calibration.

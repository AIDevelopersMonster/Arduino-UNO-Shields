# TEST-04F — revised raw touch candidate

## Candidate pins

Based on the actual MAR2406 shield layout and the behavior of the earlier failed mapping, TEST-04F tests:

| Touch electrode | UNO pin |
| --- | --- |
| XP | D6 |
| XM | A2 |
| YP | A1 |
| YM | D7 |

This is still a hypothesis until the physical sample produces two independent position axes.

## Why this candidate

The earlier D8/D9/A2/A3 mapping clearly detected electrical contact but did not yield two usable coordinates.

The revised candidate moves the digital touch pair to D6/D7 and uses A1/A2 as the analog sense lines.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04F_Raw_Touch_D6_D7_A1_A2
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04F_Raw_Touch_D6_D7_A1_A2
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Bench sequence

With the display lying in landscape, long side horizontal:

1. top-left
2. top-right
3. bottom-left
4. bottom-right
5. center

Fully release between points.

## Expected good result

A correct mapping should show:

- X changing strongly from left to right;
- Y changing strongly from top to bottom;
- center values lying roughly between edge values;
- a repeatable pressure/contact response.

Send the five POINT lines exactly as printed.

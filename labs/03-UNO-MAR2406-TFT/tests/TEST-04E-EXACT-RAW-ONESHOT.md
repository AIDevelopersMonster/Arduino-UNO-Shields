# TEST-04E — exact raw engine, one-shot output

## Reason

The original direct TEST-04 was the only version that clearly reacted to touch on the physical shield.

Later tests changed more than the presentation layer:

- TEST-04B changed the pin arrangement and sampling topology;
- TEST-04C introduced the Adafruit TouchScreen library;
- TEST-04D changed the low-level pin preconditioning/order.

That was too many variables at once.

TEST-04E returns to the **exact acquisition sequence from the original reacting TEST-04**. Only the output logic is changed.

## What is intentionally unchanged

- XP = D8
- XM = A2
- YP = A3
- YM = D9
- GPIO direction changes
- ADC settling method
- shared-pin restore sequence
- pressure proxy

No external library is used.

## What is changed

The firmware samples internally but prints nothing while idle.

Based on the real first capture:

- idle pressure proxy was about 640;
- clear touch pressure proxy was about 1090.

Therefore TEST-04E uses:

- press threshold: P > 900
- release threshold: P < 800

One press prints one averaged line, then the firmware waits for release.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04E_Exact_Raw_OneShot
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\04E_Exact_Raw_OneShot
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Physical sequence

Touch and fully release between each point:

1. top-left
2. top-right
3. bottom-left
4. bottom-right
5. center

Expected:

```text
POINT#1 X=... Y=... Z1=... Z2=... P=...
READY
POINT#2 X=... Y=... Z1=... Z2=... P=...
...
```

If this version reacts, we have isolated the failure in the later test redesign rather than in the panel itself.

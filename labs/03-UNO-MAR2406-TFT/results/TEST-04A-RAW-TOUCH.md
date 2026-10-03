# TEST-04A — raw touch candidate A result

**Date:** 2026-10-03

Candidate A wiring tested:

- XP = D8
- XM = A2
- YP = A3
- YM = D9

## Bench capture summary

Observed accumulated ranges:

- X: approximately 788..829
- Y: approximately 3..907
- pressure proxy: approximately 606..1103

Representative stable released-like samples were around:

```text
X=808 Y=76
```

Representative pressed samples reached approximately:

```text
X=820 Y=893
```

## Interpretation

Candidate A does **not** provide two independent position axes:

- X remains confined to a very narrow band (roughly 40 ADC counts);
- Y changes over almost the full ADC span;
- therefore the mapping cannot yet be treated as the real 4-wire electrode assignment.

The pressure proxy also exceeds the nominal 10-bit ADC range because TEST-04A used a simple diagnostic expression rather than a calibrated resistance model.

## Result

**INCONCLUSIVE / CANDIDATE A REJECTED FOR CALIBRATION**

Do not proceed to screen calibration with this pin assignment.

Next step: TEST-04B with the alternate common UNO resistive-touch mapping:

- XP = D9
- YP = A2
- XM = A3
- YM = D8

and the coordinate/pressure sampling sequence aligned with the standard TouchScreen algorithm.

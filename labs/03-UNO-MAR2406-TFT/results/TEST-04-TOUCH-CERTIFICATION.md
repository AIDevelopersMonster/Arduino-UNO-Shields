# TEST-04 — touch calibration result — PASS

Date: 2026-10-03

## Hardware / batch

Arduino UNO compatible board + MAR2406 2.4-inch TFT Touch Shield.

For the currently tested batch, the verified resistive-touch wiring is:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

## Interactive calibration result

The five-target calibration wizard completed successfully.

Final calibration:

```text
TS_LEFT = 153
TS_RT   = 930
TS_TOP  = 962
TS_BOT  = 168
```

Pressure window:

```text
MINPRESSURE = 40
MAXPRESSURE = 2000
```

TouchScreen resistance parameter:

```text
300 ohm
```

## Verification pass

Final verification / refinement sequence:

```text
TL      RAW X=216 Y=785 -> SCREEN X=19  Y=71   EXPECT X=20  Y=70   ERR -1,+1  PASS
TR      RAW X=859 Y=795 -> SCREEN X=217 Y=66   EXPECT X=219 Y=70   ERR -2,-4  PASS
BL      RAW X=219 Y=206 -> SCREEN X=20  Y=303  EXPECT X=20  Y=299  ERR  0,+4  PASS
BR      RAW X=858 Y=215 -> SCREEN X=216 Y=300  EXPECT X=219 Y=299  ERR -3,+1  PASS
CENTER  RAW X=539 Y=567 -> SCREEN X=118 Y=158  EXPECT X=120 Y=160  ERR -2,-2  PASS
```

The BL point required one refinement step before passing.

## Conclusion

**PASS**

- both touch axes are functional;
- X increases from left to right;
- Y decreases from top to bottom in raw coordinates;
- all five verification targets passed within the configured +/-4 px tolerance;
- final calibrated constants are recorded above.

These constants are certified for the tested physical sample / current batch and must not be treated as universal for all visually similar MAR2406 shields.

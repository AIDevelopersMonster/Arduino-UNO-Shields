# TEST-04E — exact raw engine / one-shot result

**Date:** 2026-10-03  
**Physical orientation:** shield/display lying in landscape, long side horizontal  
**Touch order:** TL -> TR -> BL -> BR -> CENTER

## Captured values

```text
POINT#1 TL     X=765 Y=837 Z1=835 Z2=768 P=1089
POINT#2 TR     X=766 Y=739 Z1=743 Z2=727 P=1038
POINT#3 BL     X=772 Y=841 Z1=838 Z2=775 P=1086
POINT#4 BR     X=768 Y=730 Z1=723 Z2=721 P=1024
POINT#5 CENTER X=765 Y=195 Z1=210 Z2=496 P=737
```

## Interpretation

The direct raw engine definitely reacts to physical touch, but it does **not** provide two usable independent coordinate axes:

- X remains almost constant: 765..772 across all five points;
- Y separates left-side and right-side corner presses only weakly (~837/841 versus ~739/730);
- the center reading (Y=195) is inconsistent with a normal monotonic 4-wire coordinate interpolation;
- pressure proxy changes strongly, confirming real electrical interaction with the resistive panel.

Therefore the panel is reacting, but the assumed electrode-to-UNO mapping cannot yet be certified as a valid two-axis mapping.

## Result

**TOUCH DETECTED / AXIS MAPPING NOT VERIFIED**

Software calibration to 320 x 240 must not be attempted yet.

The next justified step is targeted resistance/continuity measurement of the four candidate touch lines with the shield unpowered, rather than more arbitrary pin permutations.

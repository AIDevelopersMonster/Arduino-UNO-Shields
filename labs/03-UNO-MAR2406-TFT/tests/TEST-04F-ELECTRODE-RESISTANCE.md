# TEST-04F — targeted touch-electrode resistance check

## Why hardware measurement is now justified

TEST-04E proves that the panel reacts electrically, but one raw axis is effectively stuck.

At this point further software permutations would add ambiguity. A 4-wire resistive panel has a simple electrical signature that can be checked directly.

## Safety / setup

1. Disconnect USB and all power.
2. Preferably remove the TFT shield from the UNO.
3. Multimeter in resistance mode (ohms), not powered continuity injection if avoidable.
4. Do not press the touch panel for the first measurements.

## Candidate touch lines

The current assumed mapping is:

- D8 = XP
- A2 = XM
- A3 = YP
- D9 = YM

If that mapping is correct, the two opposite-electrode pairs should be:

- **D8 <-> A2** : one resistive sheet
- **A3 <-> D9** : the other resistive sheet

Each should normally show a finite resistance, typically a few hundred ohms to around 1 kOhm depending on the panel.

The cross-pairs should be high/open with no touch and change when the panel is pressed.

## Measure these six pairs

Record resistance with NO touch:

1. D8 - A2
2. A3 - D9
3. D8 - A3
4. D8 - D9
5. A2 - A3
6. A2 - D9

Then press near the center and repeat the four cross-pairs:

- D8 - A3
- D8 - D9
- A2 - A3
- A2 - D9

## Expected pattern for a healthy 4-wire panel

Exactly two pairings should behave as stable low/fixed resistances independent of touch. Those are the two sheet end-to-end pairs.

The remaining four are cross-layer combinations. They should be open/high without touch and become finite/variable when pressed.

This directly identifies the real electrode pairing and tells us whether one sheet is broken or whether our UNO pin assignment is wrong.

## What to report

Send the six no-touch resistance values and, for the four cross-pairs, the pressed-center values.

No further firmware change is needed until this matrix is known.

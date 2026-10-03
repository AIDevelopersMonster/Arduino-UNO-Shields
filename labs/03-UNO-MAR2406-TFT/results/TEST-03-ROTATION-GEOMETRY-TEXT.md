# TEST-03 — rotation, geometry and text result

**Date:** 2026-10-03  
**Board:** Arduino UNO compatible  
**Shield:** MAR2406 2.4-inch TFT Touch  
**Controller:** ILI9341  
**Firmware:** `sketches/03_Rotation_Geometry_Text/03_Rotation_Geometry_Text.ino`

## Observed result

The physical display shows:

- `ROT 1` upright;
- logical dimensions `W320 H240`;
- `TL` in the physical top-left corner;
- `TR` in the physical top-right corner;
- `BL` in the physical bottom-left corner;
- `BR` in the physical bottom-right corner;
- full-frame borders reaching the screen edges;
- both diagonals;
- a centered cross;
- readable bitmap text;
- intact corner markers and geometry.

No gross clipping, axis swap or address-window corruption is visible.

## Canonical landscape orientation

**Selected project default: ROT 1**

- MADCTL: `0x28`
- logical size: **320 x 240**
- intended use: normal landscape position of the Arduino UNO + MAR2406 shield as photographed in the laboratory

This coordinate system becomes the canonical frame for subsequent HMI and touch work.

## Result

**PASS — rotation, geometry and text verified.**

Verified by TEST-03:

- landscape mode works;
- geometry primitives work;
- text rendering works;
- logical coordinate limits are correct;
- canonical HMI orientation is fixed at 320 x 240.

Next step: identify the resistive-touch wiring and build TEST-04 raw touch diagnostics using this fixed display coordinate system.

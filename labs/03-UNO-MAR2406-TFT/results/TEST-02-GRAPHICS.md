# TEST-02 — ILI9341 graphics smoke-test result

**Date:** 2026-10-03  
**Board:** Arduino UNO compatible  
**Shield:** MAR2406 2.4-inch TFT Touch  
**Controller:** ILI9341 (confirmed by TEST-01)  
**Firmware:** `sketches/02_ILI9341_Graphics/02_ILI9341_Graphics.ino`

## Observed result

The display successfully left its uninitialized state and rendered the final six-color RGB565 test pattern with clean, full-width bands.

Observed on the physical screen, from top to bottom in the photographed board orientation:

1. magenta
2. yellow
3. white
4. blue
5. green
6. red

This is consistent with the TEST-02 logical portrait pattern

`red | green | blue | white | yellow | magenta`

being viewed after a 90-degree counter-clockwise physical rotation of the portrait coordinate system.

No gross address-window corruption, missing bands or obvious color-channel failure was visible.

## Result

**PASS — visible RGB565 pixel writes confirmed.**

Verified by TEST-02:

- ILI9341 initialization succeeds;
- 8-bit parallel write path works;
- address-window programming works;
- RGB565 color writes work;
- full-screen and rectangular fills work;
- the physical panel is usable for the next graphics stage.

## Orientation note

TEST-02 used ILI9341 MADCTL `0x48`, i.e. a 240 x 320 portrait logical frame.

The shield is naturally convenient to use in the photographed 320 x 240 landscape orientation. TEST-03 will therefore explicitly test all four MADCTL rotations and select the landscape orientation used by later HMI work.

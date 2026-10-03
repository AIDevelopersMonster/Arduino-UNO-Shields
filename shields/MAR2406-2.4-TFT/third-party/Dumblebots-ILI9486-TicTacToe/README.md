# Third-party reference — Dumblebots / Aditya Agarwal Tic-Tac-Toe

This directory preserves a third-party reference implementation reviewed during the MAR2406 study.

## Upstream

- Author / repository owner: Aditya Agarwal
- Dumblebots article: https://www.dumblebots.com/blog/using-3-5-tft-lcd-display-ili9486-arduino-part-6-tictactoe-game
- Upstream repository: https://github.com/Aditya-A-garwal/Arduino-TFT-LCD-3-5-Tic-Tac-Toe
- Reviewed upstream commit: `8c48faccfad93f6e841765afca4ae3f49c953f8c`
- License: GNU GPL v3.0

The files under `upstream/` are retained as an unmodified reference copy of the upstream source files reviewed for this project. The upstream GPL-3.0 license is included in this directory.

## What the upstream program does

The program implements a touch-controlled Tic-Tac-Toe game for a 3.5-inch ILI9486 TFT shield. The user chooses X or O. X starts. When it is Arduino's turn, the program chooses a free grid cell using `random(0, 3)`.

The code was documented by the upstream author as tested on Arduino UNO R3 as well as newer UNO R4 boards.

## Audit for our MAR2406 project

The architecture is useful as a reference, but the program is **not directly compatible** with our tested MAR2406 2.4-inch shield.

Main differences:

| Item | Upstream | Our verified MAR2406 |
| --- | --- | --- |
| LCD controller | ILI9486 | ILI9341 |
| Resolution/layout | 320 x 480 portrait | canonical 320 x 240 landscape |
| LCD init | hard-coded `tft.begin(0x9486)` | ILI9341 / verified ID 0x9341 |
| Touch pins | XP=D8, XM=A2, YP=A3, YM=D9 | XP=D6, XM=A2, YP=A1, YM=D7 |
| Touch calibration | X 129..902, Y 94..961 | LEFT=153, RIGHT=930, TOP=962, BOTTOM=168 |
| Arduino opponent | random empty cell | our TEST-08 currently two-player |
| microSD history | not used by the game | our TEST-08 appends and restores history |
| replay | reset required after game over | our TEST-08 can start another game |

## Code review notes

1. **Arduino really is the opponent.** The branch `if (turn != player)` performs the computer move.
2. **The opponent is not strategic.** It repeatedly selects random X/Y indices until it finds an empty cell.
3. **The random sequence is not seeded in the source.** On a classic Arduino core this generally makes move sequences repeat after reset unless another source seeds the PRNG.
4. **Game-end behavior is deliberate but simple.** After a win or tie the firmware enters an infinite loop, so another game requires reset.
5. **No SD-card persistence is implemented in this Tic-Tac-Toe source.**
6. **The game logic itself is compact enough for UNO R3.** Its main memory cost comes from the graphics/touch libraries rather than the 3x3 board or opponent logic.
7. **Grid hit-testing uses inclusive cell boundaries.** A touch exactly on a grid line can be associated with the earlier cell; this is acceptable for the original demo but should be avoided in a production HMI.
8. The source is a useful proof that an ATmega328P can drive TFT + resistive touch while also generating Arduino moves. It is not evidence that this exact binary will work on MAR2406 without adaptation.

## Project policy

This folder is kept under `third-party/` so upstream code and licensing remain clearly separated from the original LAB-03 firmware.

Do not silently merge these source files into our own sketches. If ideas or code are adapted, preserve the GPL attribution/license requirements and document the changes.

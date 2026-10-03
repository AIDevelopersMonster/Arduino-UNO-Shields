# Tic-Tac-Toe vs Arduino — MAR2406 adaptation

This is an **independent MAR2406 implementation** of the Player-vs-Arduino idea reviewed in the Dumblebots / Aditya Agarwal ILI9486 example.

No upstream source files are copied into this repository.

## Reference

- Article: https://www.dumblebots.com/blog/using-3-5-tft-lcd-display-ili9486-arduino-part-6-tictactoe-game
- Upstream repository: https://github.com/Aditya-A-garwal/Arduino-TFT-LCD-3-5-Tic-Tac-Toe

## Adapted for our verified hardware

```text
LCD: ILI9341
Orientation: ROT1 / 320x240
Touch: XP=D6 XM=A2 YP=A1 YM=D7
Calibration: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168
```

The original reference targets ILI9486 / 320x480 and different touch wiring, so its source cannot be flashed directly to our MAR2406 shield.

## Game behavior

- choose X or O on the touch screen;
- X always starts;
- Arduino automatically plays the opposite piece;
- Arduino selects a random free cell;
- winner and draw detection;
- occupied cells are rejected;
- touch after GAME OVER returns to the selection menu;
- no microSD is used in this example.

The Arduino move is deliberately random because this example first verifies the hardware adaptation and Player-vs-Arduino architecture. A stronger rule-based or minimax opponent can be added later if desired.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\TicTacToe_vs_Arduino
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\TicTacToe_vs_Arduino
```

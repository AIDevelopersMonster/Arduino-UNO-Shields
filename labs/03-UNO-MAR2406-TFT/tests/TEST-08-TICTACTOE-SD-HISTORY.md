# TEST-08 — Tic-Tac-Toe with persistent microSD history

## Purpose

TEST-08 is the LAB-03 functional demonstrator built on top of the already certified display, touch and microSD stack.

It is not just another synthetic hardware check. The sketch implements a complete two-player Tic-Tac-Toe game and verifies that the MAR2406 shield can be used as a small persistent HMI appliance.

## Unique project features

- full touch-controlled Tic-Tac-Toe game;
- persistent result history on microSD;
- append-only game log;
- history rebuilt after reset or power loss;
- complete move sequence stored for every finished game;
- CRC-8 on every record;
- damaged or truncated records are ignored during history reconstruction;
- no Arduino `String` objects are used;
- fixed-size buffers are used to keep SRAM consumption predictable on the ATmega328P.

## Verified hardware configuration

Display:

```text
ILI9341
ROT1 / 320x240
```

Touch wiring:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified ROT0 calibration:

```text
LEFT   = 153
RIGHT  = 930
TOP    = 962
BOTTOM = 168
```

microSD:

```text
CS   = D10
MOSI = D11
MISO = D12
SCK  = D13
```

## Persistent file

`XOLOG.CSV`

The file is append-only during normal gameplay.

Record format:

```text
G,<game>,<result>,<moves>,<sequence>,<crc8>
```

Example:

```text
G,12,X,5,0-3-1-4-2,6A
```

Where:

- `game` is the persistent game number;
- `result` is `X`, `O`, or `D` for draw;
- `moves` is the number of moves;
- `sequence` stores board cells 0..8 in played order;
- `crc8` protects the record payload.

## Startup behavior

After reset or power-up the firmware:

1. initializes LCD, touch and microSD;
2. creates `XOLOG.CSV` if it does not exist;
3. scans the complete log;
4. validates each record using CRC-8;
5. rebuilds:
   - total games;
   - X wins;
   - O wins;
   - draws;
   - last valid game number;
   - last result;
6. displays the restored statistics before the first game starts.

## Gameplay

- X always starts a new game.
- Each touch selects one of the 9 board cells.
- Touching an occupied cell is ignored.
- The firmware checks all 8 winning lines after each move.
- A finished game is appended to the SD log before the final result screen is shown.
- `PLAY AGAIN` starts a new board without erasing history.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\08_TicTacToe_SD_History
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\08_TicTacToe_SD_History
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

## TEST-08 PASS criteria

The test is considered passed only after all of the following are physically verified:

1. startup statistics are read from microSD and shown on the display;
2. all nine board cells respond to the calibrated touch coordinates;
3. occupied cells cannot be overwritten;
4. X win is detected correctly;
5. O win is detected correctly;
6. draw is detected correctly;
7. a finished game is appended to `XOLOG.CSV`;
8. the saved record contains the complete move sequence;
9. after power cycling the UNO, the previous statistics and last result are restored;
10. a deliberately damaged record is ignored rather than corrupting the reconstructed history.

Passing TEST-08 demonstrates a complete persistent application using LCD + touch + microSD on the tested Arduino UNO + MAR2406 combination.

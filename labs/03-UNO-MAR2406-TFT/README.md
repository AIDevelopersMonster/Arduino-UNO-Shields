# LAB-03 — Arduino UNO + MAR2406 2.4-inch TFT Touch Shield

## Status

**COMPLETE — 2026-10-04**

LAB-03 is closed as a completed hardware/software study for the tested Arduino UNO + MAR2406 shield combination.

The laboratory progressed from controller identification through graphics, resistive touch calibration and microSD access to an integrated LCD + Touch + SD test and a final touch-controlled Tic-Tac-Toe demonstrator with persistent game history.

## Hardware

- Arduino UNO compatible board / ATmega328P
- MAR2406 2.4-inch TFT shield
- 240 x 320 LCD
- ILI9341 controller
- 8-bit parallel display bus
- resistive touch panel
- microSD slot

Shield documentation:

`../../shields/MAR2406-2.4-TFT/`

## Final verified configuration

### Display

```text
Controller: ILI9341
Canonical orientation: ROT1
Logical size: 320 x 240
```

### Resistive touch

Verified wiring for the tested shield sample/batch:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified ROT0 calibration used by the landscape applications:

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

These touch pins and calibration constants are sample/batch-specific. Similar-looking 2.4-inch UNO TFT shields may use different touch wiring or calibration.

### microSD

```text
CS   = D10
MOSI = D11
MISO = D12
SCK  = D13
```

## Final test sequence

| Test | Purpose | Final state |
| --- | --- | --- |
| TEST-01 | LCD controller identification | **PASS** — ILI9341 / 0x9341 confirmed |
| TEST-02 | Direct graphics / RGB565 | **PASS** |
| TEST-03 | Rotation, geometry and text | **PASS** — ROT1 / 320x240 selected |
| TEST-04 | Resistive touch calibration | **PASS** — five-point calibration certified |
| TEST-05 | Touch Paint / landscape HMI | **PASS** |
| TEST-06 | microSD write, reopen and verify | **PASS** |
| TEST-07 | LCD + Touch + microSD integration | **PASS** |
| TEST-08 | Tic-Tac-Toe + persistent SD history | **FINAL DEMONSTRATOR / BUILD PASS** |

TEST procedures are stored in `tests/`. Bench result notes are stored in `results/` where captured.

## TEST-07 integration result

TEST-07 verifies the three principal shield subsystems together:

- ILI9341 display;
- calibrated resistive touch;
- microSD file logging.

The physical test completed with:

```text
LCD PASS
TOUCH PASS
SD LOG PASS
TEST-07 PASS
```

Firmware:

`sketches/07_Display_Touch_SD/07_Display_Touch_SD.ino`

Procedure:

`tests/TEST-07-DISPLAY-TOUCH-SD.md`

## TEST-08 final demonstrator

TEST-08 turns the verified hardware stack into a complete small application:

- two-player X/O Tic-Tac-Toe;
- touch selection of all nine cells;
- win and draw detection;
- occupied-cell protection;
- append-only game history on microSD;
- complete move sequence stored for each game;
- statistics reconstructed after restart;
- fixed-size buffers and no Arduino `String` objects.

History file:

`XOLOG.TXT`

Example records:

```text
X,03142
O,041328
D,041235786
```

Firmware:

`sketches/08_TicTacToe_SD_History/08_TicTacToe_SD_History.ino`

Procedure:

`tests/TEST-08-TICTACTOE-SD-HISTORY.md`

### Arduino UNO build result

Verified with Arduino AVR core 1.8.8:

```text
Flash: 30746 / 32256 bytes (95%)
SRAM globals: 1106 / 2048 bytes (54%)
Linker-reported SRAM remaining: 942 bytes
```

The remaining flash margin is reserved for fixes. TEST-08 is intentionally treated as a feature-complete UNO demonstrator rather than a base for additional large features.

## Build example

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\08_TicTacToe_SD_History
```

Upload example:

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\08_TicTacToe_SD_History
```

## Videos

Introductory LAB-03 hardware video:

https://youtube.com/shorts/TexEwx2-TlQ

Physical five-point touch calibration:

https://youtu.be/0ftVpnH3YtU

## Closure

The tested MAR2406 shield is now documented with a reproducible controller ID, canonical display orientation, working touch pin map/calibration, verified microSD interface, integrated subsystem test and final application demonstrator.

Further work on this shield is not required for LAB-03. New experiments should start as a new laboratory or feature branch rather than extending the closed LAB-03 test series.

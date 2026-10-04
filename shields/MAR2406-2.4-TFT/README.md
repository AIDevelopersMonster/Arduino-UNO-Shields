# MAR2406 2.4-inch TFT LCD Touch Shield

## Identification

Hardware photographed for this project is labelled:

- **SKU:** MAR2406
- **Display:** 2.4-inch TFT
- **Resolution:** 240 x 320
- **LCD driver marked on package:** ILI9341
- **LCD bus:** 8-bit parallel
- **Touch:** resistive touch panel
- **Storage:** microSD slot
- **Form factor:** Arduino UNO shield

The PCB silkscreen exposes the LCD and SD signal names. We will use documented/observed pin mapping first and only use continuity measurements if a test produces an ambiguity.

## Arduino UNO signal map

| Shield signal | UNO pin |
| --- | --- |
| LCD_D0 | D8 |
| LCD_D1 | D9 |
| LCD_D2 | D2 |
| LCD_D3 | D3 |
| LCD_D4 | D4 |
| LCD_D5 | D5 |
| LCD_D6 | D6 |
| LCD_D7 | D7 |
| LCD_RD | A0 |
| LCD_WR | A1 |
| LCD_RS | A2 |
| LCD_CS | A3 |
| LCD_RST | A4 |
| SD_SS | D10 |
| SD_DI / MOSI | D11 |
| SD_DO / MISO | D12 |
| SD_SCK | D13 |

Power pins available on the shield include 5 V, 3.3 V and GND.

## Verification policy

The package identifies the LCD controller as ILI9341. TEST-01 on the physical shield returned `0xD3 -> 00 00 93 41`, so this sample is now **experimentally verified as ILI9341**.

Continuity testing is **not** a prerequisite. It will be used only when software diagnostics leave a specific signal or connection uncertain.

## Related laboratory

See [LAB-03 — Arduino UNO + MAR2406 2.4-inch TFT Touch Shield](../../labs/03-UNO-MAR2406-TFT/).

## Touch revision warning

Do not assume that all visually similar MAR2406 / 2.4-inch UNO TFT shields use the same resistive-touch wiring.

For the batch tested in LAB-03, the working touch assignment is:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Tested calibration values for that sample:

```text
TS_LEFT = 153
TS_RT   = 930
TS_TOP  = 962
TS_BOT  = 168
```

Other batches / revisions may differ in pin sharing and calibration direction/range. Treat these values as verified for the current tested batch, not as universal MAR2406 constants.

## External references

Reference implementation reviewed during the LAB-03 work:

- Dumblebots article — Arduino 3.5-inch ILI9486 Tic-Tac-Toe: https://www.dumblebots.com/blog/using-3-5-tft-lcd-display-ili9486-arduino-part-6-tictactoe-game
- Upstream source repository by Aditya Agarwal: https://github.com/Aditya-A-garwal/Arduino-TFT-LCD-3-5-Tic-Tac-Toe

The external implementation targets different hardware (ILI9486 / 320x480 and different touch wiring), so it is kept only as a reference link and is not copied into this repository.


## Example — Tic-Tac-Toe vs Arduino

Adapted MAR2406 version of the Player-vs-Arduino Tic-Tac-Toe concept:

`examples/TicTacToe_vs_Arduino/TicTacToe_vs_Arduino.ino`

The implementation is original for this repository and uses the verified MAR2406 ILI9341 / 320x240 / D6-A2-A1-D7 touch configuration. The external Dumblebots / Aditya Agarwal project remains linked only as a reference.


## Example — Boolean Algebra Interactive Learning HMI

Physically verified on the tested Arduino UNO + MAR2406 shield:

`examples/Boolean_Algebra_Interactive_HMI/Boolean_Algebra_Interactive_HMI.ino`

Features:

- LEARN and QUIZ modes;
- interactive A/B input toggles;
- NOT, AND, OR, XOR, NAND, NOR and XNOR;
- live result calculation;
- truth tables with active-row highlighting;
- touch-controlled lesson navigation;
- 0/1 quiz answers with immediate correct/wrong feedback.

Status: **PASS on physical hardware**.

This is a complete interactive educational HMI running directly on the ATmega328P with the verified ILI9341 / ROT1 320x240 display and calibrated resistive touch configuration.

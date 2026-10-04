# Boolean Algebra Interactive Learning HMI

Interactive educational HMI for **Arduino UNO / ATmega328P + MAR2406 2.4-inch TFT Touch Shield**.

**Physical bench status: PASS**

The sketch was physically tested on the project hardware and confirmed working.

## Hardware

```text
Arduino UNO / ATmega328P
MAR2406 2.4" TFT Touch Shield
LCD: ILI9341
Orientation: ROT1 / 320x240
Touch: resistive
```

Verified touch wiring:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified calibration:

```text
TS_LEFT = 153
TS_RT   = 930
TS_TOP  = 962
TS_BOT  = 168
```

## Features

- interactive **LEARN** mode;
- interactive **QUIZ** mode;
- touch-controlled A and B inputs;
- live Boolean result;
- truth tables;
- active-row highlighting;
- operations:
  - NOT A
  - AND
  - OR
  - XOR
  - NAND
  - NOR
  - XNOR
- PREV / NEXT lesson navigation;
- 0 / 1 answer buttons;
- CORRECT / TRY AGAIN feedback.

This is a real interactive educational HMI running directly on the ATmega328P rather than a static TFT graphics demonstration.

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Boolean_Algebra_Interactive_HMI
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Boolean_Algebra_Interactive_HMI
```

## Libraries

- Adafruit GFX Library
- MCUFRIEND_kbv
- Adafruit TouchScreen

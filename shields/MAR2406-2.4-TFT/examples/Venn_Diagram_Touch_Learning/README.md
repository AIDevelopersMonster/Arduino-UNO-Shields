# Venn Diagram Touch Learning

Interactive educational demonstration for the verified Arduino UNO + MAR2406 2.4-inch TFT Touch Shield.

## Purpose

The example turns the shield into a small touch learning device for basic set theory using two intersecting circles.

### Learn mode

The `NEXT` button cycles through:

- Set A
- Set B
- A AND B — intersection
- A OR B — union
- A - B
- B - A
- A XOR B — symmetric difference

The selected area is filled on the Venn diagram. Touching any diagram region identifies whether the point belongs to:

- A only
- A and B
- B only
- outside both sets

### Quiz mode

The display asks the student to touch one of three regions:

- A ONLY
- A AND B
- B ONLY

Correct and incorrect answers are counted on screen.

## Verified hardware

```text
LCD: ILI9341
Orientation: ROT1 / 320x240
Touch: XP=D6 XM=A2 YP=A1 YM=D7
Calibration: LEFT=153 RIGHT=930 TOP=962 BOTTOM=168
```

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Venn_Diagram_Touch_Learning
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\shields\MAR2406-2.4-TFT\examples\Venn_Diagram_Touch_Learning
```

The demo uses only TFT + Touch and does not require microSD.

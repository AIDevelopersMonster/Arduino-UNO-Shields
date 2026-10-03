# TEST-02 — ILI9341 graphics smoke test

## Prerequisite

TEST-01 passed on the physical shield:

`0xD3 -> 00 00 93 41`

Controller: **ILI9341 confirmed**.

## Purpose

Initialize the verified ILI9341 directly over the MAR2406 8-bit parallel bus and prove visible pixel writes without adding an external graphics library.

Firmware:

`../sketches/02_ILI9341_Graphics/02_ILI9341_Graphics.ino`

## Build and upload

From the repository root:

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\02_ILI9341_Graphics
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\02_ILI9341_Graphics
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Expected visible sequence

The entire LCD should show, in order:

1. red
2. green
3. blue
4. white
5. black

The final stable image is six vertical bars:

**red | green | blue | white | yellow | magenta**

Serial output should end with:

```text
COLOR BARS
TEST-02 visual result now requires operator observation.
```

## PASS criteria

Record **PASS** only if:

- the display leaves the uninitialized state;
- all five full-screen fills are visible;
- the final six bars occupy the screen;
- colors are distinct and in the expected order;
- there is no gross address-window corruption.

Minor orientation questions are not a TEST-02 failure; rotation will be handled in the following graphics/orientation stage.

If the display stays blank or garbled, do not start broad continuity testing. First record exactly what appeared on screen and the serial output.

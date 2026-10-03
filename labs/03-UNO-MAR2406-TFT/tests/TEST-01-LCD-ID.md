# TEST-01 — LCD controller ID probe

## Purpose

Identify the LCD controller on the physical MAR2406 shield before loading a graphics library or treating the package marking as bench-verified evidence.

Firmware:

`../sketches/01_LCD_ID_Probe/01_LCD_ID_Probe.ino`

The probe talks to the shield directly over the documented 8-bit parallel bus. It has **no external library dependency**.

## Hardware setup

1. Power off / disconnect USB.
2. Fit the MAR2406 shield fully onto the Arduino UNO headers.
3. microSD is not required for this test.
4. Connect the UNO by USB.
5. Do not press or calibrate the touch panel yet.

The sketch does not initialize the graphics controller. A white, black or unchanged LCD image is therefore **not a failure** in TEST-01.

## Arduino CLI

From the repository root:

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\01_LCD_ID_Probe
arduino-cli board list
```

Upload after substituting the actual UNO port for `COMx`:

```powershell
arduino-cli upload -p COMx --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\01_LCD_ID_Probe
arduino-cli monitor -p COMx -c baudrate=115200
```

## What to capture

The serial monitor prints raw responses for:

- `0xD3` — ID4
- `0x04` — display identification
- `0x09` — display status
- `0xDA`, `0xDB`, `0xDC` — ID bytes where supported

For a normal ILI9341, the important evidence is an adjacent `93 41` signature in the raw `0xD3` response. A common capture is:

```text
D3 ID4  [0xD3] : 00 00 93 41
```

Do not infer a failed display solely from `00` or `FF`. A stuck response can also indicate bus-direction, timing, level-shifter or connection behavior.

## Interactive commands

While the serial monitor is open:

- `r` — repeat all register reads;
- `x` — hardware-reset the LCD and repeat all register reads.

## Result record

Copy the complete serial output into a result file before changing firmware.

Suggested filename:

`../results/TEST-01-LCD-ID.txt`

Classification:

- **PASS / ILI9341** — stable `0x9341` signature observed;
- **IDENTIFIED / OTHER** — stable response identifies another controller;
- **INCONCLUSIVE** — no reliable controller ID yet.

Continuity measurements are deferred unless this software result gives us a specific line or connection to investigate.

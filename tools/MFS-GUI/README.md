# MFS-GUI

Video-friendly desktop GUI for
[LAB-02 — Arduino UNO + Multi-Function Shield](../../labs/02-UNO-MultiFunction-Shield/).

The GUI is intentionally thin. Hardware logic remains in the Arduino firmware
and all control uses the documented ASCII serial protocol.

For the physical LAB-02 shield, buzzer frequency control is intentionally not
exposed because PWM/tone drive sharply reduces loudness; the buzzer is used as
active ON/OFF only.

## Requirements

- Windows 10 or another desktop OS with Python 3
- Tkinter (included with normal Windows Python installations)
- pyserial 3.x
- LAB-02 firmware uploaded to Arduino UNO

Install the one Python dependency:

```powershell
py -m pip install -r .\tools\MFS-GUI\requirements.txt
```

Run:

```powershell
py .\tools\MFS-GUI\mfs_gui.py
```

Or:

```powershell
.\tools\MFS-GUI\run_gui.ps1
```

## Tabs

- OVERVIEW — all live states on one screen
- LEDs — manual D1-D4 control
- BUTTONS — live state plus press counters
- POT A0 — raw ADC, percentage and observed min/max
- DISPLAY — direct values and digit-select clone diagnostic
- BUZZER — active buzzer ON/OFF and short beep
- SYSTEM — protocol log and raw command entry

## First connection

1. Close Arduino Serial Monitor so it releases the COM port.
2. Start the GUI.
3. Refresh ports.
4. Select the UNO COM port.
5. Connect.
6. Wait for the UNO reset/boot.
7. The GUI sends `INFO` after the connection settles.

Continuous `@STATE` telemetry is not printed in the log by default, keeping the
SYSTEM tab readable during video capture.

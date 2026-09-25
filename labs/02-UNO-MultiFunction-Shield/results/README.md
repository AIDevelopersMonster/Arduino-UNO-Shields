# LAB-02 results

## Stage A status

**PASS — first laboratory configuration accepted on the physical shield.**

Bench-confirmed in the current configuration:

| Item | Result | Evidence / notes |
| --- | --- | --- |
| Serial 115200 | PASS | Stable firmware/GUI communication. |
| D1 / D13 | PASS | Individual LED control works. |
| D2 / D12 | PASS | Individual LED control works. |
| D3 / D11 | PASS | Individual LED control works. |
| D4 / D10 | PASS | Individual LED control works. |
| S1 / A1 | PASS | Live state and press counting work. |
| S2 / A2 | PASS | Live state and press counting work. |
| S3 / A3 | PASS | Live state and press counting work. |
| VR / A0 | PASS | Live analog value follows the potentiometer. Exact min/max may be recorded separately if needed. |
| 4-digit display | PASS | Display control works in the accepted configuration. |
| Buzzer ACTIVE mode | PASS | D3 HIGH gives strong sustained sound; D3 LOW is quiet. |
| Buzzer ACTIVE/PASSIVE selector | PASS | Modes are separated and mutually exclusive in firmware and GUI. |
| PASSIVE mode hardware | NOT INSTALLED | Supported for a future passive replacement/variant, but not physically certified on the current active buzzer. |
| Full stimulus sequence | PASS | Accepted for the first laboratory configuration. |

## Accepted buzzer behavior

The installed buzzer is used in **ACTIVE** mode by default.

- `CFG,BUZZER,ACTIVE`
- `BUZ,ON`
- `BUZ,OFF`
- `BEEP,<milliseconds>`

PASSIVE mode is retained as a separate non-overlapping path for a future
replacement part:

- `CFG,BUZZER,PASSIVE`
- `TONE,<Hz>`
- `TONE,OFF`

This keeps the laboratory universal without degrading the current active-buzzer
behavior.

# APP-01 — 4-Relay Serial Controller

> **Status: BENCH PASS / FROZEN AUXILIARY PROFILE (2026-10-11).** R1-R4 on D4-D7, ACTIVE HIGH and USB/COM 115200 control were physically accepted for the tested relay module. No active feature roadmap is maintained.

Arduino UNO + four relay inputs on D4-D7. USB/COM only. No W5100, no microSD, no network.

## Pins

| UNO | Relay |
| --- | --- |
| D4 | R1 |
| D5 | R2 |
| D6 | R3 |
| D7 | R4 |

Bench polarity: **ACTIVE HIGH**.

## Serial protocol

115200 8N1.

Commands:

```text
PING
STATUS
R1 ON
R1 OFF
R1 TOGGLE
R2 ON
R3 OFF
R4 TOGGLE
ALL ON
ALL OFF
HELP
```

Typical replies:

```text
PONG
OK R1 ON
STATE R1=ON R2=OFF R3=OFF R4=OFF
```

## Build and upload

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin

git restore --source=origin/main --worktree -- `
  labs/06-KON-Relay-Control/APP-01_4Relay_Serial_Controller/

$sketch = ".\labs\06-KON-Relay-Control\APP-01_4Relay_Serial_Controller"

arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
```

## Terminal CLI

Do not keep Arduino monitor open while the PowerShell CLI or GUI is using COM4.

```powershell
cd C:\GitHub\Arduino-UNO-Shields\labs\06-KON-Relay-Control\APP-01_4Relay_Serial_Controller
.\Relay-Serial-CLI.ps1 -Port COM4
```

## Windows GUI

```powershell
.\Relay-Serial-GUI.ps1 -Port COM4
```

or double-click:

```text
Start-Relay-Serial-GUI.cmd
```

The GUI has Connect/Disconnect, STATUS, ON/OFF/TOGGLE for R1-R4, and ALL ON/OFF.

Status: **BENCH PASS for the bounded USB/COM four-channel control profile**. This does not certify relay contact ratings, mains switching, reset transients, Modbus or long-term reliability.

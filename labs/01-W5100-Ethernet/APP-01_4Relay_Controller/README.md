# APP-01 — W5100 4-Relay Controller

Applied Ethernet-only controller for Arduino UNO + W5100 shield.

## Hardware

| UNO pin | Function |
| --- | --- |
| D4 | Relay 1 |
| D5 | Relay 2 |
| D6 | Relay 3 |
| D7 | Relay 4 |
| D10 | W5100 CS |
| D11 | SPI MOSI |
| D12 | SPI MISO |
| D13 | SPI SCK |

The microSD card must be physically removed because D4 is reused as Relay 1.

Bench result: this relay module uses **ACTIVE HIGH** inputs:

```cpp
const bool RELAY_ACTIVE_LOW = false;
```

This value is now fixed to `false` for the tested module.

Arduino GPIO pins must drive only the relay module logic inputs. Do not power relay coils directly from D4-D7.

## Control methods

### Web UI

After DHCP the Serial monitor prints the current address:

```text
IP: 192.168.1.xxx
WEB: http://192.168.1.xxx/
```

The browser dashboard controls Relay 1-4 with ON, OFF and TOGGLE buttons plus ALL ON / ALL OFF.

### Serial CLI

115200 baud commands:

```text
HELP
STATUS
R1 ON
R1 OFF
R1 TOGGLE
R2 ON
R3 OFF
R4 TOGGLE
ALL ON
ALL OFF
```

The Serial CLI remains available even if DHCP fails.

### HTTP API

| GET | Function |
| --- | --- |
| `/api/status` | JSON state |
| `/api/r1/on` ... `/api/r4/on` | relay ON |
| `/api/r1/off` ... `/api/r4/off` | relay OFF |
| `/api/r1/toggle` ... `/api/r4/toggle` | toggle |
| `/api/all/on` | all ON |
| `/api/all/off` | all OFF |

### PowerShell CLI

```powershell
.\Relay-CLI.ps1 -IP 192.168.1.76 -Action status
.\Relay-CLI.ps1 -IP 192.168.1.76 -Action on -Channel 1
.\Relay-CLI.ps1 -IP 192.168.1.76 -Action toggle -Channel 4
.\Relay-CLI.ps1 -IP 192.168.1.76 -Action all-off
```

### Windows GUI

```powershell
.\Relay-GUI.ps1 -IP 192.168.1.76
```

or double-click:

```text
Start-Relay-GUI.cmd
```

The GUI exposes ON / OFF / TOGGLE for every channel and ALL ON / ALL OFF.

## Build and upload

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin

git restore --source=origin/feature/w5100-4relay-controller --worktree -- `
  labs/01-W5100-Ethernet/APP-01_4Relay_Controller/

$sketch = ".\labs\01-W5100-Ethernet\APP-01_4Relay_Controller"

arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Host-side software test

After reading the current DHCP IP, open a second PowerShell:

```powershell
cd C:\GitHub\Arduino-UNO-Shields\labs\01-W5100-Ethernet\APP-01_4Relay_Controller
.\Test-Relay-Api.ps1 -IP <actual-IP>
```

The script cycles each software channel ON/OFF and finally ALL ON/OFF while checking JSON state.

## Acceptance

Software PASS requires consistent state across Web UI, Serial CLI, PowerShell CLI and GUI.

Hardware PASS additionally requires physical confirmation that all four relay channels respond correctly.

Status: **PENDING bench test**.

## Security

This is a trusted-LAN controller only. It has no authentication, TLS or authorization. Do not expose port 80 to the public Internet.

# TEST-04 — Ethernet Web Control | PENDING

**Purpose:** turn the verified HTTP server into a **local-network demonstration controller** for Arduino UNO D6/D7; return live software state, uptime and request counter. Firmware published; physical bench validation has not yet been submitted.

## Hardware and safety

- UNO + blue W5100 shield, RJ45 to DHCP LAN, USB to PC; microSD **removed**.
- W5100 CS=D10, microSD CS=D4 (HIGH). D6 and D7 are OUTPUT **LOW on boot**.
- Optional LED testing must use a 330–1000 ohm series resistor, or measure with a multimeter.
- **No relay, mains load, or internet port forwarding.** This demo has no authentication, TLS, CSRF protections, watchdog or certified safe shutdown.

## API

| Request | Effect / response |
| --- | --- |
| `GET /` | HTML dashboard with D6/D7 buttons, IP, uptime and request counter |
| `GET /api` | JSON: `d6`, `d7`, `uptime_s`, `requests` |
| `GET /d6/on` / `/d6/off` | Set D6 HIGH/LOW; redirect to `/` |
| `GET /d7/on` / `/d7/off` | Set D7 HIGH/LOW; redirect to `/` |

## Arduino CLI

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-04_Web_Control"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

In a second PowerShell window (after reading the **current** DHCP IP):

```powershell
$ip = "192.168.1.80" # replace if DHCP changed
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d6/on" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d6/off" -UseBasicParsing | Out-Null
Invoke-WebRequest "http://$ip/d7/on" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d7/off" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
```

## Acceptance, evidence and non-claims

1. Firmware compiles/uploads; DHCP starts server and prints actual IP.
2. Browser dashboard and JSON endpoint answer; D6/D7 ON/OFF states update correctly.
3. Serial counter increments on HTTP requests; reload doesn't lose state.
4. **Independent electrical proof:** multimeter or current-limited LEDs confirm voltage/output response on physical D6/D7.

**Status remains PENDING until hardware output is supplied.** JSON state alone is a software-level PASS, not proof of pin voltage. Record results in a dated `RESULT_YYYY-MM-DD.md` file. [Full laboratory plan](../README.md).

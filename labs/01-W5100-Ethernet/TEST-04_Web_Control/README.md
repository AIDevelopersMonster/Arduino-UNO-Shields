# TEST-04 — Ethernet Web Control

**Status:** source published, hardware result pending.

Arduino UNO ATmega328P + W5100 Ethernet Shield. D6 and D7 are digital outputs, LOW on startup. W5100 CS D10, SD CS D4 held HIGH; microSD card absent. RJ45 goes to a DHCP LAN router. Network is trusted/isolated: no authentication or TLS. Never port forward this experimental controller.

Web routes: `/` (HTML panel), `/api` (JSON showing d6/d7, uptime_s, requests), `/d6/on`, `/d6/off`, `/d7/on`, `/d7/off` (HTTP 303 redirect to dashboard).

## PowerShell CLI

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-04_Web_Control"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Obtain the actual IP from Serial (DHCP may reassign it). In another PowerShell window, **replace the example IP**:

```powershell
$ip = "192.168.1.80"
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d6/on" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d6/off" -UseBasicParsing | Out-Null
Invoke-WebRequest "http://$ip/d7/on" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
Invoke-WebRequest "http://$ip/d7/off" -UseBasicParsing | Out-Null
Invoke-RestMethod "http://$ip/api"
```

Open `http://<actual IP>/` in browser. Test ON/OFF and refresh. Software state is not independent proof of physical GPIO voltage. For physical testing, use multimeter or properly current-limited LEDs (330–1000 ohm in series) and ground, without relays or mains loads.

**PASS criteria:** compile/upload succeed, DHCP lease, dashboard HTTP 200, JSON and both pins' states respond to ON/OFF, serial request counter advances. Physical GPIO PASS requires measured voltage or observed LEDs. No SD is used.

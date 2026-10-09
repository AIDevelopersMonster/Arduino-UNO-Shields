# TEST-03 — HTTP Web Server | FULL PASS

**Purpose:** serve HTML from ATmega328P over W5100 TCP port 80 and answer a text health endpoint; no microSD.

## Procedure

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-03_HTTP_Server"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Use the **actual** DHCP address printed by firmware, not an assumed fixed IP. On the same LAN open `http://<actual-IP>/` in Chrome and then use:

```powershell
$ip = "192.168.1.80" # historical bench IP; replace with current lease
Invoke-WebRequest "http://$ip/health" -UseBasicParsing
Invoke-WebRequest "http://$ip/" -UseBasicParsing
```

Observe HTTP 200, served HTML, increasing request counter and uptime. Avoid exposing this plain HTTP laboratory server to the internet.

## Bench result

**FULL PASS (2026-10-09):** DHCP `192.168.1.80`; HTTP **200 OK** for `/health` and `/`, browser-rendered HTML, sequential responses; observed counter up to **19** and uptime **375 seconds**. [Evidence](RESULT_2026-10-09.md).

**Non-claims:** no authentication, TLS, prolonged stress test, or SD service. [Full laboratory plan](../README.md).

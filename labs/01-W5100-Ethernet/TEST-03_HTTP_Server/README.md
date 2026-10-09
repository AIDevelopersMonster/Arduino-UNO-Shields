# TEST-03 — HTTP server on W5100 without SD

**Status: awaiting user bench test.** TEST-01 SPI PASS; TEST-02 DHCP and Ping PASS (4/4, 0% loss), recorded separately.

## Hardware

Arduino UNO + blue W5100 Ethernet Shield. Keep SD card removed. Connect RJ45 to same DHCP-enabled LAN as Windows PC. Shield W5100 CS=D10; SD CS=D4 deselected.

## Windows PowerShell

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-03_HTTP_Server"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

If compilation reports missing Ethernet library: `arduino-cli lib install Ethernet`.

Read the **actual DHCP IP** from Serial (may differ from prior test's 192.168.1.76). From another PowerShell window, replacing address accordingly:

```powershell
Invoke-WebRequest -Uri "http://192.168.1.76/health" -UseBasicParsing
Invoke-WebRequest -Uri "http://192.168.1.76/" -UseBasicParsing
```

Or open `http://<actual Arduino IP>/` in browser. PowerShell ping alone does not certify HTTP.

## PASS criteria

- DHCP assigned IPv4.
- Browser or Invoke-WebRequest gets HTTP 200 for `/` and `/health`.
- Serial shows HTTP GET request counts.
- Reload page twice; uptime and counter should progress.
- Record complete Serial and response outputs. HTTP success is not yet a long-term reliability certification.

No SD use, TLS, authentication, external Internet access, or persistent storage; keep server on a trusted LAN only.

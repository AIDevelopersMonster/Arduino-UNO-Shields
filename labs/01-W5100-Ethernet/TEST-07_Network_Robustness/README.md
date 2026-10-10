# TEST-07 — Ethernet Network Robustness

**Status: SOURCE READY / hardware test PENDING.** Arduino UNO + blue W5100, SD absent.

## Purpose

Verify normal LAN reachability, intentional cable outage, subsequent recovery and a short bounded soak. Arduino serves binary UDP Echo on port **5002**, logs uptime/IP/counters every 10 s, and calls Ethernet.maintain() for DHCP. Classic W5100 may not expose reliable linkStatus(), so rely on real traffic and physical LINK LED. A short test cannot certify actual DHCP lease renewal unless renewal happens during observation.

| Phase | Action | Acceptance |
| --- | --- | --- |
| A | Cable connected: 10 probes | 10/10 correct replies |
| B | RJ45 disconnected: 10 probes | At least 8 timeouts/failures, expected |
| C | RJ45 reconnected: 30 probes | Last 10/10 correct replies |
| D | Bounded soak: 60 seconds by default | At least 95% replies |

UDP has no guaranteed delivery. The PowerShell script uses binary sequence-identified probes without automatic retry. The cable is removed from **Arduino Ethernet RJ45 only**. Keep USB power connected throughout. If the DHCP IP changes, enter its new value at the script prompt.

## CLI — first PowerShell window

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = '.\labs\01-W5100-Ethernet\TEST-07_Network_Robustness\TEST-07_Cable_Recovery'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

## Host test — second PowerShell 7 window

```powershell
cd C:\GitHub\Arduino-UNO-Shields
$ip = '192.168.1.84' # EXAMPLE from TEST-06; replace with NEW DHCP address
& .\labs\01-W5100-Ethernet\TEST-07_Network_Robustness\Test-NetworkRobustness.ps1 -IP $ip -SoakSeconds 60
```

Follow prompts to remove and reconnect RJ45. Send both complete PowerShell summary and Arduino Serial monitor logs. Record observations as RESULT_YYYY-MM-DD.md only after hardware test.

## Files

- [Arduino sketch](TEST-07_Cable_Recovery/TEST-07_Cable_Recovery.ino)
- [PowerShell host tester](Test-NetworkRobustness.ps1)

Router settings, DHCP configuration and firmware remain untouched. Only the Ethernet cable attached to the Arduino shield is unplugged and replugged.

## Scope

Trusted LAN only, UDP unauthenticated. A PASS proves only these bounded phases; it does not certify prolonged service, automatic restoration in every network, physical PHY link detection, or lease renewal if no renewal event occurred.
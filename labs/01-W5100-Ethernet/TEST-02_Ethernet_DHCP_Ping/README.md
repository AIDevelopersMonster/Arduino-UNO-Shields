# TEST-02 — Ethernet DHCP + Ping | PASS

**Purpose:** prove that W5100 on the verified SPI bus obtains an IPv4 address and answers ICMP Echo from a PC. **No microSD.**

## Connection

USB Arduino UNO; RJ45 W5100 shield to trusted DHCP router/switch; Windows PC on same LAN. W5100 CS=D10, SD CS=D4 HIGH. Classic W5100's library `linkStatus()` can remain `UNKNOWN`: this is not, by itself, failure.

## PowerShell

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-02_Ethernet_DHCP_Ping"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Once the DHCP IP is printed, run in another PowerShell window: `ping -n 4 <actual-IP>` (replace placeholder; do not type `ping` into Serial Monitor).

## Bench result

**PASS (2026-10-09):** W5100 detected, DHCP IP `192.168.1.76`, netmask `255.255.255.0`, gateway/DNS `192.168.1.254`; PC Ping replied **4/4**, **0% loss**, **0–1 ms**. [Evidence](RESULT_2026-10-09.md).

**Non-claims:** W5100 library did not report LINK state; successful network traffic proves operational connectivity in this trial, not soak stability. [Full laboratory plan](../README.md).

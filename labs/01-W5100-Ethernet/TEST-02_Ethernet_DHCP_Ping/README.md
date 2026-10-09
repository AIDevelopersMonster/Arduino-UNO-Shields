# TEST-02 — Ethernet LINK + DHCP IP + Ping (no SD)

**Status:** AWAITING HARDWARE TEST. TEST-01 SPI register read/write already PASS on this blue W5100 shield.

## Connection

- Arduino UNO + W5100 shield; **remove microSD**.
- Connect RJ45 to a LAN port of a router/switch on a network with **DHCP enabled**.
- PC should be on the same local network. USB remains connected for Serial.
- W5100 CS D10, SD CS D4 held HIGH.
- For classic W5100, Ethernet library's `linkStatus()` may return **UNKNOWN**; use physical LINK LED and real ping instead. This is not automatically a failure.

## PowerShell (from repository root)

```powershell
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-02_Ethernet_DHCP_Ping"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Requires Arduino Ethernet library (install if missing with `arduino-cli lib install Ethernet`). After recording the leased IP, stop monitor with Ctrl+C and run `ping -n 4 <IP>` in PowerShell. Replace <IP> with the exact printed IPv4 address, e.g. `ping -n 4 192.168.1.123` only if that is your real address.

## Acceptance

- Hardware: W5100 recognized.
- LINK: LED indication observed, preferably confirmed by ping.
- DHCP: PASS and nonzero local IP shown.
- PING: packets received from PC, no loss for this first controlled experiment.

DHCP PASS does **not** itself certify Ping; send both the complete Serial log and PC ping output. ICMP echo handling is part of W5100 TCP/IP hardware and does not require an application server.

If DHCP FAIL, do not assign an arbitrary static address: record LEDs and network topology first.

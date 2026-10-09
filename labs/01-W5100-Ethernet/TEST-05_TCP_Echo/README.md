# TEST-05 — TCP Client / Server: echo and reconnection

**Status: PENDING physical test.** Source published, not yet hardware-certified.

## Test scope

An Arduino UNO + W5100 operates as a **TCP server on port 5000**.
A Windows PowerShell script acts as a TCP **client** and checks:
- 20 independent sessions with exact ASCII echo;
- a 64-byte payload (maximum accepted);
- connection closure and repeat connection;
- an incomplete transmission followed by a fresh successful connection.

Protocol: the client sends one ASCII line terminated with LF. The Arduino returns `ECHO <payload>\n` and closes. Maximum length 64 bytes, 1500 ms read deadline. No microSD, no HTTP.

This test does **not** include Arduino acting as an outbound TCP client (a separate future test may address that). It does not certify security, throughput, concurrent client service, link-flap recovery, or long soak endurance.

## Wiring and security

Use the blue W5100 shield on UNO, W5100 CS D10, SD CS D4 held HIGH, RJ45 to DHCP-enabled trusted LAN and USB to Windows COM4. Remove microSD card. Do **not** expose port 5000 through router port-forwarding or to the Internet.

## CLI

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-05_TCP_Echo"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Read the **current DHCP IP** from Serial output (previous addresses such as 192.168.1.81 are not guaranteed). In a **second PowerShell window**:

```powershell
Test-NetConnection -ComputerName 192.168.1.81 -Port 5000
pwsh -NoProfile -File .\labs\01-W5100-Ethernet\TEST-05_TCP_Echo\Test-TcpEcho.ps1 -IP 192.168.1.81 -Count 20
```

Replace `192.168.1.81` with actual printed DHCP IP. If already running PowerShell 7 from repository root, you may use:

```powershell
& .\labs\01-W5100-Ethernet\TEST-05_TCP_Echo\Test-TcpEcho.ps1 -IP 192.168.1.81 -Count 20
```

## Acceptance criteria

**PASS** if script prints `RESULT: PASS`, 20 normal requests plus boundary and reconnect checks succeed, and the UART log contains `TCP CONNECT`, `TCP ECHO`, `TCP CLOSE`. On the deliberately aborted session an `incomplete` error is expected and must not prevent the next connection.

Record both host and UART logs in a `RESULT_YYYY-MM-DD.md` file; do not mark as PASS before user hardware evidence. The initial 64-byte test is application payload, not Ethernet MTU/bandwidth.

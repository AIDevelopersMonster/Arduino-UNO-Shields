# TEST-06 — Binary UDP Communication

**Status: PENDING hardware test.** Arduino UNO + blue W5100 Shield; no microSD.

UDP port **5001** receives binary payloads of 1–128 bytes and sends exact bytes back to the sender. No TCP connection, no HTTP. UDP does not guarantee delivery or ordering.

## Test matrix

| Case | Payload | Acceptance |
| --- | --- | --- |
| Boundaries | 1, 16, 64, 128 bytes | Exact binary Echo |
| Sequence | 20 distinct 32-byte datagrams | Exact Echo, zero observed timeouts |

The PowerShell client checks UDP source endpoint, length and every byte; it never retries failed packets. Datagrams over 128 bytes are deliberately dropped but not tested in this first acceptance series.

## Wiring

W5100 CS D10; SD CS D4 HIGH; SPI via UNO ICSP; RJ45 to DHCP LAN router; Serial 115200 on COM4. Remove microSD. Trusted LAN only, no port forwarding.

## Arduino CLI — PowerShell

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = '.\labs\01-W5100-Ethernet\TEST-06_UDP_Communication'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Find current DHCP IP in Serial. In a **second** PowerShell window, from repository root:

```powershell
$ip = '192.168.1.83' # EXAMPLE ONLY; replace with new address
& .\labs\01-W5100-Ethernet\TEST-06_UDP_Communication\Test-UdpEcho.ps1 -IP $ip -Count 20
```

Expected final result: `RESULT: PASS / sent=24 verified=24 failures=0 timeouts=0`. This is a target, not an observed result. Share complete host and Arduino logs before certification.

Refer to [LAB-01B programme](../README.md).
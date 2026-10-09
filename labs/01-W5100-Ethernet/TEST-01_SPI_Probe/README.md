# TEST-01 — Raw SPI Register Probe | PASS

**Purpose:** verify low-level W5100 SPI register read/write and restoration without Ethernet library, router, Ethernet cable or microSD.

**Hardware:** Arduino UNO, blue W5100 shield; CS D10, SD CS D4 HIGH; SPI 1 MHz Mode 0; Serial 115200.

## Procedure

From repo root in PowerShell:

```powershell
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-01_SPI_Probe"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Sketch reads Mode Register (MR) and Retry Time Register (RTR), stores original RTR, writes `0x1234`, checks it, then restores and verifies original RTR.

## Bench result

**PASS (2026-10-09):** MR `0x00`, original RTR `0x07D0`, test `0x1234`, restore OK. [Verbatim evidence and limitations](RESULT_2026-10-09.md).

**Non-claims:** SPI register access does not prove Ethernet PHY, DHCP, Ping, HTTP or SD. [Full laboratory plan](../README.md).

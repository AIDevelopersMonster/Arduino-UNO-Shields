# LAB-01B — Arduino UNO + W5100 Ethernet Shield (blue board, no SD)

## Repository status

**FROZEN / MAINTENANCE-ONLY (2026-10-11).** The bounded Ethernet results are retained for reproducibility and occasional auxiliary use. There is no active W5100 development roadmap in this repository. The controller is treated as legacy hardware; new work is justified only by a concrete auxiliary need.

> **Scope and naming.** This directory is the independently tested **blue W5100 Ethernet Shield** sequence. Ethernet-only operation is the accepted project scope. The onboard microSD/shared-SPI path was investigated through TEST-12 and rejected as unreliable on the tested clone samples. Evidence and PASS reports apply to the **specific tested samples**, not all W5100 shields.

## Hardware and wiring

- Arduino UNO / ATmega328P, blue WIZnet W5100 Ethernet Shield, HanRun RJ45, USB to Windows PC, LAN to DHCP-enabled router for TEST-02 onward.
- W5100 chip select **D10**, microSD CS **D4** held HIGH, SPI via ICSP = UNO D11 (MOSI), D12 (MISO), D13 (SCK).
- TEST-04: GPIO output D6 and D7, LOW on startup. No external mains load. If testing LEDs, use series resistors; a multimeter provides direct voltage evidence.
- Serial: **115200 baud**, Windows PowerShell and Arduino CLI. Replace COM4 and DHCP addresses with real current values.
- W5100 Ethernet library `linkStatus()` may report `UNKNOWN` even when DHCP and ICMP work; do not count this alone as failure.

## Evidence-driven test programme

| Test | Objective and acceptance | Actual observed result | Evidence |
| --- | --- | --- | --- |
| [TEST-01 SPI Probe](TEST-01_SPI_Probe/) | Read MR; write/read RTR=0x1234; restore original RTR | **PASS** — MR=0x00, original RTR=0x07D0, write/read and restore OK | [Result](TEST-01_SPI_Probe/RESULT_2026-10-09.md) |
| [TEST-02 DHCP + Ping](TEST-02_Ethernet_DHCP_Ping/) | W5100 detection, DHCP IPv4, PC ICMP echo with replies | **PASS** — DHCP 192.168.1.76; Ping 4/4, 0% loss, 0–1ms | [Result](TEST-02_Ethernet_DHCP_Ping/RESULT_2026-10-09.md) |
| [TEST-03 HTTP Server](TEST-03_HTTP_Server/) | HTTP 200 on /health and /; HTML displayed; repeated requests | **FULL PASS** — DHCP 192.168.1.80; both HTTP endpoints 200; browser HTML; 19 requests, uptime 375s observed | [Result](TEST-03_HTTP_Server/RESULT_2026-10-09.md) |
| [TEST-04 Web Control](TEST-04_Web_Control/) | Browser commands for D6/D7; independent physical output verification | **SOFTWARE PASS** — browser, 10 request routes; physical GPIO still pending | [Evidence](TEST-04_Web_Control/RESULT_2026-10-09.md) |
| [TEST-05 TCP Echo](TEST-05_TCP_Echo/) | 20 independent TCP exchanges, 64-byte boundary, reconnect after abort | **FULL PASS** — 22 exact echoes, 23 connections, expected aborted request | [Result](TEST-05_TCP_Echo/RESULT_2026-10-09.md) |
| [TEST-06 UDP Echo](TEST-06_UDP_Communication/) | Binary echo 1–128 bytes, 20 sequenced packets, host byte verification | **FULL PASS** — 24/24 binary echoes, 0 timeouts | [Result](TEST-06_UDP_Communication/RESULT_2026-10-09.md) |
| [TEST-07 RJ45 Recovery](TEST-07_Network_Robustness/) | LAN baseline, Arduino RJ45 unplug/replug, short soak | **FULL PASS** — 100/100 connected Echo responses, 10/10 expected outage timeouts | [Result](TEST-07_Network_Robustness/RESULT_2026-10-11.md) |

**Pass granularity:** TEST-01–03 certify only their listed functions; TEST-04 has a software-only result; TEST-05 has FULL PASS for the specified TCP echo protocol. Neither short Ping nor a few HTTP requests certify throughput, uptime over days, security, or operation of the microSD subsystem. TEST-04 software state and actual pin voltage are separate measurements.

## Reproduction workflow

1. Assistant edits firmware/documentation **directly in this GitHub repository**.
2. User runs `git pull --ff-only` locally, then `arduino-cli compile`, `arduino-cli upload` and `arduino-cli monitor`.
3. User supplies actual console output, browser screenshots or instrument measurements.
4. A `RESULT_YYYY-MM-DD.md` file is created/updated **only** from observed evidence, with PASS/PARTIAL/FAIL and non-claims.
5. Unverified steps remain unverified; compile success is not hardware certification.

Example reproduction commands for TEST-05:

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-05_TCP_Echo"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Leave microSD out. Check the current Arduino DHCP IP. In another PowerShell window run `Test-TcpEcho.ps1` from TEST-05 with `-IP <actual-IP>`. Do not assume previously assigned DHCP addresses remain valid.

## Project decision after TEST-12

The onboard microSD/shared-SPI investigation is **closed**. TEST-12 failed the final stock-library acceptance sequence (`SD -> LAN -> SD`) on replacement hardware: SD baseline write/read passed, but Ethernet DHCP failed immediately after the SD phase. Together with the prior state-dependent and card-dependent results, the tested clone boards are classified as defective or unsuitable for reliable combined W5100 + onboard microSD operation.

No further SD repair, rework, pull-up experiments, library forks, or attempts to force a PASS are planned. The W5100 itself is now a **legacy / outdated Ethernet controller**, so deep repair or custom driver work for this inexpensive clone is not justified by the project goals. LAB-01 continues as **Ethernet-only** for bounded educational and applied networking tasks. In Ethernet firmware, keep microSD CS D4 HIGH when the SD subsystem is not used. Future work may continue with DHCP, TCP, UDP, HTTP, web control and telemetry only where using this legacy hardware remains useful.

## Documentation map

- [TEST-01](TEST-01_SPI_Probe/README.md) — raw SPI probe and certified evidence.
- [TEST-02](TEST-02_Ethernet_DHCP_Ping/README.md) — DHCP and host-side Ping.
- [TEST-03](TEST-03_HTTP_Server/README.md) — browser and HTTP endpoint validation.
- [TEST-04](TEST-04_Web_Control/README.md) — Web Control API, software test evidence and outstanding GPIO measurements.
- [TEST-05](TEST-05_TCP_Echo/README.md) — TCP echo firmware, Windows client and reconnection protocol.
- [TEST-06](TEST-06_UDP_Communication/README.md) — binary UDP Echo and PowerShell verification.
- [TEST-07](TEST-07_Network_Robustness/README.md) — unplug/replug Arduino RJ45 only; no router changes.
- [Hardware inventory](../../shields/W5100-Ethernet-SD/README.md) and [legacy lab with SD](../01-UNO-W5100-SD-4GB/README.md).

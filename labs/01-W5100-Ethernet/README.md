# LAB-01B — Arduino UNO + W5100 Ethernet Shield (blue board, no SD)

> **Scope and naming.** This directory is the independently tested **blue W5100 Ethernet Shield** sequence (TEST-01–05) with microSD **removed**. It supplements, but does not supersede, the older [LAB-01 — UNO + W5100 + SD 4GB](../01-UNO-W5100-SD-4GB/README.md) and the [shield hardware inventory](../../shields/W5100-Ethernet-SD/README.md). Evidence and PASS reports apply to the **specific tested sample**, not all W5100 clones.

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

**Pass granularity:** TEST-01–03 certify only their listed functions; TEST-04 has a software-only result; TEST-05 has FULL PASS for the specified TCP echo protocol. Neither short Ping nor a few HTTP requests certify throughput, uptime over days, security, or operation of the microSD subsystem. TEST-04 software state and actual pin voltage are separate measurements.

## Test workflow (GitHub-first)

1. Assistant edits firmware/documentation **directly in this GitHub repository**.
2. User runs `git pull --ff-only` locally, then `arduino-cli compile`, `arduino-cli upload` and `arduino-cli monitor`.
3. User supplies actual console output, browser screenshots or instrument measurements.
4. A `RESULT_YYYY-MM-DD.md` file is created/updated **only** from observed evidence, with PASS/PARTIAL/FAIL and non-claims.
5. Future steps remain PENDING until demonstrated; compile success is not hardware certification.

Commands for the current TEST-05:

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-05_TCP_Echo"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Leave microSD out. Check the current Arduino DHCP IP. In another PowerShell window run `Test-TcpEcho.ps1` from TEST-05 with `-IP <actual-IP>`. Do not assume previously assigned DHCP addresses remain valid.

## Forward roadmap — planned, not certified

| Stage | Topic | Acceptance concept |
| --- | --- | --- |
| TEST-04 | Browser Web Control | D6/D7 UI + JSON + independent output level measurement |
| TEST-05 | TCP echo server + Windows client | **FULL PASS**; 20 sessions, 64-byte boundary, interrupted client/reconnect |
| TEST-06 | UDP communication | Datagram send/receive and integrity checks in LAN |
| TEST-07 | Network robustness | DHCP renewal, link unplug/reconnect, bounded soak run and failure logs |
| TEST-08 | microSD hardware certification | Card presence, init, read/write, file integrity; separate SPI/CS verification |
| TEST-09 | W5100 + microSD integration | Network traffic together with SD file read/write, SRAM pressure |
| TEST-10 | Applied miniature network device | Reproducible GPIO / telemetry demonstration with documented limits |

TEST-05 firmware and host client are hardware-verified for this bounded test. TEST-06 and later are proposed. The older LAB-01 SD branch has separate historic results and should not be silently merged with these outcomes.

## Documentation map

- [TEST-01](TEST-01_SPI_Probe/README.md) — raw SPI probe and certified evidence.
- [TEST-02](TEST-02_Ethernet_DHCP_Ping/README.md) — DHCP and host-side Ping.
- [TEST-03](TEST-03_HTTP_Server/README.md) — browser and HTTP endpoint validation.
- [TEST-04](TEST-04_Web_Control/README.md) — Web Control API, software test evidence and outstanding GPIO measurements.
- [TEST-05](TEST-05_TCP_Echo/README.md) — TCP echo firmware, Windows client and reconnection protocol.
- [Hardware inventory](../../shields/W5100-Ethernet-SD/README.md) and [legacy lab with SD](../01-UNO-W5100-SD-4GB/README.md).

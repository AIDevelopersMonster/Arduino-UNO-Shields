# LAB-01B — Arduino UNO + W5100 Ethernet Shield (blue board, no SD)

> **Scope and naming.** This directory contains the **blue W5100 Ethernet Shield** sequence with microSD **removed**: TEST-01–06 have separate evidence; TEST-07 Baseline, a repeated Cable run, TcpAbort, StartupDhcp, Soak and DhcpRenew have hardware PASS; DhcpOutage is deferred to a separate isolated router, with the PC on Wi-Fi and UNO on Ethernet, by operator decision. The first Cable FAIL remains documented. It supplements, but does not supersede, the older [LAB-01 — UNO + W5100 + SD 4GB](../01-UNO-W5100-SD-4GB/README.md) and the [shield hardware inventory](../../shields/W5100-Ethernet-SD/README.md). Evidence and PASS reports apply to the **specific tested sample**, not all W5100 clones.

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
| [TEST-07 Network Robustness](TEST-07_Network_Robustness/) | Initial DHCP retry, natural renewal, cable/DHCP recovery, TCP aborts, bounded soak, SRAM and logs | **6 hardware scenarios PASS / DhcpOutage DEFERRED** | [Hardware result](TEST-07_Network_Robustness/RESULT_2026-10-10.md); [development checks](TEST-07_Network_Robustness/VALIDATION_2026-10-09.md) |
| [TEST-08 microSD Hardware](TEST-08_microSD_Hardware/) | SD SPI/card/FAT, exclusive create, write/read CRC, seek, append, remount, cleanup | **BUILD VERIFIED / HARDWARE PENDING** | [Development checks](TEST-08_microSD_Hardware/VALIDATION_2026-10-10.md); no hardware result yet |

**Pass granularity:** TEST-01–03 certify only their listed functions; TEST-04 has a software-only result; TEST-05 has FULL PASS for the specified TCP echo protocol. Neither short Ping nor a few HTTP requests certify throughput, uptime over days, security, or operation of the microSD subsystem. TEST-04 software state and actual pin voltage are separate measurements.

## Test workflow (GitHub-first)

1. Assistant edits firmware/documentation **directly in this GitHub repository**.
2. User runs `git pull --ff-only` locally, then `arduino-cli compile`, `arduino-cli upload` and `arduino-cli monitor`.
3. User supplies actual console output, browser screenshots or instrument measurements.
4. A `RESULT_YYYY-MM-DD.md` file is created/updated **only** from observed evidence, with PASS/PARTIAL/FAIL and non-claims.
5. Future steps remain PENDING until demonstrated; compile success is not hardware certification.

Commands for TEST-07 (PowerShell 7; close other COM owners):

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git switch feature/w5100-test07-network-robustness
git pull --ff-only
$test = ".\labs\01-W5100-Ethernet\TEST-07_Network_Robustness"
& "$test\Build-Test07.ps1" -UploadPort COM4
& "$test\Test-NetworkRobustness.ps1" -SerialPort COM4 -Scenario Baseline -DurationSeconds 60
```

Leave microSD out. The TEST-07 runner obtains the current DHCP IP from UART; do not assume previously assigned addresses remain valid. Pinned dependency installation, cable/startup/DHCP/soak scenarios and objective gates are in the [TEST-07 guide](TEST-07_Network_Robustness/README.md). After its implementation branch is merged, the same files are available on `main`.

## Forward roadmap — planned, not certified

| Stage | Topic | Acceptance concept |
| --- | --- | --- |
| TEST-04 | Browser Web Control | D6/D7 UI + JSON + independent output level measurement |
| TEST-05 | TCP echo server + Windows client | **FULL PASS**; 20 sessions, 64-byte boundary, interrupted client/reconnect |
| TEST-06 | UDP communication | **FULL PASS** — 24 byte-exact UDP replies, zero timeouts |
| TEST-07 | Network robustness | Implemented: DHCP retry/renewal, cable recovery, 7 scenarios and memory gates; **6 hardware scenarios PASS / DhcpOutage DEFERRED** |
| TEST-08 | microSD hardware certification | Implemented: 15 checks and PowerShell evidence; **BUILD VERIFIED / HARDWARE PENDING** |
| TEST-09 | W5100 + microSD integration | Network traffic together with SD file read/write, SRAM pressure |
| TEST-10 | Applied miniature network device | Reproducible GPIO / telemetry demonstration with documented limits |

TEST-05 firmware and host client are hardware-verified for this bounded test. TEST-06 is hardware-verified for 24 UDP datagrams. TEST-07 Baseline and a repeated Cable run have hardware PASS with the original host runner; the first Cable FAIL remains recorded. TcpAbort also has a user-reported hardware PASS; its runner version is not present in the supplied result line. StartupDhcp, Soak and DhcpRenew now have operator-reported hardware PASS as well. DhcpRenew was resumed after identifying the RV6699 v4 Renew timeout field; the completed runner requires natural maintain rc=2. Supplied DhcpRenew UART events show four successful renewals at approximately 120 s intervals; its console shows at least 5,395 successful UDP checks and 118 TCP checks, zero reported errors and 238 free-SRAM samples of 1,133 B. This is consistent with using the proposed Renew timeout=120 as seconds on this stand; DHCPACK fields and the final summary/hashes remain unverified. DhcpOutage is deferred by operator decision to a separate isolated router, with the PC on Wi-Fi and UNO on Ethernet. The current result is 6 PASS / 1 DEFERRED, without FULL PASS; report-version/hash compatibility is also unverified. TEST-08 is now implemented and build-verified, awaiting this sample's card test. TEST-09 onward remain proposed. The older LAB-01 SD branch has separate historic results and should not be silently merged with these outcomes.

## Documentation map

- [TEST-01](TEST-01_SPI_Probe/README.md) — raw SPI probe and certified evidence.
- [TEST-02](TEST-02_Ethernet_DHCP_Ping/README.md) — DHCP and host-side Ping.
- [TEST-03](TEST-03_HTTP_Server/README.md) — browser and HTTP endpoint validation.
- [TEST-04](TEST-04_Web_Control/README.md) — Web Control API, software test evidence and outstanding GPIO measurements.
- [TEST-05](TEST-05_TCP_Echo/README.md) — TCP echo firmware, Windows client and reconnection protocol.
- [TEST-06](TEST-06_UDP_Communication/README.md) — binary UDP Echo and PowerShell verification.
- [TEST-07](TEST-07_Network_Robustness/README.md) — network recovery firmware, bounded scenarios, UART/packet logs and memory gates.
- [TEST-08](TEST-08_microSD_Hardware/README.md) — separate SPI and microSD hardware/filesystem checks, automatic UART run and evidence.
- [Hardware inventory](../../shields/W5100-Ethernet-SD/README.md) and [legacy lab with SD](../01-UNO-W5100-SD-4GB/README.md).

# LAB-01B — Arduino UNO + W5100 Ethernet Shield (blue board, Ethernet and SD)

> **Scope and naming.** This directory contains the **blue W5100 Ethernet Shield** sequence with microSD **removed for TEST-01–07 and inserted for TEST-08–09**: TEST-01–06 have separate evidence; TEST-07 Baseline, a repeated Cable run, TcpAbort, StartupDhcp, Soak and DhcpRenew have hardware PASS; DhcpOutage is deferred to a separate isolated router, with the PC on Wi-Fi and UNO on Ethernet, by operator decision. The first Cable FAIL remains documented. It supplements, but does not supersede, the older [LAB-01 — UNO + W5100 + SD 4GB](../01-UNO-W5100-SD-4GB/README.md) and the [shield hardware inventory](../../shields/W5100-Ethernet-SD/README.md). Evidence and PASS reports apply to the **specific tested sample**, not all W5100 clones.

## Hardware and wiring

- Arduino UNO / ATmega328P, blue WIZnet W5100 Ethernet Shield, HanRun RJ45, USB to Windows PC, LAN to DHCP-enabled router for TEST-02 onward.
- W5100 chip select **D10**, microSD CS **D4**, held HIGH for TEST-01–07 and used for TEST-08–09, SPI via ICSP = UNO D11 (MOSI), D12 (MISO), D13 (SCK).
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
| [TEST-08 microSD Hardware](TEST-08_microSD_Hardware/) | SD SPI/card/FAT, exclusive create, write/read CRC, seek, append, remount, cleanup | **v0.2 integrity PASS; CARD_INIT/RTR FAIL; v0.3 CARD_INIT FAIL; v0.4 ETH_SPI_BEFORE FAIL; USB-SD 16 MiB PASS; UNO SD_ONLY 13/13 PASS; W5100_COMPARE ETH_DRIVER_INIT FAIL** | [Hardware evidence](TEST-08_microSD_Hardware/RESULT_2026-10-10.md); [development checks](TEST-08_microSD_Hardware/VALIDATION_2026-10-10.md) |
| [TEST-09 Ethernet + SD](TEST-09_Ethernet_SD_Integration/) | Sequenced UDP packets written/synced/read from SD before echo; bounded load, RAM, RTR and cleanup | **v0.2 BUILD VERIFIED / hardware PENDING; first v0.1 CARD_INIT FAIL retained** | [Guide](TEST-09_Ethernet_SD_Integration/README.md); [development checks](TEST-09_Ethernet_SD_Integration/VALIDATION_2026-10-10.md) |

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
| TEST-08 | microSD hardware certification | Implemented: 15 checks and PowerShell evidence; **v0.2 integrity PASS; CARD_INIT/RTR FAIL; v0.3 CARD_INIT FAIL; v0.4 ETH_SPI_BEFORE FAIL; USB-SD 16 MiB PASS; UNO SD_ONLY 13/13 PASS; W5100_COMPARE ETH_DRIVER_INIT FAIL** |
| TEST-09 | W5100 + microSD integration | Implemented: bounded UDP -> uncached SD file readback -> UDP, SRAM and evidence gates; **v0.2 BUILD VERIFIED / hardware PENDING; first v0.1 CARD_INIT FAIL retained** |
| TEST-10 | Applied miniature network device | Reproducible GPIO / telemetry demonstration with documented limits |

TEST-05 firmware and host client are hardware-verified for this bounded test. TEST-06 is hardware-verified for 24 UDP datagrams. TEST-07 Baseline and a repeated Cable run have hardware PASS with the original host runner; the first Cable FAIL remains recorded. TcpAbort also has a user-reported hardware PASS; its runner version is not present in the supplied result line. StartupDhcp, Soak and DhcpRenew now have operator-reported hardware PASS as well. DhcpRenew was resumed after identifying the RV6699 v4 Renew timeout field; the completed runner requires natural maintain rc=2. Supplied DhcpRenew UART events show four successful renewals at approximately 120 s intervals; its console shows at least 5,395 successful UDP checks and 118 TCP checks, zero reported errors and 238 free-SRAM samples of 1,133 B. This is consistent with using the proposed Renew timeout=120 as seconds on this stand; DHCPACK fields and the final summary/hashes remain unverified. DhcpOutage is deferred by operator decision to a separate isolated router, with the PC on Wi-Fi and UNO on Ethernet. The current result is 6 PASS / 1 DEFERRED, without FULL PASS; report-version/hash compatibility is also unverified. TEST-08's initial v0.1 run passed the SD operations but failed ETH_SPI_AFTER and BOOT framing; its supplied evidence is preserved. Firmware/runner v0.2 adds shared-bus idle clocks, RTR diagnostics and a BOOT delimiter; its real run passed BOOT and the initial RTR check but failed CARD_INIT (CMD0 timeout, error_code=1/error_data=255). Both failed runs are retained. The subsequent unchanged-v0.2 run after complete power-off passed all 15 checks, including exact SD readbacks, cleanup, post-SD RTR=07D0 on both reads and sampled min_free=940 B (run 20261010-150739-TEST08-4E60244A). This certifies that bounded cold-start run on the tested sample; the earlier failure causes and repeat initialization after MCU-only reset remain unresolved. The ordinary rerun 20261010-152700-TEST08-E09B34E9 subsequently reported firmware/host PASS, checks=15/failures=0/min_free=940 B/elapsed_ms=278; only its terminal excerpt was supplied, not all intermediate UART evidence. TEST-09 is implemented with bounded UDP/SD integrity verification; the operator confirmed AVR 1.8.8 build/upload at 25,236 B Flash / 1,440 B SRAM. Its first hardware run 20261010-155517-TEST09-0EDDF020 failed CARD_INIT (CMD0 timeout, code=1/data=255) before file creation or networking. The requested standalone TEST-08 card diagnostic then passed all 15 checks in run 20261010-160339-TEST08-76CEC702 (three correct CRC readbacks, cleanup, RTR preservation, min_free=940 B, elapsed_ms=276). After another measured upload, run 20261010-160427-TEST08-DC5F04E5 failed CARD_INIT (code=1/data=255) before file operations. Both AVR 1.8.8 builds measured 14,134 B Flash / 1,050 B static SRAM. All outcomes are retained: data integrity is demonstrated, repeat initialization is unresolved. That standalone run, 20261010-162204-TEST08-6F7F511B, initialized SD and passed all card operations/cleanup but failed ETH_SPI_AFTER: first RTR=0750, second=07D0, expected=07D0 (one differing returned bit). TEST-08 v0.3 now prepares a W5100-only 1 MHz frequency diagnostic with unchanged SD clocks and strict two-read gate; its first actual run 20261010-163611-TEST08-D39AF82E failed CARD_INIT (code=1/data=255) after the initial W5100 check passed. AVR 1.8.8 build/upload measured 14,142 B Flash / 1,050 B SRAM. Post-SD RTR was not reached. The operator cancelled that proposed cold run before execution and required an algorithm audit. TEST-08 v0.4 / TEST-09 v0.2 repair CS startup and driver-owned handoff, latch occupied-bus faults, validate RTR before writes and account for close failures; TEST-09 hardware remains PENDING. TEST-08 v0.4 now has two actual ETH_SPI_BEFORE FAIL records: E272AB07 (0750 then 07D0) and DDA2F317 (FFFF twice after requested card replacement), both after CARD_INIT PASS and before register writes/file operations. A separate Windows USB-card procedure passed 16 MiB write/read and post-reconnection readback (82E149BC65C6); the supplied console is retained. This does not certify W5100 or full card capacity. The actual UNO SD_ONLY run B652B736 passed all 13 SD/file checks without W5100 register access: three exact CRC readbacks, cleanup, min_free=938 B and elapsed_ms=443. Its unchanged host verdict replays PASS. A separate W5100_COMPARE diagnostic ran as 886B359A: CARD_INIT PASS followed by ETH_DRIVER_INIT FAIL, Ethernet 2.0.2 rc=0/chip=0 at 8 MHz. No RTR comparisons or file operations occurred; the unchanged verdict replays FAIL. W5100 raw helper frequency is back at the original 4 MHz; SD and Ethernet-library frequencies are unchanged. The next control is existing TEST-07 Baseline without microSD, with guided power-off/removal/reconnection and no router changes; that control is PENDING. Because firmware and power state also change, it cannot isolate SD presence alone. Full TEST-08 v0.4, TEST-09, SD_ONLY and W5100_COMPARE firmware remain unchanged. TEST-09 remains without hardware integration acceptance. TEST-10 remains proposed. The older LAB-01 SD branch has separate historic results and should not be silently merged with these outcomes.

## Documentation map

- [TEST-01](TEST-01_SPI_Probe/README.md) — raw SPI probe and certified evidence.
- [TEST-02](TEST-02_Ethernet_DHCP_Ping/README.md) — DHCP and host-side Ping.
- [TEST-03](TEST-03_HTTP_Server/README.md) — browser and HTTP endpoint validation.
- [TEST-04](TEST-04_Web_Control/README.md) — Web Control API, software test evidence and outstanding GPIO measurements.
- [TEST-05](TEST-05_TCP_Echo/README.md) — TCP echo firmware, Windows client and reconnection protocol.
- [TEST-06](TEST-06_UDP_Communication/README.md) — binary UDP Echo and PowerShell verification.
- [TEST-07](TEST-07_Network_Robustness/README.md) — network recovery firmware, bounded scenarios, UART/packet logs and memory gates.
- [TEST-08](TEST-08_microSD_Hardware/README.md) — separate SPI and microSD hardware/filesystem checks, automatic UART run and evidence.
- [TEST-09](TEST-09_Ethernet_SD_Integration/README.md) — bounded UDP exchange with SD write/cache-discard/readback and exact host verification.
- [Hardware inventory](../../shields/W5100-Ethernet-SD/README.md) and [legacy lab with SD](../01-UNO-W5100-SD-4GB/README.md).

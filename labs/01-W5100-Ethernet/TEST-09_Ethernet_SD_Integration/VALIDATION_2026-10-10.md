# Current TEST-09 v0.2 — SPI-order repair, hardware PENDING

The initial v0.1 hardware CARD_INIT FAIL remains in the result record.
The operator requested an algorithm audit before selecting physical conditions.
No proposed cold retest was performed. Current v0.2 validation is appended below.

# TEST-09 — development validation / 2026-10-10

**BUILD VERIFIED / actual first hardware CARD_INIT FAIL.** No physical UNO, card or network was
exercised by the assistant for TEST-09. The operator
confirmed AVR 1.8.8 build/upload at 25,236 B Flash / 1,440 B SRAM, then supplied
the [actual failed run](RESULT_2026-10-10.md); no UDP/SD load began. TEST-08's actual evidence belongs to
its separate result record and does not certify this new firmware.

- Actual Arduino CLI 1.3.1 build: arduino:avr:uno, AVR 1.8.6, Ethernet 2.0.2,
  SD 1.3.0. **25,236/32,256 B Flash; 1,440/2,048 B static SRAM**. Both gates pass.
  Final helper compiler_err is empty. The first uncached build emitted only
  existing AVR core new.cpp unused tag parameter warnings.
- The real helper records used libraries, executable sections and source/HEX
  SHA256; no AVR 1.8.8 size is inferred. Library source files are not vendored.
- All four PowerShell files parse. **29 host verdict cases pass**, including
  complete declared synthetic evidence and failures for reset, missing results,
  early completion, metadata, token, CRC, RAM, cleanup, byte/counter mismatches,
  DHCP failures and failed/duplicate-sequence UDP probes.
- g++ -std=c++11 -Wall -Wextra -Werror compiles the sketch's actual crc16,
  sdRoundtrip and handleCommand functions against declared mocks. The model
  checks matching/wrong-session STOP, single RUN, 30..600 s duration limits,
  command overflow, cache discard before read, all four packet sizes, and
  rejection of corrupt readback/short write/sync/close failure. It does not
  model real card power, electrical signaling or the whole Arduino scheduler.
- Python binascii.crc_hqx independently matches the PowerShell payload CRCs
  for token 0123ABCD / sequence 0x01020304: 16 B=5A6D, 32 B=C43B,
  64 B=50ED, 128 B=230D. Sequence encoding is little-endian 04 03 02 01.

Hardware acceptance requires the actual bounded host run, all its exact UDP
echoes and terminal UART evidence, sampled SRAM >=512 B, preserved RTR and
file cleanup. No synthetic console is saved as a hardware run.

Measured local firmware SHA256: `105CADB8A25DCBD82F3C9B1051959CC43176C2695D2371AA7E9D3A577E52CCA5`.
Measured local non-bootloader HEX SHA256: `B8FD4E0759D35097B0C9980E4A39BD0FA64B9027716C67E56BF212744EBD27B7`.
These identify this AVR 1.8.6 build; the operator must retain actual local measurements.


## v0.2 ownership and abort repair

See the [SPI-order audit](../SPI_ORDER_AUDIT_2026-10-10.md). Safe CS latches,
driver-owned read completion and owner checks precede shared-bus handoff.
Known bus faults suppress further SPI cleanup/diagnostics, failed closes count,
and preparation failure no longer starts unprepared Ethernet diagnostics.
Source/version and bus_fault=0 gates retain strict data/RTR/RAM/load acceptance.
Network SPI frequency settings are unchanged.

Final actual AVR 1.8.6 / SD 1.3.0 / Ethernet 2.0.2 compilation: **25,558 B Flash /
1,444 B static SRAM** (limits 29,000/1,536). Final compiler stderr is empty.
All eight TEST-08/09 PowerShell files parse. **32 host cases pass**; additions
reject wrong firmware, missing fault evidence and bus_fault=1. The existing
actual RUN/STOP/CRC/uncached SD function model passes. The shared-bus model
compiles the actual v0.2 helpers and verifies selected-owner rejection and
pending-read completion. A new native model compiles actual finish and tests
normal cleanup, owner faults with no further SD/Ethernet I/O, file/root close
failures, failed handoff and early preparation failure without Ethernet access.

Run from the repository root with Python 3 and g++:

```powershell
python .\labs\01-W5100-Ethernet\TEST-08_microSD_Hardware\tests\test_spi_release.py
python .\labs\01-W5100-Ethernet\TEST-09_Ethernet_SD_Integration\tests\test_firmware_model.py
python .\labs\01-W5100-Ethernet\TEST-09_Ethernet_SD_Integration\tests\test_abort_model.py
```

These mocks do not prove physical throughput, pin waveforms, peak SRAM or a
root cause for the initial CARD_INIT failure. Hardware v0.2 PENDING; test the
corrected standalone TEST-08 v0.4 first.

Final local firmware SHA256:
`8D501615C8E2CE6F353B3F650C6548C23E197F9279B95122F754BC879619D6C4`.
Final local non-bootloader HEX SHA256:
`E5F5984DA0481AC7CE4F533A792AD4CA96F5687E64741B32BE8291CC1C4B3C00`.

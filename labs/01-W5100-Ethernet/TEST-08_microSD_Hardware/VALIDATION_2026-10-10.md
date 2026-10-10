# TEST-08 — development verification / 2026-10-10

**Firmware/runner 0.2 BUILD VERIFIED; operator's cold-start hardware PASS, 15/15.**
The [actual result record](RESULT_2026-10-10.md) preserves both earlier failed
runs and the subsequent supplied 15-check PASS after complete power-off.
The compiled SPI model tests a declared peripheral behavior; the cold-start
hardware run additionally confirms exact readbacks and post-SD RTR preservation
on this sample. It does not prove why either earlier failure occurred.
No UNO, shield or card was exercised by the assistant. The operator's initial
v0.1 hardware FAIL is preserved in [its own report](RESULT_2026-10-10.md).

- Real Arduino CLI 1.3.1 compilation: `arduino:avr:uno`, AVR 1.8.6, SD 1.3.0.
  Flash **14,134/32,256 B**, static SRAM **1,050/2,048 B**. Memory gates pass.
- SD sources are the official Arduino `1.3.0` tag, commit
  `a3866205cf9af4eea3e10dcdc5ac2c713f293337`.
- Final helper compilation succeeds without warnings. The initial cold build
  exposed inherited unused `tag` parameters in AVR core `new.cpp`.
- All PowerShell files parse successfully. The build helper produces the expected
  source/HEX hashes and memory summary using the real compiler.
- PowerShell 7 host verdict tests: **29 cases**. Complete synthetic evidence is
  accepted; missing/duplicate checks, failed initialization, malformed fields,
  incomplete results, wrong firmware/pins/token, unsupported FAT, zero capacity,
  corrupt CRC/length, reset and SRAM minimum/drift violations are rejected.
  Standalone startup noise is accepted only alongside a valid independent BOOT;
  glued/hidden duplicate BOOT markers are rejected. Missing/mismatched RTR probe,
  restore and post-SD readings are rejected.
- Native shared-SPI peripheral model: g++ compiles the actual Ethernet helpers
  extracted from each sketch. Current v0.2 passes; the original v0.1 from commit
  `b6fbc15ea2e78fc210446135a59e5ccfaad0db22` fails the same SD-MISO-release condition.
  This confirms the code repair under the model's assumption, not the physical
  cause of the operator's failure. No library replacement or hardware reset is used.
- CRC reference from Python `binascii.crc_hqx(data, 0xFFFF)`: A535 for 2048 bytes,
  2B28 for 2112 bytes. Both payloads contain all 256 byte values.

These checks validate compilation and host decision logic. They do not establish
SPI wiring, card presence, successful SD writes or hardware SRAM measurements.
The supplied cold-start console also passes the unchanged v0.2 verdict module.
Complete local `summary.json` / `serial.log` and actual v0.2 AVR 1.8.8 build
measurements have not yet been supplied; their hashes are not inferred.
PowerShell telemetry is disabled for local host verification processes.

Measured firmware SHA256:
`BF2F58CD4F18E442063425243D1A21B7BAAA2E0B00C47F106C26EDE71E2D3904`.
Measured non-bootloader HEX SHA256:
`44FA6C3EECC4C994417760BFD36354709C8E32D714DB42D2A636F9DD9CAB34CA`.
Hashes identify this local AVR 1.8.6 build; another core/toolchain may yield
a different binary and must report its own build measurements.

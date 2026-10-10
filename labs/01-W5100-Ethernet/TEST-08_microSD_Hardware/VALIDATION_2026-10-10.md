# TEST-08 — development verification / 2026-10-10

**BUILD VERIFIED; hardware PENDING.** No UNO, shield or card was exercised here.

- Real Arduino CLI 1.3.1 compilation: `arduino:avr:uno`, AVR 1.8.6, SD 1.3.0.
  Flash **13,848/32,256 B**, static SRAM **1,050/2,048 B**. Memory gates pass.
- SD sources are the official Arduino `1.3.0` tag, commit
  `a3866205cf9af4eea3e10dcdc5ac2c713f293337`.
- Final helper compilation succeeds without warnings. The initial cold build
  exposed inherited unused `tag` parameters in AVR core `new.cpp`.
- All PowerShell files parse successfully. The build helper produces the expected
  source/HEX hashes and memory summary using the real compiler.
- PowerShell 7 host verdict tests: **21 cases**. Complete synthetic evidence is
  accepted; missing/duplicate checks, failed initialization, malformed fields,
  incomplete results, wrong firmware/pins/token, unsupported FAT, zero capacity,
  corrupt CRC/length, reset and SRAM minimum/drift violations are rejected.
- CRC reference from Python `binascii.crc_hqx(data, 0xFFFF)`: A535 for 2048 bytes,
  2B28 for 2112 bytes. Both payloads contain all 256 byte values.

These checks validate compilation and host decision logic. They do not establish
SPI wiring, card presence, successful SD writes or hardware SRAM measurements.
The first real runner output and complete `summary.json` are still required.
PowerShell telemetry is disabled for local host verification processes.

Measured firmware SHA256:
`31FBB78FC55D3C13D97AB808403ED0DA1F9C2EE7B7222C1B178891310EC19CD5`.
Measured non-bootloader HEX SHA256:
`9E8AF340D93FC055EE8428E4F84E0D18BFFDB7461846DDC401E95E29D54DE0D8`.
Hashes identify this local AVR 1.8.6 build; another core/toolchain may yield
a different binary and must report its own build measurements.

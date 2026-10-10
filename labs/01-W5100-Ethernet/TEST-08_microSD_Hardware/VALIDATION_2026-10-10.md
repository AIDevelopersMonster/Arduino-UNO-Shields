# TEST-08 — development verification / 2026-10-10

**Firmware/runner 0.3 BUILD VERIFIED; 1 MHz diagnostic hardware PENDING.**
The new frequency diagnostic is described below. Earlier v0.2 data-integrity
PASS runs and intermittent CMD0 / RTR FAIL runs remain in the actual result
record. In the latest 6F7F511B run the first post-SD RTR read differs by bit 7
(0750 versus 07D0), while the second read is correct; the verdict remains FAIL.

## Historical v0.2 development checks

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
  extracted from each sketch. The then-current v0.2 passes; the original v0.1 from commit
  `b6fbc15ea2e78fc210446135a59e5ccfaad0db22` fails the same SD-MISO-release condition.
  This confirms the code repair under the model's assumption, not the physical
  cause of the operator's failure. No library replacement or hardware reset is used.
- CRC reference from Python `binascii.crc_hqx(data, 0xFFFF)`: A535 for 2048 bytes,
  2B28 for 2112 bytes. Both payloads contain all 256 byte values.

These checks validate compilation and host decision logic. They do not establish
SPI wiring, card presence, successful SD writes or hardware SRAM measurements.
The supplied cold-start console also passes the unchanged v0.2 verdict module.
Complete operator `summary.json` / `serial.log` and source/HEX hashes have not
been supplied. Subsequent actual v0.2 AVR 1.8.8 build/upload output confirms
14,134 B Flash / 1,050 B static SRAM; this does not establish artifact hashes.
PowerShell telemetry is disabled for local host verification processes.

Measured firmware SHA256:
`BF2F58CD4F18E442063425243D1A21B7BAAA2E0B00C47F106C26EDE71E2D3904`.
Measured non-bootloader HEX SHA256:
`44FA6C3EECC4C994417760BFD36354709C8E32D714DB42D2A636F9DD9CAB34CA`.
Hashes identify this local AVR 1.8.6 build; another core/toolchain may yield
a different binary and must report its own build measurements.

## v0.3 W5100-frequency diagnostic

The behavioral change is W5100 helper SPISettings from 4 MHz to 1 MHz,
including the idle byte with both slaves deselected. SD library/init/data
settings, mode, CS selection, probe/restore and both post-SD reads are unchanged.
BOOT and build/run summaries identify v0.3 and 1 MHz. Version/frequency/source
hash gates require a new measured upload and do not admit v0.2 as this condition.

- Actual Arduino CLI 1.3.1 / AVR 1.8.6 / SD 1.3.0 helper build: **14,142 B Flash /
  1,050 B static SRAM**, within the unchanged 29,000/1,200 B limits. Inherited
  AVR-core new.cpp unused-parameter warnings are present; compilation succeeds.
- All four PowerShell files parse. **32 host verdict cases pass**. Added cases
  reject an absent or 4 MHz frequency banner and one incorrect RTR bit even
  when the second read matches. Existing data-integrity/cleanup/SRAM/token/BOOT
  rejection gates remain strict. Fixture successes are synthetic, not hardware.
- The g++ shared-bus model compiles the actual 1 MHz helpers, verifies their
  SPISettings and release sequence and preserves probe/restore/read assertions.
  It models ideal transfers with a specified SD-MISO-release condition; it
  does not emulate analog noise, setup/hold margins or the observed bit error.
- Before updating the module to v0.3, the full 6F7F511B console was replayed
  through unchanged v0.2 decision logic and returned FAIL for RTR preservation,
  incomplete check count and terminal failure. Historical evidence is not
  rewritten to match the new BOOT version.

No new physical run has occurred. One future PASS at 1 MHz would certify that
bounded run only, not establish a causal explanation, eliminate intermittent
CMD0 failures, prove 4 MHz reliability or validate TEST-09 network traffic.

Measured local v0.3 firmware SHA256:
`724766F976E4C549D6514A68E19762FA7EEAC56A4695BD183242B0FF5755D083`.
Measured local v0.3 non-bootloader HEX SHA256:
`49F247FFCBF88598C05B0721CCE15BDDA4C4A1BB4D254B25CBF1788607E7773F`.
These identify the stated AVR 1.8.6 build; the operator's AVR 1.8.8 build must
record its own binary and measurements.

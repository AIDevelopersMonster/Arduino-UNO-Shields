# TEST-08 — development verification / 2026-10-10

**Current firmware/runner 0.4 BUILD VERIFIED; hardware PENDING.**
The last actual v0.3 run failed CARD_INIT. The operator stopped condition
selection and requested the SPI-order audit; no proposed cold run occurred.
Current v0.4 repairs and their validation are recorded below.

## Historical v0.3 hardware record

**Firmware/runner 0.3 BUILD VERIFIED; actual 1 MHz run CARD_INIT FAIL.**
The operator's AVR 1.8.8 build/upload measures 14,142 B Flash / 1,050 B static
SRAM. Run D39AF82E passes the initial W5100 check but fails SD CMD0 init
(code=1/data=255); its supplied console is accepted as FAIL by the unchanged
v0.3 module. This is a real hardware failure, not a development-test failure.
The proposed cold-condition diagnostic has not yet occurred.
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

The operator's first v0.3 physical run failed SD init before post-SD RTR checks.
One future PASS at 1 MHz would certify that
bounded run only, not establish a causal explanation, eliminate intermittent
CMD0 failures, prove 4 MHz reliability or validate TEST-09 network traffic.

Measured local v0.3 firmware SHA256:
`724766F976E4C549D6514A68E19762FA7EEAC56A4695BD183242B0FF5755D083`.
Measured local v0.3 non-bootloader HEX SHA256:
`49F247FFCBF88598C05B0721CCE15BDDA4C4A1BB4D254B25CBF1788607E7773F`.
These identify the stated AVR 1.8.6 build; the operator's AVR 1.8.8 build must
record its own binary and measurements.


## Current v0.4 SPI-order repair

The [audit](../SPI_ORDER_AUDIT_2026-10-10.md) distinguishes code defects from
unproven physical causes. W5100 returns to the original 4 MHz. CS latch HIGH
precedes OUTPUT; SD init precedes W5100 clocks; pending read completion stays
with Sd2Card before deselected idle clocks and a separate W5100 transaction.
Selected owners are rejected without forcing SD CS high; the fault latches.
RTR initial stability is checked before writing the probe or saved value.
Close failures count, and known owner faults suppress further SPI cleanup.
The baseline follows initial SD init and spans file operations plus remount
reinitialization; pre/post preservation across the first startup init is not
measured by this baseline. Both final reads remain mandatory and exact.

- Final actual AVR 1.8.6 / SD 1.3.0 helper build: **14,422 B Flash / 1,052 B
  static SRAM**. Gates 29,000/1,200 B pass. Final compiler stderr is empty.
- All eight TEST-08/09 PowerShell files parse; TEST-08 host verdict suite has
  **36 passing cases**, including selected-owner fault/missing fault evidence,
  wrong version/frequency, unverified zero/FFFF baseline and bad-first/good-second RTR.
- g++ compiles actual helpers from both sketches. It tests inactive latch before
  output, legitimate pending-read completion, unreleased-owner rejection,
  latched-fault behavior, idle-byte handoff and unsuppressed one-bit corruption.
- The preceding v0.3 helpers fail this same model's declared pending-read case:
  application forcibly deselects unfinished SD operation. This condition is a
  regression model, not evidence that a partial read existed in the operator's
  failing run. SD 1.3.0 ordinarily completes reads automatically in default mode.
- Before editing the module, supplied D39AF82E was replayed with unchanged v0.3
  logic as FAIL. Historical consoles and verdicts remain unchanged.

The model establishes protocol/ownership behavior under its declared assumptions,
not analog margins or the physical root cause. No assistant-operated hardware run
has occurred. The next actual check is one ordinary v0.4 run with the same card
and cables; physical startup/frequency-condition selection is paused.

Final local firmware SHA256:
`96AA6C320BA123BDAA8BA064B9017B6B4FCCDEAC1F6211078EE6A1536FDE0286`.
Final local non-bootloader HEX SHA256:
`0E5B0A2CD49D058EA977A3B49DC09E2BB94B8FC7217969DB73D70A4CFD5C4805`.
Another AVR/core build must report its own binary hashes and measurements.


## USB-card procedure 0.1 and new operator evidence

TEST-08 v0.4 subsequently produced two actual ETH_SPI_BEFORE FAIL records,
E272AB07 (0750/07D0) and DDA2F317 (FFFF twice), both following CARD_INIT PASS.
Their unchanged-v0.4 verdict replays fail. The local build/ownership model
results above remain software results. Firmware is unchanged.

The separately prepared Windows USB-card procedure has two parsed PowerShell
files and an actual Add-Type C# build. Its 11 local file-core cases pass,
including a 16 MiB roundtrip with independent SHA-256 and refusal of occupied
filenames, data corruption, wrong seeds, truncation, trailing data and expired
budget. See [USB_SD_CHECK.md](USB_SD_CHECK.md) for scope and commands.
Windows Storage discovery/remount and the operator's card have not been tested
locally. The operator subsequently reported actual USB-SD PASS, recorded below.

## Operator USB-SD PASS and separate UNO SD_ONLY diagnostic

Operator run `20261010-183308-USBSD-82E149BC65C6` reports 16 MiB written,
every-byte/SHA-256 readback PASS and readback PASS after actual USB reader
disconnection/reconnection. Both SHA-256 values are
`F3D84EBB8A9E35117DDFF9B5A26177D003345FE343EAC8A91A99C11444FB4074`.
The supplied console excerpt is preserved in evidence. Full run summary,
seed and source hashes were not supplied. This bounded USB-SD result does
not change either full TEST-08 v0.4 FAIL or certify UNO shared SPI.

The next check is now the separate [SD_ONLY](diagnostics/SD_Only/README.md),
not another full TEST-08 retry. It retains SD file operations, omits W5100
register accesses, holds ETH CS high, uses exclusive SDONLY.BIN, and has
13 checks. Ethernet is still physically installed on the shared bus.
Its distinct BOOT/result/runner prevents promotion to full TEST-08 PASS.

Actual final local build with Arduino CLI 1.3.1 / AVR 1.8.6 / SD 1.3.0:
**13,758 B Flash / 1,050 B static SRAM**; compiler stderr is empty; libraries
are SPI 1.0 and SD 1.3.0 only. All four diagnostic PowerShell files parse.
All **36 synthetic host verdict cases** pass. Independent Python payload
CRC-HQX matches A535 for 2048 B and 2B28 for 2112 B. Application source
contains no Ethernet include, SPI.transfer, RTR helper or ETH_CS LOW;
13 named checks start with CARD_INIT and finish with RAM. This is source
and host validation, not a physical SPI waveform or SD_ONLY hardware result.

Final SD_ONLY source SHA-256:
`7B6702EC1BA40A1107206D5E54246E519B3D97D1F6007FD29485D8834DCE818C`.
Local non-bootloader HEX SHA-256:
`BC6D411DADEFC44B1B87823263DF93DD04EE26F4856CDFBF5DAF4686466143CD`.
Final runner/module hashes match the measured build. Full TEST-08 v0.4 and
TEST-09 v0.2 firmware hashes above remain unchanged. Hardware SD_ONLY PENDING.

# Shared SPI algorithm audit — TEST-08 v0.4 / TEST-09 v0.2

The operator stopped selection of frequency/power conditions after the actual
TEST-08 v0.3 CARD_INIT failure D39AF82E and required an algorithm audit.
The proposed v0.3 cold run was not performed. Both revised firmwares are BUILD
VERIFIED. TEST-08 v0.4 subsequently failed ETH_SPI_BEFORE in two actual runs;
TEST-09 v0.2 hardware remains PENDING. All supplied PASS and FAIL records remain
unchanged. The operator selected a separate Windows USB-card check, which
subsequently passed 16 MiB write/read and reconnected readback (82E149BC65C6).
UNO SD_ONLY subsequently passed 13/13 (B652B736). A separate W5100_COMPARE
diagnostic is now prepared, hardware PENDING.

## Findings and repairs

| Finding in the preceding code | Repair | Evidence limit |
| --- | --- | --- |
| setup used OUTPUT before setting CS HIGH. AVR pinMode does not preload the output latch; a LOW latch can briefly select a slave. | Set both inactive latches HIGH, then both OUTPUT directions, then SPI.begin. Applied in TEST-08 and TEST-09. | This prevents an application-start select pulse under the declared latch condition. No actual pulse was measured; bootloader/reset-time pin behavior is outside this code's control. |
| Helpers forced SD CS HIGH, then overwrote SPI configuration without checking driver completion/ownership. | While Ethernet is deselected, call Sd2Card.readEnd; require both CS inactive; clock FF in SD's configuration; end that transaction; only then start/select Ethernet. If the owner remains selected, latch bus_fault and return FAIL without forcing its CS. | In normal default-mode successful SD 1.3.0 reads, the library already completes the block. The old unsafe partial-read case is proven in the declared model, not shown to have occurred in the supplied failing run. |
| TEST-08 addressed W5100 before initially placing SD into SPI mode. | SD init is first; no W5100 probe after failed CARD_INIT. First two CHECK indexes are now CARD_INIT=1 and ETH_SPI_BEFORE=2. | This is a declared initialization-order invariant. TEST-09 already initialized SD first and still failed; order alone cannot explain all observed CMD0 failures. |
| TEST-08 wrote its probe and restored the first RTR sample before validating the two initial readings. | Verify a stable nonzero/non-FFFF baseline before either write; never restore an unverified sample. Exact probe/restore and both final reads remain mandatory. | Latest observed bit discrepancy was after SD, not during the initial baseline. It does not prove this latent defect caused that discrepancy. |
| Final closes were ignored, and final diagnostics could enter Ethernet after SD preparation failure or after an owner fault. | Count close failures; include root close in TEST-08 cleanup gate; skip new SPI cleanup/diagnostics after a known bus fault. TEST-09 only uses prepared Ethernet, and closes the UDP socket before final RTR reads after a successful handoff. | No close failure was observed in the supplied successful-cleanup runs. These are error-path correctness repairs. |

The intended boundary order is driver completion, inactive-CS check, deselected
SD idle clocks, SD transaction end, Ethernet transaction begin, Ethernet select,
exact 32-bit W5100 frame, Ethernet deselect and transaction end. The code does
not introduce an additional pause, application retry, voting over read results,
post-SD RTR write, reformatting or a relaxed first-read criterion.

W5100 raw helpers are restored to 4 MHz; SD init remains 250 kHz and SD data
4 MHz. TEST-09's Ethernet-library SPI frequency settings are unchanged.
Version/clock/source-hash gates distinguish revised firmwares. RESULT must have
bus_fault=0 for acceptance; a known owner fault prevents further SPI work.

The TEST-08 initial RTR baseline now follows the first SD initialization. Its
preservation check covers all subsequent file operations and the repeated
card initialization during remount. A before/after RTR measurement across the
very first startup SD init is not claimed by this baseline. This scope change
is explicit; all post-baseline readings, data integrity and cleanup checks
remain strict. CRC success verifies tested readbacks; it does not cancel an
observed RTR discrepancy or a failed initialization.

## Validation

Final actual Arduino CLI 1.3.1, AVR 1.8.6 builds with SD 1.3.0:

| Firmware | Flash / 32,256 B | Static SRAM / 2,048 B | Hardware status |
| --- | --- | --- | --- |
| TEST-08 v0.4 | 14,422 | 1,052 | Two actual ETH_SPI_BEFORE FAIL |
| TEST-09 v0.2, Ethernet 2.0.2 | 25,558 | 1,444 | PENDING |

Final compiler stderr is empty. All eight PowerShell files parse; 36 TEST-08
and 32 TEST-09 host cases pass. Native g++ compiles actual helpers from both
sketches and checks safe output latches, driver-owned partial-read completion,
selected-owner rejection, fault latching, idle clocks and unsuppressed bad
read bytes. Preceding TEST-08 v0.3 fails the same model's pending-read condition
by forcing SD deselection. The actual TEST-09 finish function is separately
compiled to test safe aborts, both close failures, handoff failure and an early
init failure with no unprepared Ethernet access. Existing command/SD models pass.

These are software tests with declared peripheral mocks. They do not measure
analog waveforms, voltage levels, physical pin transients, peak SRAM, throughput
or the cause of the operator's failures. The subsequent normal run E272AB07
failed with 0750/07D0 before any RTR write. After the requested replacement,
DDA2F317 returned FFFF twice; CARD_INIT passed in both. Those findings do not
prove a damaged card, waveform cause or a new software defect. A proposed
byte-level trace was not implemented because the operator selected the
[Windows USB-card procedure](TEST-08_microSD_Hardware/USB_SD_CHECK.md) instead.
No firmware or acceptance-gate change was made after these failures.

Primary inspected implementations:
[AVR 1.8.8 pinMode/digitalWrite](https://github.com/arduino/ArduinoCore-avr/blob/1.8.8/cores/arduino/wiring_digital.c),
[AVR 1.8.6 SPI transactions](https://github.com/arduino/ArduinoCore-avr/blob/1.8.6/libraries/SPI/src/SPI.h),
[SD 1.3.0 init/readEnd/writeBlock/select handling](https://github.com/arduino-libraries/SD/blob/1.3.0/src/utility/Sd2Card.cpp),
[SD 1.3.0 filesystem cache flush](https://github.com/arduino-libraries/SD/blob/1.3.0/src/utility/SdVolume.cpp).


## Follow-up isolation after USB-card PASS

The supplied USB-SD console reports 16 MiB write, exact readback and the same
SHA-256 after USB reconnection. This does not isolate UNO SPI causes. The next
[SD_ONLY diagnostic](TEST-08_microSD_Hardware/diagnostics/SD_Only/) reuses SD file
operations while removing W5100 register transactions. Ethernet remains on the
physical shared bus with its CS HIGH; an electrical isolation claim is not made.
SD init/data clocks stay unchanged. The diagnostic is separately named and
requires 13 SD checks; it cannot be counted as the full 15-check TEST-08 PASS.
Both full firmwares remain unchanged. SD_ONLY now has actual PASS B652B736: all
13 checks, three exact CRC readbacks, cleanup, min_free=938 B and 443 ms.
The supplied console replays PASS under the unchanged diagnostic verdict.
This is one bounded run, not a hardware root-cause proof.

Next [W5100_COMPARE](TEST-08_microSD_Hardware/diagnostics/W5100_Compare/) compares
TEST-08 raw and actual Ethernet 2.0.2 driver RTR reads at the same 4 MHz.
Three ABBA cycles are captured before files, after write/append and after
remount. No read is discarded or outvoted. A discrepancy remains FAIL while
SD diagnostics continue if chip-select ownership remains sound. An owner fault
aborts further SPI. The library is initialized once after SD init; that
normal driver initialization resets/configures W5100 at 8 MHz on UNO and
differs from the old raw-only initialization. Results cannot erase that
confound or certify the original algorithm without initialization. Hardware
W5100_COMPARE is PENDING; full TEST-08/09 and SD_ONLY sources are unchanged.

# W5100_COMPARE v0.1 — raw / Ethernet 2.0.2 RTR comparison

Prepared after actual USB-SD 16 MiB PASS and UNO SD_ONLY 13/13 PASS B652B736.
**BUILD VERIFIED; hardware W5100_COMPARE PENDING.** Full TEST-08 v0.4,
TEST-09 v0.2 and SD_ONLY firmware/verdicts are unchanged.

This separate diagnostic compares the TEST-08 v0.4 raw read algorithm with
the actual Ethernet 2.0.2 `W5100.readRTR()` implementation on the same UNO.
Both comparison paths use 4 MHz / MSB first / mode 0 and the same SD driver
completion/idle-byte handoff. RAW uses per-byte begin/end transactions and
digitalWrite CS, as before; LIB uses the library's paired read and fast AVR
CS within one transaction. It distinguishes these complete access paths,
not a single instruction or an analog cause.

SD is initialized first at 250 kHz, then uses 4 MHz. The Ethernet library is
initialized once through `Ethernet.init(10)` / `W5100.init()`. This performs
its normal chip detection, reset and configuration, at actual UNO 8 MHz
(SPI divider reported as ETH_INIT init_spi_hz). It differs from raw-only
TEST-08 startup, which never called that driver initialization. Subsequent
comparison reads are explicitly 4 MHz and report SPCR=50/SPI2X=0.
No frequency selection or extra application startup delay is introduced;
the library's own initialization delay/detection belongs to the tested path.

No DHCP, UDP, TCP or router configuration is involved. There is no application
RTR probe/restore/write; the library's initialization itself writes registers.
An observed PASS cannot establish that old raw accesses without initialization
work or that a particular instruction caused the prior failures.

## One automatic run — PowerShell 7

Leave the USB-verified card inserted in the same UNO/Shield; leave existing
cables connected. Close Arduino monitor. No power/card/frequency selection,
operator timing or keys during the test are required.

```powershell
git pull --ff-only
$cmp = ".\labs\01-W5100-Ethernet\TEST-08_microSD_Hardware\diagnostics\W5100_Compare"
& "$cmp\Build-W5100Compare.ps1" -UploadPort COM4
& "$cmp\Test-W5100Compare.ps1" -SerialPort COM4 -DurationSeconds 90
```

Branch: feature/w5100-test09-ethernet-sd. Build gates: AVR 1.8.6/1.8.8,
SD 1.3.0, Ethernet 2.0.2, Flash <=29000 B and static SRAM <=1200 B.
The helper records source/HEX hashes and uploads the measured binary. The
runner verifies build/source/BOOT, sends a token and saves serial.log /
summary.json in its own runs. PASS/FAIL appear in green/red.
90 s is the host response limit, not a load interval or MCU watchdog.

## What is retained and what PASS requires

Three comparison phases: BEFORE files, AFTER_IO write/append/readback and
AFTER_REMOUNT. Each phase has three RAW–LIB–LIB–RAW cycles. All **36 RTR
observations** are logged as RREG with phase/cycle/order/method/value and
actual SPI control bits. Four reads are captured before their UART printing.
The first LIB value is the declared baseline, not an inferred true register
value. Zero/FFFF is rejected. Every RAW and LIB observation must equal it.
An incorrect first read remains FAIL even when later reads match; no voting.

Exactly **17 CHECK**, all PASS, in order:

CARD_INIT, ETH_DRIVER_INIT, RTR_BEFORE, CARD_INFO, FAT_VOLUME, ROOT_OPEN,
EXCLUSIVE_CREATE, WRITE_2048, REOPEN_VERIFY_2048, SEEK_BOUNDARIES, APPEND_64,
REOPEN_VERIFY_2112, RTR_AFTER_IO, REMOUNT_VERIFY, RTR_AFTER_REMOUNT,
REMOVE_TEST_FILE, RAM.

SD payload/readback/CRC/seek/append/remount code is copied from SD_ONLY.
Exclusive W51CMP.BIN refuses an occupied name. Readbacks must be exactly
2048 B / A535, 2112 B / 2B28, remount 2112 B / 2B28, including every byte
and EOF; the run-owned file is removed. Sampled min_free >=512 B and free
SRAM drift <=64 B are required. Terminal checks=17, failures=0, bus_fault=0,
compare_failures=0, rtr_samples=36, verified bytes=2112 and CRC=2B28.

An RTR discrepancy is logged and counted; SD diagnostics continue to provide
file evidence while ownership remains valid. The complete result still FAILs.
SD-stage or driver-init failure stops progression. A chip-select ownership
fault latches, aborts new SPI work and suppresses cleanup through the uncertain
bus. Failures may leave the owned file for diagnosis. No existing file is
overwritten/deleted. No automatic retry is performed after failure.

Matching paths in this bounded run do not prove physical signal margins,
full media capacity, repeatability, peak stack, network integration or the
root cause of earlier TEST-08 failures. The two paths can share a common fault;
equal values alone do not independently prove true physical register contents.

## Actual software validation, 10 October 2026

Arduino CLI 1.3.1 / AVR 1.8.6 / SD 1.3.0 / Ethernet 2.0.2:
**17,184 B Flash / 1,130 B static SRAM**. Compiler stderr contains four
unused-parameter warnings for `tag` in AVR core new.cpp; none from this sketch. Four
PowerShell files parse; **54 synthetic verdict cases pass**, including missing
phases/samples, wrong methods/SPI bits, wrong chip/initialization, bad CRC/RAM,
FFFF/zero and an incorrect first raw value followed by good observations.
The native model compiles the actual read/comparison helpers and verifies
ownership, 36 observations, bad-first retention and FFFF rejection. Its LIB
peripheral is a frame-compatible mock; the actual library is compiled by the
Arduino build. Neither model is hardware evidence.

Firmware SHA-256:
`AA0EBFEDF7F42903B5E54724E4061A8CAE1CAB8800A9CB4E9459803854DE4170`.
Local non-bootloader HEX SHA-256:
`F78A13F99B7B7BB43321D1AC1111ADDCC8E00AD6E89C8EA81C9EE7C4DDAFA0AF`.
User AVR 1.8.8 build must record its own measurements and hashes.

Primary implementation inspected:
[Ethernet 2.0.2 W5100 init/read](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/utility/w5100.cpp),
[readRTR/AVR CS](https://github.com/arduino-libraries/Ethernet/blob/2.0.2/src/utility/w5100.h),
[SD 1.3.0 transactions](https://github.com/arduino-libraries/SD/blob/1.3.0/src/utility/Sd2Card.cpp).

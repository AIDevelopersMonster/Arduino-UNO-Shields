# TEST-10 — KON_SPI_Shared transition test

First bench test for library `KON_SPI_Shared v0.1`.

## Why this exists

TEST-09D proved that SD and W5100 work independently, but sequential switching failed in the observed setup. Audit of the stock libraries found:

- Arduino SD 1.3.0 uses SPI transactions internally, but `SD.end()` only closes the root object; it does not reset the card or SPI peripheral.
- SD initialization explicitly sends >=74 clocks with CS HIGH before CMD0.
- Ethernet 2.0.2 has no public Ethernet `end()`.
- W5100 has a hardware software-reset bit in Mode Register 0x0000, but the library helper is private.
- W5100 initialization uses a static `initialized` guard, so a later `W5100.init()` is not the same lifecycle as the first boot.

TEST-10 adds explicit bus boundaries rather than modifying the entire upstream libraries immediately.

## New handoff behavior

`S0`: closes the SD root, raises D4 and sends 16 idle clocks with **both CS HIGH**.

`L1`: parks both CS, performs a raw W5100 MR reset, restores standard 2KB/socket RMSR/TMSR allocation, then invokes normal Ethernet DHCP.

`L0`: stops UDP and parks both CS.

`S1`: parks both CS, emits 80 clocks at 250 kHz, then calls normal `SD.begin(4)` and verifies a disposable file.

This is experimental. The release-clock behavior is a deliberate interoperability experiment, not yet claimed as the root-cause fix.

## Operator sequence

```text
S1
S0
L1
L0
S1
```

The desired result is PASS at every transition without AVR reset.

## Files/data

S1 deletes/recreates `K10.TXT`; do not use a card where that filename contains important data.

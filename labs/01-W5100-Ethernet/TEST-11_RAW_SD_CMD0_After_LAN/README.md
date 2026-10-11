# TEST-11 — RAW SD CMD0 After LAN

Purpose: determine whether the reproducible TEST-10 failure `LAN -> SD.begin()` is inside the Arduino SD library lifecycle or below it at raw SPI/card-command level.

This sketch intentionally does **not** include `SD.h`.

## Core probe

`C0` sends the SD SPI initialization preamble directly:

1. D4 HIGH and D10 HIGH.
2. 80 clocks at 250 kHz while both devices are deselected.
3. D4 LOW.
4. raw SD `CMD0` with CRC `0x95`.
5. wait for R1.
6. D4 HIGH.
7. 8 trailing clocks.

For a responding SD card entering SPI idle state, expected R1 is `0x01`.

It also reports a passive `MISO_PULLUP` sample with both CS pins HIGH. This is only a weak diagnostic: HIGH does not prove true high-impedance, while a persistent LOW despite the internal pull-up is suspicious.

## Test sequence

After flash/reset:

```text
C0
L1
L0
C0
```

Do not reset the UNO between commands.

Interpretation:

- `C0 PASS` before LAN and `C0 FAIL` after LAN: failure exists below `SD.begin()`; investigate SPI electrical/protocol state and W5100/shield behavior.
- `C0 PASS` both before and after LAN while TEST-10 `SD.begin()` fails: focus on SD library reinitialization/lifecycle.
- `C0 FAIL` even before LAN: TEST-11 baseline is invalid; do not infer anything about LAN.

`L1` uses the same W5100 MR software reset + DHCP path as TEST-10 and binds UDP port 5005.

No filesystem access occurs and no SD file is modified.

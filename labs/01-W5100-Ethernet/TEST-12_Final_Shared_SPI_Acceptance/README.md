# TEST-12 — Final Shared SPI Acceptance

Final bounded acceptance test for Arduino UNO + W5100 Ethernet Shield + onboard microSD.

This test intentionally removes the experimental KON-SPI recovery layer. It uses only the stock Arduino libraries `SPI`, `SD`, `Ethernet`, and `EthernetUDP`.

## Automatic sequence

After a 1500 ms power-settle delay:

1. **SD1** — `SD.begin(4)`, write `K12.TXT`, close, reopen, byte-verify, `SD.end()`.
2. **LAN** — W5100 DHCP, open UDP port 5007, then stop UDP.
3. **SD2** — repeat SD initialization and independent write/read verification after LAN activity.

Both chip-select pins are explicitly parked HIGH when inactive. A small number of idle SPI clocks are emitted with both slaves deselected between phases.

There are no operator commands, no retries, and no raw W5100 reset. This makes the result easy to interpret.

## Acceptance criterion

Only this result is a FULL PASS:

```text
SD1 INIT PASS
SD1 WRITE PASS
SD1 READ VERIFY PASS
LAN DHCP PASS IP=...
LAN UDP PASS PORT=5007
LAN STOP / D10 HIGH
SD2 INIT PASS
SD2 WRITE PASS
SD2 READ VERIFY PASS
FINAL: FULL PASS / SD -> LAN -> SD
```

Any other final status is FAIL for reliable shared SD/W5100 use on this shield/UNO combination.

## Data warning

`K12.TXT` is deleted/recreated by the test. Do not use a card where this filename contains valuable data.

## Lab decision after this test

If TEST-12 fails again on the replacement UNO, stop the SD investigation. Record the onboard microSD subsystem / shared SPI integration of this shield as unreliable for this lab and continue LAB-01 in **Ethernet-only** mode with D4 held HIGH.

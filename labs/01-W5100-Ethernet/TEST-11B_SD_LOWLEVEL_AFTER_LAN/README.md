# TEST-11B — SD Low-Level Init After LAN

This replaces the invalid hand-written CMD0 baseline in TEST-11.

The sketch uses the Arduino SD library's own `Sd2Card::init()` directly, but does **not** mount a FAT volume and does not open files. This preserves the known-good SD command implementation while exposing its native low-level error code.

## Sequence

```text
C
C
L1
L0
C
```

No AVR reset between commands.

Expected valid baseline: the first two `C` commands should both report `init=PASS`.

After LAN activity, the final `C` is decisive:

- PASS: low-level card initialization still works; investigate volume/root or higher SD lifecycle.
- FAIL error 0x01 CMD0: card does not enter SPI idle state after LAN.
- FAIL error 0x02 CMD8: version/voltage-check stage failed.
- FAIL error 0x08 ACMD41: card remained in initialization loop.
- FAIL error 0x06 CMD58: OCR stage failed.

The sketch does not modify any SD file.

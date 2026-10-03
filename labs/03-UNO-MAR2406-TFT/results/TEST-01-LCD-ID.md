# TEST-01 — LCD controller identification result

**Date:** 2026-10-03  
**Board:** Arduino UNO compatible  
**Shield:** MAR2406 2.4-inch TFT Touch  
**Port used during bench run:** COM4  
**Firmware:** `sketches/01_LCD_ID_Probe/01_LCD_ID_Probe.ino`

## Raw serial capture

```text
LAB-03 TEST-01 - MAR2406 LCD ID Probe v0.1
Arduino UNO, direct 8-bit parallel register read
No external display library is used.
A white/uninitialized LCD during this test is normal.

Bus map: D0=D8 D1=D9 D2=D2 D3=D3 D4=D4 D5=D5 D6=D6 D7=D7
Control: RD=A0 WR=A1 RS=A2 CS=A3 RST=A4
Hardware reset...
Reset complete.

=== MAR2406 LCD REGISTER PROBE ===
D3 ID4  [0xD3] : 00 00 93 41
04 RDDID [0x04] : 41 00 00 00
09 RDDST [0x09] : 00 00 61 00 00
DA ID1   [0xDA] : 00 00
DB ID2   [0xDB] : 00 00
DC ID3   [0xDC] : 00 00

RESULT: ILI9341 signature 0x9341 detected in D3 response.
TEST-01: candidate PASS; record the full capture in the lab result.
Commands: r=repeat, x=reset+repeat
```

## Interpretation

The raw `0xD3` response contains the adjacent bytes `93 41`.

This is the expected ILI9341 identification signature, so the controller on the tested physical MAR2406 sample is now **bench-verified as ILI9341**.

The zero responses from `0xDA`, `0xDB` and `0xDC` do not override the positive `0xD3` identification on this shield/read path.

## Result

**PASS — ILI9341 confirmed on the physical shield.**

The successful read also gives direct evidence that the LCD parallel data bus and the control path used by TEST-01 are functioning well enough for register communication.

Next step: TEST-02 — initialize the ILI9341 and verify visible RGB565 graphics.

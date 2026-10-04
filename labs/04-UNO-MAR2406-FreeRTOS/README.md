# LAB-04 — Arduino UNO + MAR2406 + FreeRTOS

## Goal

Determine what a real preemptive RTOS can practically do on the classic
Arduino UNO R3 / ATmega328P with the verified MAR2406 TFT Touch shield.

LAB-03 remains closed. LAB-04 is the RTOS experiment line on the same hardware.

## Platform

- Arduino UNO R3 / ATmega328P, 16 MHz
- 32 KB Flash
- 2 KB SRAM
- MAR2406 2.4-inch TFT Touch Shield
- ILI9341, ROT1 / 320x240
- resistive touch: XP=D6, XM=A2, YP=A1, YM=D7
- microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13
- Arduino AVR core
- Arduino_FreeRTOS_Library / FreeRTOS for AVR

## Results

### TEST-01 — FreeRTOS + TFT + Touch

**PASS on physical hardware.**

Confirmed:

- FreeRTOS scheduler running;
- two application tasks;
- different task priorities;
- FreeRTOS queue;
- live worker heartbeat;
- live RTOS tick;
- TFT output;
- resistive Touch input.

Verified build:

```text
Flash: 26520 / 32256 bytes (82%)
SRAM globals: 592 / 2048 bytes (28%)
```

This proves that FreeRTOS itself is viable on the UNO for a compact,
carefully-budgeted application.

### TEST-02 — FreeRTOS SD File Manager

**CLOSED — FAIL AS A PRACTICAL ARCHITECTURE.**

First design:

```text
FreeRTOS + SD + MCUFRIEND_kbv + Adafruit_GFX + file protocol
```

Result: exceeded UNO Flash.

Staged design without TFT/GFX:

```text
Flash: 21166 / 32256 bytes (65%)
SRAM globals: 1137 / 2048 bytes (55%)
Linker-reported SRAM remaining: 911 bytes
```

With a 256-byte FILE task stack:

- SD initialization PASS;
- FILE task started;
- PING PASS;
- INFO PASS;
- LS / did not complete.

With a 384-byte FILE task stack:

- SD initialization PASS;
- task creation returned PASS;
- scheduler did not reach the FILE task startup message.

Project conclusion: runtime SRAM is too tight for a reliable FreeRTOS + SD
filesystem design using this library stack on the ATmega328P, while the full
GUI version also exceeds Flash.

The experiment is intentionally stopped here. Further byte-level optimization
would have low value compared with using either:

- ordinary Arduino scheduling on this UNO for SD/file-manager work; or
- a larger MCU for RTOS + filesystem + GUI.

## Current LAB-04 status

- TEST-01: **PASS**
- TEST-02: **FAIL / CLOSED**
- further FreeRTOS experiments on this UNO should stay compact and avoid the SD
  filesystem unless there is a specific reason to revisit the memory limit.

The failed TEST-02 firmware is retained for reproducibility.

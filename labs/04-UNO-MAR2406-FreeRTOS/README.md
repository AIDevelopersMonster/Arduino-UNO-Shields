# LAB-04 — Arduino UNO + MAR2406 + FreeRTOS

## Goal

Run a real preemptive RTOS on the same Arduino UNO R3 + MAR2406 hardware that
was certified in LAB-03, then determine how far the 32 KB Flash / 2 KB SRAM
ATmega328P can be pushed with tasks, queues, storage and an interactive HMI.

LAB-03 remains closed. LAB-04 starts a new software-architecture line on the
same verified hardware.

## Platform

- Arduino UNO R3 / ATmega328P, 16 MHz
- MAR2406 2.4-inch TFT Touch Shield
- ILI9341, canonical ROT1 / 320x240
- resistive touch: XP=D6, XM=A2, YP=A1, YM=D7
- microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13
- Arduino AVR core
- Arduino_FreeRTOS_Library / FreeRTOS for AVR

## Architecture rule

TFT and Touch have one owner task whenever the graphics stack is present,
because the shield shares GPIO resources between the LCD interface and the
resistive touch panel.

D13 is reserved for SPI SCK whenever microSD is active; it is not used as a
heartbeat LED.

## Test plan

1. **TEST-01 — Scheduler + TFT + Touch monitor**
   - HMI task
   - Worker task
   - FreeRTOS queue
   - live RTOS tick and touch state
   - physical PASS obtained on the verified shield
2. **TEST-02 — SD File Manager over USB Serial**
   - FreeRTOS FILE task
   - PC directory listing
   - binary PUT / GET in 32-byte blocks
   - CRC-16/CCITT verification
   - first full TFT/GFX + SD build exceeded UNO Flash
   - revised TEST-02A removes MCUFRIEND_kbv/Adafruit_GFX and proves storage first
3. Reintroduce a minimal TFT status layer with a direct ILI9341 driver if the
   remaining Flash/SRAM budget permits.
4. Task timing / priority experiment.
5. Port an existing application to RTOS architecture if memory permits.

## TEST-01

Firmware:

`sketches/01_FreeRTOS_TFT_Touch_Monitor/01_FreeRTOS_TFT_Touch_Monitor.ino`

Procedure:

`tests/TEST-01-FREERTOS-SCHEDULER-TFT-TOUCH.md`

## TEST-02

Firmware:

`sketches/02_FreeRTOS_SD_File_Manager/02_FreeRTOS_SD_File_Manager.ino`

Procedure:

`tests/TEST-02-FREERTOS-SD-FILE-MANAGER.md`

Status: **BUILD PASS / READY FOR BENCH**.

Verified TEST-02A build:

```text
Flash: 21166 / 32256 bytes (65%)
SRAM globals: 1137 / 2048 bytes (55%)
Linker-reported SRAM remaining: 911 bytes
```

Flash is no longer the limiting resource in the staged build. Runtime SRAM is
now the primary constraint because FreeRTOS task stacks and control structures
consume additional memory after startup.

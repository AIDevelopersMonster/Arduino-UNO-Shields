# LAB-04 — Arduino UNO + MAR2406 + FreeRTOS

## Goal

Run a real preemptive RTOS on the same Arduino UNO R3 + MAR2406 hardware that
was certified in LAB-03, then determine how far the 32 KB Flash / 2 KB SRAM
ATmega328P can be pushed with tasks, queues and an interactive TFT/Touch HMI.

LAB-03 remains closed. LAB-04 starts a new software-architecture line on the
same verified hardware.

## Platform

- Arduino UNO R3 / ATmega328P, 16 MHz
- MAR2406 2.4-inch TFT Touch Shield
- ILI9341, canonical ROT1 / 320x240
- resistive touch: XP=D6, XM=A2, YP=A1, YM=D7
- calibrated touch: LEFT=153, RIGHT=930, TOP=962, BOTTOM=168
- Arduino AVR core
- Arduino_FreeRTOS_Library / FreeRTOS for AVR

## Architecture rule

**TFT and Touch have one owner task.**

The shield shares pins between the LCD interface and the resistive touch panel.
LAB-04 therefore starts with an HMI task that owns both devices. Other tasks
communicate with HMI through RTOS mechanisms instead of writing to the display
directly.

## Test plan

1. **TEST-01 — Scheduler + TFT + Touch monitor**
   - HMI task
   - Worker task
   - FreeRTOS queue
   - LED heartbeat
   - live RTOS tick and touch state
2. Task timing / priority experiment.
3. Queue and producer/consumer experiment.
4. Optional SD logger task, subject to SRAM budget.
5. Port one existing application to RTOS architecture if memory permits.

## TEST-01

Firmware:

`sketches/01_FreeRTOS_TFT_Touch_Monitor/01_FreeRTOS_TFT_Touch_Monitor.ino`

Procedure:

`tests/TEST-01-FREERTOS-SCHEDULER-TFT-TOUCH.md`

Status: **READY FOR BUILD / BENCH**.

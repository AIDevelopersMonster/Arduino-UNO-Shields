# LAB-04 / TEST-01 — FreeRTOS scheduler + TFT + Touch

## Goal

Prove that the classic Arduino UNO R3 / ATmega328P can run a real preemptive
FreeRTOS scheduler while continuing to operate the verified MAR2406 TFT and
resistive touch interface.

This is deliberately a scheduler/architecture test, not yet a game.

## Architecture

Two application tasks are created:

- **HMI task, priority 2** — the only task allowed to access TFT and Touch.
- **WORK task, priority 1** — toggles the built-in LED and publishes a heartbeat.

The worker heartbeat is transferred to the HMI task through a FreeRTOS queue.

Keeping TFT and Touch under one task is intentional: on this shield the LCD and
resistive touch share GPIO resources, so concurrent access from independent
tasks would introduce a race.

## Required library

Install the AVR Arduino FreeRTOS library:

```powershell
arduino-cli lib install "FreeRTOS"
```

The sketch includes `Arduino_FreeRTOS.h` first, as required by the upstream
Arduino_FreeRTOS_Library.

## Compile

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\04-UNO-MAR2406-FreeRTOS\sketches\01_FreeRTOS_TFT_Touch_Monitor
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\04-UNO-MAR2406-FreeRTOS\sketches\01_FreeRTOS_TFT_Touch_Monitor
```

## Expected screen

```text
FREERTOS / UNO                 LAB-04

SCHEDULER   RUNNING
HMI TASK    PRIORITY 2
WORKER      PRIORITY 1   BEAT ...
RTOS TICK   ...
TOUCH       UP / DOWN
COORD       X=... Y=...
```

The built-in LED should toggle independently of touch interaction.

## PASS criteria

TEST-01 passes when all of the following are observed on the physical board:

1. the FreeRTOS scheduler starts;
2. the HMI screen remains responsive;
3. the worker heartbeat continuously increases;
4. LED_BUILTIN toggles;
5. touch DOWN/UP state is detected;
6. touch coordinates remain consistent with LAB-03 calibration;
7. no spontaneous reset, hang, corrupted TFT output or touch/LCD pin conflict is observed.

Record the Arduino compiler Flash/SRAM report before declaring PASS.

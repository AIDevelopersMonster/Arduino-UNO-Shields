# LAB-05 — KON-OS on Arduino UNO + MAR2406

## Goal

Build a deliberately small operating environment for the classic Arduino UNO
R3 / ATmega328P instead of trying to force a modern RTOS/filesystem/GUI stack
into 2 KB of SRAM.

Working name: **KON-OS**.

The design takes inspiration from early small computers: one shared stack,
static buffers, cooperative scheduling, a command shell, and external programs
or data on removable storage.

## Hardware

- Arduino UNO R3 / ATmega328P
- 16 MHz
- 32 KB Flash
- 2 KB SRAM
- MAR2406 2.4-inch TFT Touch Shield
- microSD: CS=D10, MOSI=D11, MISO=D12, SCK=D13

TEST-01 uses the microSD slot but deliberately leaves TFT/Touch inactive.

## Non-claims

KON-OS 0.1 is not POSIX, not a protected multitasking OS, and not a replacement
for FreeRTOS.

It is a purpose-built embedded operating environment whose design objective is
to make useful interactive software possible inside the ATmega328P memory
envelope.

## Core rules

- no FreeRTOS;
- no Arduino String;
- no malloc/new in KON-OS code;
- one shared AVR stack;
- fixed command buffers;
- cooperative tasks must return quickly;
- D13 is SPI SCK whenever SD is active.

## KON-OS 0.1 architecture

```text
+--------------------------------+
|          SERIAL SHELL          |
| HELP DIR TYPE WRITE MEM PS ... |
+--------------------------------+
|       COOPERATIVE KERNEL       |
| task table | timers | dispatch |
+--------------------------------+
|        DEVICE SERVICES         |
|        Serial | SPI | SD       |
+--------------------------------+
|       ATmega328P / UNO         |
+--------------------------------+
```

The first kernel has two scheduled tasks:

- SERIAL — command input and shell;
- CLOCK — low-cost kernel heartbeat/tick.

Both tasks share the ordinary AVR stack. No per-task stacks are allocated.

## TEST plan

### TEST-01 — Kernel + shell + SD

Firmware:

`sketches/01_KONOS_Shell_SD/01_KONOS_Shell_SD.ino`

Commands:

```text
HELP
INFO
MEM
UPTIME
PS
MOUNT
DIR [path]
TYPE <file>
WRITE <file> <text>
APPEND <file> <text>
DEL <file>
MKDIR <path>
RMDIR <path>
CLS
REBOOT
```

Status: **READY FOR BUILD / BENCH**.

### Planned TEST-02 — minimal display driver

Replace the large generic TFT libraries with only the ILI9341 operations needed
by KON-OS.

Target operations:

- init;
- fill;
- pixel/line/rect;
- fixed small font;
- status console.

### Planned TEST-03 — touch shell / file browser

Add the already verified MAR2406 resistive touch wiring and a simple file
browser.

### Planned TEST-04 — executable content

Introduce an interpreted application format / bytecode stored on SD so programs
can be added without reflashing the UNO.

## Why LAB-05 follows LAB-04

LAB-04 established the useful boundary:

- FreeRTOS + TFT + Touch: PASS;
- FreeRTOS + SD + practical filesystem operations: not reliable in the available
  SRAM;
- FreeRTOS + SD + full graphics stack: too large for Flash.

LAB-05 changes the architecture instead of continuing to optimize the failed
stack.

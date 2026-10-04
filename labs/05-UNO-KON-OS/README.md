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

## What makes KON-OS different from an ordinary Arduino program

An ordinary Arduino sketch is normally one fixed application:

```text
reset
  |
setup()
  |
loop()
  |
one compiled application
```

Its application logic, hardware access and user interface are compiled together.
To replace the application, the UNO is normally rebuilt and reflashed.

KON-OS is being designed around a different boundary:

```text
reset
  |
resident KON-OS kernel
  |
+------------------------------+
| cooperative scheduler        |
| system shell                 |
| device services              |
| filesystem services          |
| application interface        |
+------------------------------+
               |
      external programs/data
               |
              SD
```

The distinction is not simply that KON-OS has a command prompt or a scheduler.
An ordinary application can have both. The intended OS property is that a
**resident kernel owns the machine and provides reusable services to software
that is separate from the kernel itself**.

The project therefore uses these criteria:

1. **Resident kernel**
   - the kernel remains in Flash;
   - launching another application must not replace the kernel.

2. **Resource ownership**
   - Serial, SD, TFT, Touch and timers belong to kernel/device services;
   - applications use those services instead of independently taking over the
     hardware.

3. **Scheduling**
   - multiple services or applications can make progress through the kernel's
     cooperative dispatcher;
   - they share the normal AVR stack, avoiding one large stack per task.

4. **System services**
   - console, timing, filesystem and later graphics/input are exposed through a
     stable KON-OS interface.

5. **Application lifecycle**
   - the target is LOAD/RUN/STOP/RETURN without reflashing the UNO;
   - after an application exits, control returns to the KON-OS shell or launcher.

6. **External executable content**
   - applications are planned to live on SD in an interpreted/bytecode format;
   - adding a new application should eventually mean copying a file to SD,
     not rebuilding the kernel.

7. **System shell**
   - commands such as DIR, TYPE, MEM, PS and later RUN operate on system
     resources rather than on one hard-coded application state.

### Ordinary sketch vs KON-OS target

| Property | Ordinary Arduino sketch | KON-OS target |
| --- | --- | --- |
| Main role | One application | Persistent operating environment |
| Change application | Recompile and reflash | Select/load from SD |
| Hardware ownership | Application code | Kernel/device services |
| Scheduling | Application-specific loop | Kernel cooperative dispatcher |
| Filesystem | Optional application feature | System service |
| Shell | Optional UI | System-control interface |
| Code location | AVR code in Flash | Kernel in Flash + apps/data on SD |
| App exit | Usually not defined | Return to shell/launcher |
| Memory policy | Application-specific | Fixed/static kernel budget |

### Current status of the OS claim

KON-OS 0.1 should be described precisely as a **small operating environment /
kernel prototype**, not yet as a complete general-purpose operating system.

TEST-01 can demonstrate the resident kernel, cooperative scheduler, shell and
filesystem services. The stronger distinction from an ordinary monitor program
will arrive when TEST-04 introduces an external application format that can be
loaded from SD, run through kernel services, terminated, and returned to the
shell without reflashing the ATmega328P.

That application boundary is the key milestone. Once the same resident kernel
can execute different external programs through a stable system interface,
KON-OS is no longer merely one large Arduino application with a menu.

## Non-claims

KON-OS 0.1 is not POSIX, not a protected multitasking OS, and not a replacement
for FreeRTOS.

It has:

- no memory protection;
- no user/kernel CPU privilege separation;
- no virtual memory;
- no process isolation;
- no requirement for preemptive multitasking.

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

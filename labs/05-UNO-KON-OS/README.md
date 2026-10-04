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

Verified build:

```text
Flash: 16930 / 32256 bytes (52%)
SRAM globals: 1033 / 2048 bytes (50%)
Linker-reported SRAM remaining: 1015 bytes
```

Status: **PASS ON PHYSICAL HARDWARE**.

Verified runtime:

```text
Boot free RAM: 1009 B
Steady shell free RAM: 834 B
Tasks:
  SERIAL  period 1 ms
  CLOCK   period 100 ms
```

Physically verified commands:

```text
HELP
INFO
MEM
PS
UPTIME
DIR /
LS /
WRITE
TYPE
APPEND
MKDIR
RMDIR
DEL
```

The complete create/read/append/list/directory/delete cycle passed on the
microSD card without reset or corruption.

Unlike the FreeRTOS SD experiment, KON-OS does not allocate a separate stack for each cooperative task. The linker-reported SRAM remainder is still not the same as final runtime free memory, but the runtime model has materially less stack overhead.

### TEST-02 — KonSol 0.2 minimal direct TFT driver

Firmware:

`sketches/02_KonSol_Minimal_TFT/02_KonSol_Minimal_TFT.ino`

Procedure:

`tests/TEST-02-MINIMAL-DIRECT-TFT.md`

The first display implementation removes MCUFRIEND_kbv and Adafruit_GFX and
drives the verified ILI9341 8-bit parallel bus directly.

Implemented system-display primitives:

- hardware reset and ILI9341 init;
- RGB565 fill;
- pixel;
- horizontal line;
- rectangle;
- tiny 3x5 PROGMEM font;
- KonSol system dashboard;
- periodic DISPLAY cooperative task;
- TFT shell command for display reinitialization.

Verified build:

```text
Flash: 20264 / 32256 bytes (62%)
SRAM globals: 1059 / 2048 bytes (51%)
Linker-reported SRAM remaining: 989 bytes
```

Delta from KonSol 0.1:

```text
+3334 B Flash
+26 B global SRAM
```

Physical result:

```text
Boot free RAM: 981 B
Steady shell free RAM: 806 B
SD: READY
DIR /: PASS
TFT reinitialization: PASS
DISPLAY task: RUNNING
```

The direct 320x240 ILI9341 dashboard was visually verified on the physical
MAR2406 shield. Compared with KonSol 0.1, the complete display subsystem adds
only about 28 bytes of observed runtime SRAM overhead.

Status: **PASS ON PHYSICAL HARDWARE**.

### TEST-03 — KonSol 0.3 direct Touch + file browser

Firmware:

`sketches/03_KonSol_Touch_File_Browser/03_KonSol_Touch_File_Browser.ino`

Procedure:

`tests/TEST-03-DIRECT-TOUCH-FILE-BROWSER.md`

TEST-03 removes the remaining TouchScreen-library dependency and adds a direct
resistive-touch system service plus an on-screen microSD browser.

New fourth cooperative task:

```text
3   TOUCH     30 ms
```

The TFT UI can open the SD browser, page through directory entries, enter
directories, return to the parent directory and display a one-page ASCII text
preview.

Verified build:

```text
Flash: 23666 / 32256 bytes (73%)
SRAM globals: 1155 / 2048 bytes (56%)
Linker-reported SRAM remaining: 893 bytes
```

Delta from KonSol 0.2:

```text
+3402 B Flash
+96 B global SRAM
```

Physical bench result:

```text
Boot free RAM: 885 B
Shell INFO/MEM free RAM: 823 B
Tasks: SERIAL / CLOCK / DISPLAY / TOUCH
SD root: T07LOG.TXT, XOLOG.TXT
Watchdog REBOOT: PASS
Touch FILES: PASS
File browser: PASS
XOLOG.TXT open/view: PASS
Return to dashboard: PASS
```

The photographed hardware run confirms that KonSol 0.3 can operate the direct
ILI9341 display, direct resistive Touch service, cooperative kernel, Serial shell
and microSD filesystem together on the ATmega328P.

Status: **FULL PHYSICAL PASS**.

Extended TEST-03 dataset was also created successfully:

```text
/TEST03
  F01.TXT ... F07.TXT
  SUB/
```

After approximately 2996 DISPLAY runs and 99898 TOUCH task runs, Serial `MEM`
still reported 823 B free. The remaining UI gate was then physically verified: NEXT/PREV, SUB/INNER.TXT, BACK, UP and DASH all passed.

Extended browser certification was completed on physical hardware using a
temporary TEST03 directory containing seven files and a SUB directory.

Verified UI sequence:

```text
FILES → TEST03 → NEXT → PREV → SUB → INNER.TXT → BACK → UP → DASH
```

All navigation steps passed without reset, display corruption or filesystem
corruption. Serial MEM remained at 823 B after the extended run.

### TEST-04 — KonSol 0.4 external KAP1 application VM

Firmware:

`sketches/04_KonSol_External_APP_VM/04_KonSol_External_APP_VM.ino`

Procedure:

`tests/TEST-04-EXTERNAL-KAP1-APP.md`

KonSol 0.4 introduces the first external application boundary. A `.KAP`
program remains on microSD and is interpreted by a fifth cooperative APP task.
The complete application is streamed from SD rather than copied into SRAM.

Initial KAP1 services:

- screen clear;
- text drawing through the resident direct ILI9341 driver;
- cooperative WAIT;
- WAIT_TOUCH through the resident Touch service;
- Serial output;
- EXIT back to the KonSol dashboard.

Shell:

```text
RUN <file.KAP>
APP
```

The touch file browser also recognizes `.KAP` files and launches them instead
of opening them in the text viewer.

Status: **READY FOR BUILD / BENCH**.

## Publication

KonSol 0.1 has been published as a software record on Zenodo.

- DOI: https://doi.org/10.5281/zenodo.23139756
- Video demonstration: https://www.youtube.com/watch?v=N3PEQUbUmPM

The Zenodo record corresponds to the physically verified KonSol 0.1 state:
cooperative kernel, Serial shell, microSD filesystem operations, memory
diagnostics and the TEST-01 hardware results documented in this laboratory.

## Documentation

KON-OS 0.1 documentation is split by role:

- [System description](docs/DESCRIPTION.md) — purpose, architecture, verified
  resource usage, OS boundary, current limitations and roadmap.
- [System programmer guide](docs/SYSTEM_PROGRAMMER_GUIDE.md) — kernel,
  cooperative scheduler, task table, memory rules, command parser, SD service
  rules and system-extension procedure.
- [Functional programmer guide](docs/FUNCTIONAL_PROGRAMMER_GUIDE.md) —
  adding commands and cooperative services without changing the kernel model;
  includes rules for state machines and the future application ABI.
- [User guide](docs/USER_GUIDE.md) — boot, terminal use, filesystem workflow,
  practical examples and error handling.
- [Command reference](docs/COMMAND_REFERENCE.md) — exact shell syntax, aliases,
  responses and error codes.

## Native SD boot mode: KON-Boot

A second execution model is technically possible in addition to interpreted KON-OS applications.

The ATmega328P can self-program its Flash. A small loader placed in the AVR Boot Loader Section can read an image from microSD, program the Application Flash page by page, verify it, and then start the newly installed program.

This is **not direct execution from SD**. The sequence is:

```text
microSD
  |
  | APP.BIN
  v
KON-Boot
  |
  | erase/write Flash pages
  v
ATmega328P Application Flash
  |
 reset / jump
  v
native AVR application
```

The Boot Loader Section size on ATmega328P is selected by the BOOTSZ fuses. The available configurations are 256, 512, 1024 or 2048 words, i.e. 512 B, 1 KB, 2 KB or 4 KB. A practical SD-aware loader may therefore require a larger boot section than the very small conventional UNO bootloader.

A native image format should not be just an anonymous byte stream. KON-Boot should use a small header, for example:

```text
KON1
TARGET=ATMEGA328P
LOAD=0x0000
SIZE=...
CRC32=...
VERSION=...
<raw AVR image>
```

Minimum safety rules:

- verify target MCU before erase;
- verify image length;
- verify CRC before marking the image bootable;
- never overwrite the protected boot loader;
- retain a recovery path if programming is interrupted;
- only jump to the application after full verification.

### Two complementary application models

KON-OS can therefore support two different kinds of software:

| Mode | Storage/execution | Advantage | Cost |
| --- | --- | --- | --- |
| KON-OS bytecode/app | stays on SD, interpreted by resident kernel | instant switching, no Flash wear, kernel stays resident | slower, limited by VM/API |
| KON-Boot native .BIN | copied from SD into AVR Application Flash | full native AVR speed and almost all MCU features | rewrites Flash, reboot required |

The hybrid model is especially attractive:

```text
Boot section:       KON-Boot / recovery
Application Flash: KON-OS kernel OR selected native image
microSD:           OS images + native apps + bytecode apps + data
```

KON-Boot can eventually make the UNO behave like a very small SD-based multi-boot computer: select an image, install it into Application Flash, run it, and return to the boot/recovery environment for another image.

This is distinct from TEST-04 bytecode execution. Both paths are worth keeping: bytecode for resident-OS applications, native .BIN loading for maximum performance.

## Why LAB-05 follows LAB-04

LAB-04 established the useful boundary:

- FreeRTOS + TFT + Touch: PASS;
- FreeRTOS + SD + practical filesystem operations: not reliable in the available
  SRAM;
- FreeRTOS + SD + full graphics stack: too large for Flash.

LAB-05 changes the architecture instead of continuing to optimize the failed
stack.

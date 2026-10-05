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
   - KonSol 0.4 physically verifies RUN/WAIT/EXIT/RETURN without reflashing the UNO;
   - after an application exits, control returns to the resident KonSol environment.

6. **External executable content**
   - KAP1 applications live on SD in a streamed interpreted bytecode format;
   - adding or changing a KAP1 application does not require rebuilding the kernel.

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

KonSol 0.4 should be described precisely as a **resident cooperative operating
environment with separately stored external applications**, not as a complete
general-purpose operating system.

TEST-04 has now physically verified the key application boundary that earlier
versions treated as the target milestone: the resident kernel remains in Flash,
a KAP1 application remains on microSD, the application is launched without
reflashing the ATmega328P, uses resident TFT/Touch/Serial services, waits for a
Touch event, exits, and returns control to KonSol.

This is the first strong OS-boundary result of the project. It is narrower than
a modern protected OS claim, but it is stronger than a single monolithic Arduino
application with a menu because the resident system and external application are
separate storage and lifecycle objects.

## Non-claims

KonSol 0.4 is not POSIX, not a protected multitasking OS, and not a replacement
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

Verified build:

```text
Flash: 25720 / 32256 bytes (79%)
SRAM globals: 1228 / 2048 bytes (59%)
Linker-reported SRAM remaining: 820 bytes
```

Delta from KonSol 0.3:

```text
+2054 B Flash
+73 B global SRAM
```

Status: **FULL PHYSICAL PASS**.

KonSol 0.4 Serial bench:

```text
Boot free RAM: 812 B
Shell free RAM: 750 B
Tasks: 5
HELLO.KAP created on SD from KonSol shell
RUN /HELLO.KAP: PASS
Serial service from app: PASS
APP EXIT 0: PASS
Repeated RUN/EXIT: PASS
```

The external application lifecycle is operational from the shell and Touch
browser. Final physical TFT evidence confirmed the external SD application
rendering `HELLO FROM SD` / `TOUCH TO EXIT` through the resident display
service, followed by the already verified Touch/EXIT return path.

KonSol 0.4 therefore crosses the first strong OS boundary: the resident kernel
remains in Flash while the application is a separate file on microSD and can be
launched, interacted with and exited without reflashing the UNO.

## KAP1 application model — how `HELLO.KAP` is created and executed

`HELLO.KAP` is not an Arduino sketch and it is not AVR machine code. It is a
separate external **KAP1 bytecode application** stored on microSD and
interpreted by the resident KonSol VM.

The verified example is:

```text
4B415031
1000
110A3C03030D48454C4C4F2046524F4D205344
110A7802030D544F55434820544F2045584954
300A48454C4C4F204B415031
21
FF
```

KAP1 deliberately uses ASCII hexadecimal storage. That makes the file readable,
easy to inspect with `TYPE`, and simple enough to create from the KonSol shell
itself without a separate compiler on the PC.

### Header and opcodes

Every KAP1 application begins with:

```text
4B 41 50 31
 K  A  P  1
```

or, without spaces:

```text
4B415031
```

KonSol validates this decoded signature before execution. A missing or invalid
header produces `ERR APP_HEADER`.

The initial KAP1 instruction set in KonSol 0.4 is:

| Opcode | Meaning |
| --- | --- |
| `10 cc` | clear TFT using color index `cc` |
| `11 xx yy ss cc nn <data>` | draw `nn` text bytes on TFT |
| `20 ll hh` | cooperative WAIT in milliseconds, little-endian |
| `21` | WAIT_TOUCH |
| `30 nn <data>` | write `nn` bytes to Serial |
| `FF` | EXIT back to KonSol |

Color indices are:

```text
0 BLACK
1 WHITE
2 CYAN
3 YELLOW
4 GREEN
5 RED
6 BLUE
7 GREY
```

### Decoding the verified `HELLO.KAP`

The first instruction is:

```text
1000
```

which decodes as:

```text
10 00
|  |
|  +-- color 0 = BLACK
+----- CLS
```

The first TFT text instruction is:

```text
110A3C03030D48454C4C4F2046524F4D205344
```

decoded as:

```text
11       TEXT
0A       xUnit = 10
3C       Y = 60
03       scale = 3
03       color = YELLOW
0D       length = 13

48 45 4C 4C 4F 20 46 52 4F 4D 20 53 44
 H  E  L  L  O     F  R  O  M     S  D
```

KAP1 stores X as `xUnit`; the VM calculates:

```text
X = xUnit * 2
0A = 10 -> X = 20 pixels
```

The display therefore receives approximately:

```text
HELLO FROM SD
X=20, Y=60, scale=3, YELLOW
```

The next instruction:

```text
110A7802030D544F55434820544F2045584954
```

renders:

```text
TOUCH TO EXIT
X=20, Y=120, scale=2, YELLOW
```

The Serial instruction:

```text
300A48454C4C4F204B415031
```

decodes to:

```text
30       SERIAL
0A       10 characters
48 45 4C 4C 4F 20 4B 41 50 31
 H  E  L  L  O     K  A  P  1
```

and produces:

```text
HELLO KAP1
```

Finally:

```text
21    WAIT_TOUCH
FF    EXIT
```

`WAIT_TOUCH` is cooperative: the application does not busy-loop waiting for
the screen. It changes VM state and returns control to the kernel dispatcher.

### Creating the application from KonSol itself

The complete example can be created directly in the resident shell:

```text
A:/> WRITE /HELLO.KAP 4B415031
A:/> APPEND /HELLO.KAP 1000
A:/> APPEND /HELLO.KAP 110A3C03030D48454C4C4F2046524F4D205344
A:/> APPEND /HELLO.KAP 110A7802030D544F55434820544F2045584954
A:/> APPEND /HELLO.KAP 300A48454C4C4F204B415031
A:/> APPEND /HELLO.KAP 21
A:/> APPEND /HELLO.KAP FF
```

The decoder ignores whitespace between hexadecimal bytes:

```text
space
TAB
CR
LF
```

so the program can remain line-oriented and human-readable.

It can then be inspected:

```text
A:/> TYPE /HELLO.KAP
-----
4B415031
1000
110A3C03030D48454C4C4F2046524F4D205344
110A7802030D544F55434820544F2045584954
300A48454C4C4F204B415031
21
FF
-----
```

and launched without rebuilding or reflashing the UNO:

```text
A:/> RUN /HELLO.KAP
```

### What happens after `RUN`

The shell path is:

```text
RUN /HELLO.KAP
      |
      v
cmdRun()
      |
      v
check .KAP extension
      |
      v
appStart("/HELLO.KAP")
      |
      v
SD.open()
      |
      v
validate KAP1 header
      |
      v
appRunning = true
uiMode = UI_APP
      |
      v
taskApp()
```

The complete application is **not copied into the ATmega328P's 2 KB SRAM**.
KonSol keeps the file open and decodes it incrementally:

```text
microSD
   |
   v
vmReadNibble()
   |
   v
vmReadByte()
   |
   v
opcode
   |
   v
taskApp()
```

In practical terms:

```text
SD -> decode -> execute -> SD -> decode -> execute ...
```

This streaming model is one of the reasons external applications are practical
inside the UNO memory envelope.

### Kernel scheduling while an application runs

In KonSol 0.4, APP is the fifth cooperative task:

```text
0  SERIAL    1 ms
1  CLOCK     100 ms
2  DISPLAY   1000 ms
3  TOUCH     30 ms
4  APP       10 ms
```

`taskApp()` normally interprets at most one VM instruction per dispatch. The
resident kernel therefore continues to schedule its services while the
application is active.

When the VM reaches opcode `21`:

```cpp
appWaitTouch = true;
```

the APP task returns to the scheduler. The 30 ms TOUCH task detects a new press
and, while an application is running, records the event:

```cpp
if (appRunning)
    appTouchEvent = true;
```

A later APP dispatch consumes that event, resumes the KAP1 stream and reaches
`FF`. EXIT closes the KAP file, clears application state, reports
`APP EXIT 0`, redraws the resident dashboard and returns control to KonSol.

The verified lifecycle is therefore:

```text
             Arduino UNO Flash
        +-------------------------+
        |       KonSol 0.4        |
        | Kernel                  |
        | Scheduler               |
        | Shell                   |
        | TFT service             |
        | Touch service           |
        | SD service              |
        | KAP1 VM                 |
        +------------+------------+
                     |
                     | RUN
                     v
               microSD card
        +-------------------------+
        |       HELLO.KAP         |
        | CLS                     |
        | TEXT "HELLO FROM SD"    |
        | TEXT "TOUCH TO EXIT"    |
        | SERIAL "HELLO KAP1"     |
        | WAIT_TOUCH              |
        | EXIT                    |
        +-------------------------+
```

This is the key KonSol 0.4 result:

```text
separate application file
        |
        v
RUN
        |
        v
execution through resident services
        |
        v
EXIT
        |
        v
return to resident KonSol
```

No new Arduino sketch is compiled and the ATmega328P is not reflashed when
`HELLO.KAP` is created, changed or executed.

### TEST-05 — KAP1 multi-application certification

TEST-05 strengthens the external-application result without changing the
published KonSol 0.4 resident firmware.

Applications:

- [HELLO.KAP](apps/KAP1/HELLO.KAP) — verified first external application;
- [ABOUT.KAP](apps/KAP1/ABOUT.KAP) — independent multi-line system/about app;
- [DEMO.KAP](apps/KAP1/DEMO.KAP) — timed KAP1 demo using cooperative WAIT.

Application directory:

[apps/KAP1/](apps/KAP1/)

Test procedure:

[tests/TEST-05-KAP1-MULTI-APP.md](tests/TEST-05-KAP1-MULTI-APP.md)

The certification target is intentionally stronger than TEST-04:

```text
                 HELLO.KAP
                    |
                 ABOUT.KAP
                    |
resident KonSol ----+---- DEMO.KAP
                    |
             same kernel/services
                    |
                  EXIT
                    |
             return to KonSol
```

No Arduino firmware rebuild or upload is permitted between application runs.
All applications must remain separate files on microSD and execute through the
same resident KonSol 0.4 kernel and KAP1 VM.

Physical progress:

```text
HELLO.KAP  FULL PHYSICAL PASS (TEST-04)
ABOUT.KAP  RUN / TFT / SERIAL PASS
DEMO.KAP   RUN / SERIAL / repeated APP EXIT 0 PASS
```

DEMO.KAP was created entirely through the resident KonSol shell, read back from
microSD, and executed twice by the unchanged KonSol 0.4 firmware. Both runs
produced `DEMO KAP1` and `APP EXIT 0`, confirming a third independent
bytecode stream and repeatable external-application lifecycle.

The physical DEMO sequence was confirmed as:

```text
KAP1 DEMO
  -> ~1500 ms
PROGRAM ON SD
  -> ~1500 ms
TOUCH TO EXIT
```

The second DEMO launch was explicitly confirmed as coming from the TFT Touch
File Browser, not from the Serial shell.

Final physical state:

```text
APP: IDLE
LAST EXIT: 0
FREE RAM: 750 B

HELLO.KAP  123 B
ABOUT.KAP  197 B
DEMO.KAP   171 B
```

The final `APP` / `MEM` / `DIR /` check was repeated with the same result.
ABOUT.KAP also produced `ABOUT KAP1` and completed with `APP EXIT 0`.

Status: **TEST-05 FULL PHYSICAL PASS**.

This certifies that one unchanged resident KonSol 0.4 firmware can execute
multiple independently stored KAP1 applications, including shell and TFT Touch
File Browser launch paths, cooperative WAIT, WAIT_TOUCH, resident display and
Serial services, and clean EXIT back to KonSol without reflashing.

### TEST-06 — KonSol 0.5 / KAP2 interactive control flow

KonSol 0.5 is the next experimental branch after the physically certified
KonSol 0.4 / TEST-05 baseline.

KAP2 keeps KAP1 compatibility and adds the first compact VM control layer:

```text
4 x 16-bit registers: R0..R3
MOVI
INC / DEC
CMPI
MARK
JNZ / JZ
GET_TOUCH_X
GET_TOUCH_Y
DRAW_REG
```

The first application is:

[apps/KAP2/COUNTER.KAP](apps/KAP2/COUNTER.KAP)

It counts five Touch events, captures the last X/Y coordinates, redraws its own
state, branches back through the bytecode loop, then displays DONE and exits
back to the resident system.

Firmware:

[sketches/05_KonSol_KAP2_Interactive/05_KonSol_KAP2_Interactive.ino](sketches/05_KonSol_KAP2_Interactive/05_KonSol_KAP2_Interactive.ino)

Specification:

[docs/KAP2_SPEC.md](docs/KAP2_SPEC.md)

Physical procedure:

[tests/TEST-06-KAP2-INTERACTIVE.md](tests/TEST-06-KAP2-INTERACTIVE.md)

Important compatibility rule:

```text
4B415031 = KAP1
4B415032 = KAP2
```

KonSol 0.5 must continue to run the physically certified KAP1 applications while
adding KAP2 state and control flow.

Verified build:

```text
Flash: 26798 / 32256 bytes (83%)
SRAM globals: 1239 / 2048 bytes (60%)
Linker-reported SRAM remaining: 809 bytes
```

Delta from KonSol 0.4:

```text
+1078 B Flash
+11 B global SRAM
```

Compile-time Flash headroom remains 5458 bytes.

Physical boot result:

```text
SD: READY
Boot free RAM: 801 B
Steady shell free RAM: 737 B
Tasks: 5
TFT: PASS
Touch service: running
File browser / existing SD files: usable
```

Compared with KonSol 0.4 runtime:

```text
boot:  812 B -> 801 B  (-11 B)
shell: 750 B -> 737 B  (-13 B)
```

The new KAP2 control layer therefore remains inside the UNO SRAM envelope.

KAP1 compatibility was physically rechecked on KonSol 0.5:

```text
APP RUN /HELLO.KAP
HELLO KAP1
APP EXIT 0
```

COUNTER.KAP was then created from the resident shell, read back from microSD,
accepted as KAP2 and launched:

```text
APP RUN /COUNTER.KAP
COUNTER KAP2
APP EXIT 0
```

The operator confirmed that five counted Touch presses were physically
performed during the recorded test. The program then completed with
`APP EXIT 0`. Per-touch Serial register dumps are treated as optional
diagnostics, not as a required runtime feature.

Status: **TEST-06 FULL PHYSICAL PASS**.

This establishes the first external KAP2 program with persistent VM state,
Touch-driven iteration and conditional control flow on the physical UNO.

### TEST-07 — KASM readable source assembler

After TEST-06, the main usability bottleneck is manual hexadecimal authoring of
KAP applications. TEST-07 moves that work to a host-side assembler without
changing KonSol 0.5 or consuming additional UNO Flash/SRAM.

Added:

- [tools/kasm/kasm.py](../../tools/kasm/kasm.py) — dependency-free Python KAP1/KAP2 assembler;
- [tools/kasm/README.md](../../tools/kasm/README.md) — syntax and CLI guide;
- [apps/KAP2/COUNTER.kasm](apps/KAP2/COUNTER.kasm) — readable source for the
  physical COUNTER application;
- [tests/TEST-07-KASM-ASSEMBLER.md](tests/TEST-07-KASM-ASSEMBLER.md) —
  reproducibility procedure.

The intended development path is now:

```text
human-readable .kasm
        |
        v
host-side KASM assembler
        |
        v
ASCII-hex .KAP
        |
        v
microSD
        |
        v
resident KonSol VM
```

The assembler was then tested in the project Windows/PowerShell environment:

```text
KASM PASS: KAP2 -> COUNTER.generated.KAP
Instructions/records: 24
```

PowerShell comparison of the generated output against the physically certified
COUNTER.KAP reference returned:

```text
True
```

Because the generated normalized ASCII-hex stream is identical to the
COUNTER.KAP already exercised on physical KonSol 0.5 in TEST-06, a duplicate
hardware execution is not required for the assembler reproducibility claim.

Status: **TEST-07 FULL PASS**.

The application-development path is now physically anchored at both ends:

```text
readable .kasm
   -> KASM
   -> exact KAP2 bytecode
   -> previously certified KonSol 0.5 execution
```

### TEST-08 — KonSol 0.6 / KAP2 multi-label control flow

KonSol 0.5 proved state, Touch input and one MARK-based loop. TEST-08 removes
that single-target limitation while keeping old KAP2 bytecode valid.

KonSol 0.6 adds:

```text
up to 8 indexed labels
LABEL
JMP
JZ label
JNZ label
forward and backward named branches in KASM
```

KASM resolves human-readable names to compact label IDs. The resident VM scans
the external KAP2 file once at launch, indexes label positions, rewinds, and
then branches by direct SD seek.

Reference application:

[apps/KAP2/MULTILABEL.kasm](apps/KAP2/MULTILABEL.kasm)

Physical procedure:

[tests/TEST-08-KAP2-MULTILABEL.md](tests/TEST-08-KAP2-MULTILABEL.md)

Verified build:

```text
Flash: 27568 / 32256 bytes (85%)
SRAM globals: 1256 / 2048 bytes (61%)
Linker-reported SRAM remaining: 792 bytes
```

Delta from KonSol 0.5:

```text
+770 B Flash
+17 B global SRAM
```

The SRAM increase exactly matches the planned 17-byte persistent label table.

Status: **FULL PHYSICAL PASS**.

Physical TFT evidence confirms the complete named-branch display sequence:

```text
RED -> YELLOW -> GREEN
```

with STEP 1/2/3 reached by successive Touch events. The final Touch then returned
`APP EXIT 0`, and the resident shell reported `APP: IDLE`, `LAST EXIT: 0`
and 720 B free RAM. The SD root remained readable.

The legacy KonSol 0.5 `COUNTER.KAP` also ran unchanged under KonSol 0.6 and
exited with `APP EXIT 0`. A second post-run `MEM` still reported 720 B, all
five cooperative tasks remained active, and `DIR /` succeeded again.

TEST-08 therefore closes as FULL PHYSICAL PASS with both the new multi-label
control-flow path and backward compatibility verified on the physical UNO.

## Publication

KonSol 0.6 TEST-08 is published as a **standalone technical article**, not as a
replacement/version update of the KonSol 0.5 record.

- Standalone KonSol 0.6 article DOI: https://doi.org/10.5281/zenodo.23161379
- KonSol 0.5 DOI: https://doi.org/10.5281/zenodo.23149141
- KonSol 0.5 video: https://youtu.be/HVHLsfV9dFY
- Previous KonSol 0.4 DOI: https://doi.org/10.5281/zenodo.23144922
- Historical KonSol 0.1 DOI: https://doi.org/10.5281/zenodo.23139756
- KonSol 0.1 video demonstration: https://www.youtube.com/watch?v=N3PEQUbUmPM

The standalone 0.6 article records the TEST-08 multi-label result: indexed
labels, LABEL/JMP/JZ/JNZ, KASM symbolic labels, the physically verified
RED -> YELLOW -> GREEN control-flow sequence, clean APP EXIT 0, stable 720 B
post-run shell free RAM, continued SD access and backward compatibility with the
legacy KonSol 0.5 COUNTER.KAP application.

KonSol 0.5 remains a separate publication for the first register-based KAP2
state/control-flow layer and KASM reproducibility result. KonSol 0.4 remains the
earlier KAP1 external-application baseline, and KonSol 0.1 remains the
historical kernel/Shell/microSD milestone.

## Documentation

The original KON-OS 0.1 role-based guides are retained as the historical kernel
baseline:

- [System description](docs/DESCRIPTION.md)
- [System programmer guide](docs/SYSTEM_PROGRAMMER_GUIDE.md)
- [Functional programmer guide](docs/FUNCTIONAL_PROGRAMMER_GUIDE.md)
- [User guide](docs/USER_GUIDE.md)
- [Command reference](docs/COMMAND_REFERENCE.md)

Current external-application documentation is maintained separately so the
historical 0.1 guides are not silently rewritten:

- [KAP2 specification](docs/KAP2_SPEC.md) — KAP1/KAP2 bytecode, registers,
  flags, legacy MARK control flow and the KonSol 0.6 indexed-label extension.
- [KAP2 input/output model](docs/KAP2_IO_MODEL.md) — current Touch/TFT/Serial
  services, microSD role, MAR2406 pin ownership, capabilities not yet exposed
  to KAP applications, and the separate KON-Boot/Flash-Writer line.
- [KASM guide](../../tools/kasm/README.md) — readable `.kasm` source to
  ASCII-hex `.KAP` bytecode.

This split is intentional: version-specific application interfaces evolve,
while the early 0.1 documents remain reproducible records of the original
kernel stage.

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

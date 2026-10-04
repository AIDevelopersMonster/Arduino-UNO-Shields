# LAB-05 / TEST-01 — KON-OS 0.1 Kernel + Serial Shell + SD

## Purpose

Prove that a cooperative, single-stack operating environment can provide:

- a scheduler;
- shell;
- memory diagnostics;
- SD mount;
- directory listing;
- text file read/write;
- filesystem management;

on the same ATmega328P where the FreeRTOS SD experiment ran out of practical
memory headroom.

## Build

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull

arduino-cli compile --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\01_KONOS_Shell_SD
```

Verified build:

```text
Sketch: 16930 / 32256 bytes Flash (52%)
Globals: 1033 / 2048 bytes SRAM (50%)
Linker-reported SRAM remaining: 1015 bytes
```

Build status: **PASS**.

The result is significant because the kernel, shell and SD filesystem fit while retaining roughly half of program Flash and without FreeRTOS per-task stacks.

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\01_KONOS_Shell_SD
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Expected boot:

```text
KON-OS 0.1
Arduino UNO / ATmega328P / 16 MHz
32 KB FLASH / 2 KB SRAM
Cooperative kernel + Serial shell + microSD

BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: ...
Type HELP
A:/>
```

## Test sequence

Run:

```text
HELP
INFO
MEM
PS
UPTIME
DIR /
```

Then create a file:

```text
WRITE /KONTEST.TXT HELLO FROM KON-OS
TYPE /KONTEST.TXT
DIR /
```

Append:

```text
APPEND /KONTEST.TXT SECOND LINE
TYPE /KONTEST.TXT
```

Directory test:

```text
MKDIR /KON
DIR /
RMDIR /KON
```

Delete the test file:

```text
DEL /KONTEST.TXT
DIR /
```

## PASS criteria

TEST-01 passes when:

1. kernel boots and shell prompt appears;
2. SD mounts successfully;
3. MEM reports plausible free SRAM;
4. PS shows SERIAL and CLOCK cooperative tasks;
5. task run counters increase;
6. DIR / returns real card contents;
7. WRITE creates a file;
8. TYPE reads the written text back correctly;
9. APPEND adds a second line;
10. DEL removes the file;
11. no reset or corruption occurs during repeated shell commands.

## Important comparison with LAB-04

A successful DIR / in this test is especially significant because the same
filesystem operation was the point at which the FreeRTOS-based TEST-02 became
unreliable.


## Physical result

TEST-01 was executed on the real Arduino UNO + MAR2406 shield and passed.

Observed boot:

```text
KON-OS 0.1
Arduino UNO / ATmega328P / 16 MHz
32 KB FLASH / 2 KB SRAM
Cooperative kernel + Serial shell + microSD

BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: 1009 B
Type HELP
A:/>
```

Observed runtime diagnostics:

```text
INFO
KON-OS 0.1
CPU: ATmega328P @ 16 MHz
FLASH: 32 KB
SRAM: 2 KB
SCHED: cooperative
TASKS: 2
SD: READY
FREE RAM: 834 B

PS
ID  TASK      PERIOD  RUNS
0   SERIAL    1 ms    102636
1   CLOCK     100 ms  1026

UPTIME
UPTIME: 117114 ms
KERNEL TICKS: 1171
```

Filesystem operations all passed:

- DIR / listed the real card contents;
- WRITE created KONTEST.TXT;
- TYPE read the file back;
- APPEND added a second line;
- MKDIR created /KON;
- RMDIR removed /KON;
- DEL removed KONTEST.TXT;
- repeated DIR calls continued to work;
- LS / and lowercase ls / were accepted as aliases and produced the directory listing.

The observed free-RAM value after entering normal shell operation was 834 B.
No reset or filesystem corruption was observed during the test sequence.

### TEST-01 status

**PASS on physical hardware.**

This is the key comparison with LAB-04:

```text
LAB-04 FreeRTOS + SD:
  SD init       PASS
  shell command PASS
  directory op  FAIL / unstable

LAB-05 KON-OS:
  SD init       PASS
  scheduler     PASS
  shell         PASS
  DIR           PASS
  WRITE         PASS
  TYPE          PASS
  APPEND        PASS
  MKDIR/RMDIR   PASS
  DEL           PASS
```

The result supports the design choice to use a cooperative single-stack kernel
for this 2 KB SRAM target.

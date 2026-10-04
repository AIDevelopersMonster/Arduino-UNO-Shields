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

Record both Flash and SRAM numbers.

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

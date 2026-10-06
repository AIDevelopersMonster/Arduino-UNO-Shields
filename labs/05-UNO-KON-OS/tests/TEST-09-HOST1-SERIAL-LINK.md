# LAB-05 / TEST-09 — KonSol 0.7 HOST1 Serial Link

Status: **PHYSICAL TEST IN PROGRESS**.

## Purpose

Verify the assembled KonSol device as a complete computer with these external
interfaces only:

- TFT output;
- resistive Touch input;
- microSD storage;
- USB-TTL Serial link to another computer.

TEST-09 adds the HOST1 machine protocol on the existing Serial link while
preserving the human shell and the KonSol 0.6 KAP1/KAP2 runtime.

## Firmware

`sketches/07_KonSol_Host_Link/07_KonSol_Host_Link.ino`

## 1. Build

From repository root:

```powershell
git pull
arduino-cli compile --fqbn arduino:avr:uno `
  .\labs\05-UNO-KON-OS\sketches\07_KonSol_Host_Link
```

Observed build on the physical TEST-09 bench:

```text
Flash: 30442 / 32256 B (94%)
Global SRAM: 1316 / 2048 B (64%)
Linker SRAM remainder: 732 B
```

Compared with KonSol 0.6:

```text
Flash:   27568 -> 30442 B   (+2874 B)
Globals:  1256 -> 1316 B      (+60 B)
```

## 2. Upload and boot

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno `
  .\labs\05-UNO-KON-OS\sketches\07_KonSol_Host_Link

arduino-cli monitor -p COM4 -c baudrate=115200
```

Observed boot:

```text
KonSol 0.7
SD: READY
FREE RAM: 724 B
```

## 3. Human-shell regression

Run:

```text
INFO
MEM
PS
DIR /
```

Observed:

```text
FREE RAM: 652 B
TASKS: 5
SERIAL / CLOCK / DISPLAY / TOUCH / APP active
DIR / readable
```

This verifies that HOST1 did not replace the ordinary KonSol shell.

## 4. HOST1 control plane

HOST1 commands begin with `@` and do not emit the human `A:/>` prompt.

Run:

```text
@PING
@INFO
@MEM
@PS
@LS /
```

Physically observed:

```text
@OK INFO V=0.7 HOST=1 SD=1 APP=0 RAM=652 TASKS=5
@OK MEM 652
@END PS 5
@END LS 8
```

The complete task and directory records are emitted as machine-readable
`@TASK`, `@F` and `@D` lines.

## 5. Reliable video path: one transfer script

Close Serial Monitor first so COM4 is free, then run the repository test script:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\konsol-host1\test09-transfer.ps1
```

The script performs the complete deterministic transfer test:

1. reads the local `MULTI.KAP`;
2. computes its exact byte size, CRC16 and SHA-256;
3. opens COM4 at 115200;
4. verifies HOST1 with `@PING`;
5. uploads the application as `/HOSTAPP.KAP`;
6. verifies the uploaded file with `@PUTE`;
7. downloads the same file with `@GET`;
8. writes `HOSTAPP.roundtrip.KAP` in the repository root;
9. compares source and returned SHA-256;
10. lists the SD root and leaves `/HOSTAPP.KAP` installed for the next RUN test.

On the current Windows checkout the expected tested values are:

```text
SIZE   = 524
CRC    = 85BD
SHA256 = DD2740D4C8F770839F01969B0C9BBACE4CD18386A2AD8464108B3FE0449FAF5A
```

The important successful ending is:

```text
BEGIN SIZE = 524
END SIZE   = 524
RX SIZE    = 524
END CRC    = 85BD
SRC SHA256 = DD2740D4C8F770839F01969B0C9BBACE4CD18386A2AD8464108B3FE0449FAF5A
RX  SHA256 = DD2740D4C8F770839F01969B0C9BBACE4CD18386A2AD8464108B3FE0449FAF5A
MATCH      = True

TEST-09 HOST1 TRANSFER ROUND-TRIP: PASS
```

The exact size/CRC may differ on another checkout if text line endings differ.
The script always verifies the exact local source bytes, so `MATCH = True` is
the decisive round-trip result.

## 6. What the transfer script certifies

The script certifies an exact bidirectional path:

```text
PC -> USB-TTL -> KonSol -> microSD
microSD -> KonSol -> USB-TTL -> PC
```

The returned file must be byte-for-byte identical to the local source according
to SHA-256.

## 7. Remote application launch

Open Serial Monitor again and run:

```text
@RUN /HOSTAPP.KAP
```

Observed:

```text
APP RUN /HOSTAPP.KAP
@OK RUN
MULTILABEL KAP2
```

On TFT/Touch complete:

```text
touch 1 -> RED
touch 2 -> YELLOW
touch 3 -> GREEN
touch 4 -> EXIT
```

Observed Serial completion:

```text
APP EXIT 0
```

## 8. Final post-run gate

Run:

```text
@APP
@MEM
@PS
@LS /
@DEL /HOSTAPP.KAP
@LS /
```

Required final evidence:

- APP is IDLE with exit code 0;
- free RAM remains stable near the 652 B shell value;
- all five tasks continue running;
- SD remains readable;
- HOSTAPP.KAP is present before DEL;
- DEL returns OK;
- HOSTAPP.KAP is absent after the final LS.

## PASS criteria

TEST-09 is FULL PHYSICAL PASS when all of the following are verified:

1. build and boot;
2. legacy human shell;
3. HOST1 INFO/MEM/PS/LS control plane;
4. PC-to-KonSol upload without removing microSD;
5. size + CRC verification;
6. exact GET round-trip with matching SHA-256;
7. remote RUN of the transferred KAP2 application;
8. normal TFT/Touch RED -> YELLOW -> GREEN execution;
9. APP EXIT 0 back to resident KonSol;
10. stable post-run RAM and five cooperative tasks;
11. readable filesystem after execution;
12. remote deletion of the transferred application.


## Video procedure — what we test and why

This sequence is intended to be read and executed directly during the video.

### Step 1 — Build KonSol 0.7

Command:

```powershell
arduino-cli compile --fqbn arduino:avr:uno `
  .\labs\05-UNO-KON-OS\sketches\07_KonSol_Host_Link
```

**What we test:** that the complete KonSol 0.7 firmware with HOST1 still fits
inside the ATmega328P Flash/SRAM limits and compiles cleanly.

**Why:** HOST1 adds a second machine-oriented interface to an already dense
firmware. Before any physical test, we must prove that the result still fits
inside the real Arduino UNO resource envelope.

Expected measured build:

```text
Flash: 30442 / 32256 B (94%)
Global SRAM: 1316 / 2048 B (64%)
```

### Step 2 — Upload and boot

Commands:

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno `
  .\labs\05-UNO-KON-OS\sketches\07_KonSol_Host_Link

arduino-cli monitor -p COM4 -c baudrate=115200
```

**What we test:** that KonSol 0.7 starts on the physical UNO, initializes TFT,
kernel and microSD, and reports runtime free RAM.

**Why:** successful compilation alone does not prove that the firmware is stable
on the real 2 KB SRAM device.

Expected boot reference:

```text
KonSol 0.7
SD: READY
FREE RAM: 724 B
```

### Step 3 — Verify the original human shell

Run:

```text
INFO
MEM
PS
DIR /
```

**What we test:** that the old interactive KonSol interface still works after
HOST1 was added.

**Why:** KonSol 0.7 must extend KonSol 0.6, not replace or break it. We verify
system information, runtime memory, all five cooperative tasks, and normal SD
filesystem access.

Expected reference:

```text
FREE RAM: 652 B
TASKS: 5
DIR / readable
```

### Step 4 — Verify the HOST1 control plane

Run:

```text
@PING
@INFO
@MEM
@PS
@LS /
```

**What we test:** that the same USB-TTL Serial link now also provides a
machine-readable protocol.

**Why:** the purpose of HOST1 is communication with another computer, not only
manual terminal use. The host must be able to identify the system, read memory
status, inspect scheduler tasks and enumerate files in a deterministic format.

Expected examples:

```text
@OK PONG HOST1
@OK INFO V=0.7 HOST=1 SD=1 APP=0 RAM=652 TASKS=5
@OK MEM 652
@END PS 5
@END LS ...
```

### HOST1 transfer reliability note

The TEST-09 script deliberately sends **20 data bytes per PUTD record** and uses
a single LF line terminator. This keeps the complete Serial command at about 60
bytes, below the classic ATmega328P HardwareSerial 64-byte receive buffer.

The earlier 24-byte chunk form could produce a roughly 69-byte CRLF-terminated
line. If that line arrived while a display task temporarily occupied the CPU,
the AVR UART receive buffer could overflow and the host would wait for a reply
that never arrived. TEST-09 therefore uses 20-byte chunks as part of the
protocol test procedure, not 24-byte chunks.

The host read timeout is 10 seconds to tolerate occasional SD-card write
latency. A failed run can simply be restarted: the script begins with PUTB,
which recreates the test destination before sending data.

### Step 5 — Install an application from the PC without removing microSD

Close Serial Monitor and run:

```powershell
powershell -ExecutionPolicy Bypass -File `
  .\tools\konsol-host1\test09-transfer.ps1
```

**What we test:** a complete PC -> USB-TTL -> KonSol -> microSD application
installation path.

**Why:** this is the main practical goal of TEST-09. A new KAP2 application
must be installable from another computer without reflashing the UNO and without
physically removing the microSD card.

The script automatically:

1. reads the local `MULTI.KAP`;
2. calculates its exact size, CRC16 and SHA-256;
3. uploads it as `/HOSTAPP.KAP`;
4. verifies the file on the device;
5. downloads it back;
6. compares the returned bytes against the original.

### Step 6 — Verify transfer integrity

The successful script ending must include:

```text
MATCH      = True
TEST-09 HOST1 TRANSFER ROUND-TRIP: PASS
```

**What we test:** exact byte preservation in both directions.

**Why:** a file appearing on the SD card is not enough. We must prove that no
byte was lost or altered during:

```text
PC -> USB-TTL -> KonSol -> microSD
microSD -> KonSol -> USB-TTL -> PC
```

CRC verifies the protocol transfer, while matching SHA-256 confirms that the
returned file is byte-for-byte identical to the original host file.

### Step 7 — Run the application that was installed through HOST1

Open Serial Monitor again:

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Run:

```text
@RUN /HOSTAPP.KAP
```

Then on the TFT:

```text
Touch 1 -> RED
Touch 2 -> YELLOW
Touch 3 -> GREEN
Touch 4 -> EXIT
```

**What we test:** that the transferred file is not merely stored correctly but
is a valid executable KAP2 application that the resident VM can launch.

**Why:** this closes the full application-delivery chain. The file came from the
PC, crossed HOST1, was stored on microSD, and is now executed by KonSol without
MCU reflashing.

Expected Serial completion:

```text
APP EXIT 0
```

### Step 8 — Verify system recovery after the application exits

Run:

```text
@APP
@MEM
@PS
@LS /
```

**What we test:** that the resident KonSol environment remains healthy after a
remotely installed application completes.

**Why:** a successful application run is insufficient if it damages the shell,
scheduler, filesystem or runtime memory. The OS boundary requires clean return
to the resident environment.

Required evidence:

- APP is IDLE with exit code 0;
- free RAM remains near the normal shell value;
- all five tasks continue running;
- microSD remains readable;
- `HOSTAPP.KAP` is still present.

### Step 9 — Remove the transferred application remotely

Run:

```text
@DEL /HOSTAPP.KAP
@LS /
```

**What we test:** remote lifecycle completion.

**Why:** HOST1 should not only install and run applications; the host must also
be able to remove an installed application without touching the SD card.

Expected:

```text
@OK DEL
```

and `HOSTAPP.KAP` must be absent from the final directory listing.

### TEST-09 meaning

If all steps pass, TEST-09 proves the complete external software-delivery path:

```text
PC
 -> USB-TTL
 -> HOST1
 -> microSD
 -> KAP2 VM
 -> TFT/Touch application
 -> APP EXIT 0
 -> resident KonSol
```

The key result is not simply Serial file transfer. It is that the assembled
KonSol device can receive, verify, execute and remove external applications from
another computer without reflashing the ATmega328P and without removing the
microSD card.

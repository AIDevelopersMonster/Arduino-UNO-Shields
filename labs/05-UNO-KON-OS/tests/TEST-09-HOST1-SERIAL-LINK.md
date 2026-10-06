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

For the video, do **not** type PUTD records manually.

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

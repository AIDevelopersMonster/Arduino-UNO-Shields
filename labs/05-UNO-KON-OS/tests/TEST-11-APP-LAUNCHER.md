# LAB-05 / TEST-11 — KonSol 0.8 Application Launcher

Status: **BUILD PASS / PHYSICAL TEST PENDING**.

## Purpose

TEST-11 moves application selection onto the assembled KonSol device itself.

KonSol 0.7 already proved that another computer can install, download, run,
stop and delete external KAP applications through HOST1 and the Host Manager.
TEST-11 adds an on-device Touch launcher while preserving the existing FILES
browser and HOST1 path.

Device boundary remains unchanged:

```text
Input:       resistive Touch + USB-TTL Serial
Output:      TFT + USB-TTL Serial
Storage:     microSD
Host link:   HOST1
Execution:   resident KonSol + external KAP1/KAP2
```

## Firmware

`sketches/08_KonSol_App_Launcher/08_KonSol_App_Launcher.ino`

## First implementation

Dashboard now exposes two separate Touch paths:

```text
APPS   -> KAP-only application launcher
FILES  -> existing general filesystem browser
```

The first launcher deliberately uses the existing microSD root and existing
KAP files. It does not yet require metadata sidecars such as `.INF`.

This keeps the first TEST-11 question narrow:

> Can the physical KonSol device enumerate and launch external applications by
> Touch without a PC and without breaking the already certified 0.7 runtime?

## Build gate

Observed Arduino UNO build:

```text
Flash:        30738 / 32256 B (95%)
Global SRAM:   1317 / 2048 B (64%)
Linker SRAM remainder: 731 B
```

KonSol 0.7 reference:

```text
Flash:        30442 / 32256 B
Global SRAM:   1316 / 2048 B
```

TEST-11 launcher cost:

```text
Flash:  +296 B
SRAM:     +1 B
```

Remaining Flash headroom:

```text
32256 - 30738 = 1518 B
```

The first launcher therefore fits inside the ATmega328P envelope with almost no
additional persistent SRAM cost.

## First physical test

Upload:

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno `
  .\labs\05-UNO-KON-OS\sketches\08_KonSol_App_Launcher
```

Monitor:

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

### Gate A — boot

Expected:

```text
KonSol 0.8
SD: READY
FREE RAM: ...
```

Record the actual boot and resident-shell RAM.

### Gate B — dashboard

The physical TFT must show both:

```text
APPS
FILES
```

### Gate C — KAP-only launcher

Touch `APPS`.

Expected title:

```text
KONSOL 0.8 APP LAUNCHER
KAP APPLICATIONS
```

Only `.KAP` files should be listed. Non-application root entries such as text
logs and directories must be absent.

### Gate D — Touch launch

Select a known application, preferably `HELLO.KAP` first.

Expected lifecycle:

```text
APPS
 -> HELLO.KAP
 -> KAP1 application screen
 -> WAIT_TOUCH
 -> APP EXIT 0
 -> resident KonSol dashboard
```

Then repeat with `MULTI.KAP` or another KAP2 application to verify the KAP2
path.

### Gate E — FILES regression

Return to dashboard and touch `FILES`.

The original general browser must still expose directories and non-KAP files.
This proves that APP LAUNCHER is an additional system interface rather than a
replacement for the filesystem browser.

### Gate F — HOST1 regression

From Serial run:

```text
@INFO
@MEM
@PS
@LS /
```

Required:

- version reports 0.8;
- SD remains ready;
- five cooperative tasks remain active;
- root filesystem remains readable;
- resident RAM remains stable after launcher use and APP EXIT.

## PASS criteria

TEST-11 first stage is FULL PHYSICAL PASS when:

1. 0.8 boots on the physical UNO;
2. APPS and FILES are both visible;
3. APPS shows only KAP applications;
4. Touch launches a KAP1 application;
5. APP EXIT 0 returns to resident KonSol;
6. Touch launches a KAP2 application;
7. FILES still behaves as the ordinary file browser;
8. HOST1 status/tasks/filesystem remain operational;
9. no reset, SD corruption or progressive RAM loss is observed.

## Next stage after first PASS

Only after the minimal launcher is physically certified should TEST-11 add
application metadata/catalog data, for example:

```text
APP.KAP
APP.INF
```

Possible metadata:

```ini
NAME=Touch Counter
VERSION=1.0
TYPE=KAP2
DESCRIPTION=Touch-driven counter
```

That is intentionally deferred so the first physical launcher test remains
small, reproducible and resource-measurable.

# KSC Quick Start - Arduino UNO + HY-M302

This guide reproduces the final physically certified HY-M302 target route
(KSC-04C) from the repository.

## What this build demonstrates

The final image provides:

- the unified KSC namespace: `/dev`, `/proc`, `/sys`, `/host`;
- ANSI Commander + shell over the same VFS;
- PC keyboard and HY-M302 IR input;
- remote PC directory mounting through KSC Host;
- bounded host-file streaming;
- File Viewer + File Actions;
- streamed KSC Script v0.1 execution;
- RAM-resident SW1 runtime profile selected by `SW1_0.KSC` / `SW1_1.KSC`.

Final certified build:

```text
Flash       31024 / 32256 B = 96%
Global SRAM  1556 / 2048 B = 75%
Observed Commander free RAM ~380 B
```

## Requirements

Tested workflow:

- Arduino UNO / ATmega328P;
- HY-M302 shield;
- Windows;
- Python 3 + pyserial;
- Arduino CLI;
- Arduino AVR core;
- repository checked out locally.

The recorded environment used Arduino CLI 1.5.2-rc.1 and `arduino:avr 1.8.8`.
Later compatible versions may work but are outside the exact recorded toolchain.

Install pyserial if required:

```powershell
python -m pip install pyserial
```

## 1. Compile

From the repository root:

```powershell
arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\12_KSC_04B_FileActionsLauncher `
  .\projects\KSC-KonSol-Commander\sketches\12_KSC_04B_FileActionsLauncher
```

Expected final size for the certified source snapshot:

```text
Sketch uses 31024 bytes (96%) of program storage space.
Global variables use 1556 bytes (75%) of dynamic memory.
```

## 2. Upload

Replace `COM4` when necessary:

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\12_KSC_04B_FileActionsLauncher
```

## 3. Start KSC Host

Close Arduino Serial Monitor first; one process must own the COM port.

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4
```

KSC Host exports the repository `host-share` directory as `/host`.

## 4. Open Commander

Use the normal KSC command path to enter Commander, then navigate to:

```text
/host/DEMOS
```

The final demo directory contains:

```text
ALARM.KSC
BEEP.KSC
POLICE.KSC
README.TXT
RESET.KSC
RGB.KSC
SHOW.KSC
SOS.KSC
SW1_0.KSC
SW1_1.KSC
TRAFFIC.KSC
```

For a `.KSC` file choose:

```text
ACTIONS
  VIEW
> RUN
```

A successful run ends with:

```text
RUN OK
```

## 5. Final SW1 runtime-profile test

Run `/host/DEMOS/SW1_0.KSC`.

Expected:

```text
SW1_0 LOAD RED PROFILE
PRESS SW1 FOR RED
RUN OK
```

Press physical SW1:

```text
RED ON
BLUE OFF
```

Without reset, recompilation, or upload, run
`/host/DEMOS/SW1_1.KSC`.

Press the same physical SW1:

```text
BLUE ON
RED OFF
```

This profile is RAM-resident and is lost after MCU reset.

## 6. Expected healthy status

Representative final status:

```text
RAM 380 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

For deliberately injected KSC-03D recovery tests, `R1/R2` can be expected;
those counters represent successful recoveries, not final host errors.

## 7. Large-file streaming

The repository includes `host-share/BIG.TXT`, approximately 12 KB.

The certified remote-file path uses repeated reads of at most 32 bytes and does
not require the entire file to fit in the ATmega328P's 2 KB SRAM.

See:

- `tests/KSC-03-HOST-FILESYSTEM-MOUNT.md`
- `tests/KSC-03D-RECOVERY-CERTIFICATION.md`
- `tests/KSC-04A-COMMANDER-FILE-VIEWER.md`

## 8. Fault injection

KSC Host 0.4 supports the two certified KSC-03D recovery routes:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --drop-read-at N
```

and:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --invalidate-read-at N
```

See the KSC-03D test record for the exact certification procedure.

## Publications

HY-M302 stage preprint:

https://doi.org/10.5281/zenodo.23251546

Earlier KSC_Core two-target publication:

https://doi.org/10.5281/zenodo.23232216

KSC-04 demonstration video:

https://youtu.be/pqV5DG1o-WA

## Scope

This is a reproducible guide for the completed Arduino UNO + HY-M302 target
phase. It does not imply that the wider KSC programme is closed. New targets
should use fresh target images instead of adding more features to the already
96%-Flash HY-M302 image.

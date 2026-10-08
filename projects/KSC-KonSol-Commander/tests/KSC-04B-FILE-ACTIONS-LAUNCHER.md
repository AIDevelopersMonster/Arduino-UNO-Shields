# KSC-04B - File Actions & Launcher

Status: **IMPLEMENTED - FIRST BUILD/PHYSICAL TEST PENDING**

## Goal

Move Commander from passive file viewing to file-oriented actions.

For ordinary remote text files, KSC keeps the KSC-04A VIEW behavior.

For a remote file whose name ends in `.KSC`, Commander opens an action menu:

```text
KSC SCRIPT
----------------------------------------
> VIEW
  RUN
----------------------------------------
UP/DOWN Select
ENTER/RIGHT Action
LEFT/BACK Return
```

The first launcher format is deliberately small and streamed.

## KSC Script v0.1

Supported instructions:

```text
# comment
PRINT <text>
WRITE <vfs-path> <integer>
WAIT <milliseconds>
STOP
```

Example:

```text
PRINT KSC-04B RUN START
WRITE /dev/led/red 1
WAIT 400
WRITE /dev/led/red 0
PRINT KSC-04B RUN PASS
STOP
```

The remote script is not loaded as a whole into AVR SRAM.

Execution route:

```text
/host/DEMO.KSC
      |
      +-> OPEN
      +-> READ <= 32 B
      +-> assemble one command line <= 63 chars
      +-> execute immediately
      +-> continue streaming
      +-> CLOSE
```

The script line buffer is bounded to 64 bytes.

## Action semantics

### VIEW

Uses the already certified KSC-04A byte-window viewer.

### RUN

The host-backed target streams the `.KSC` file and executes each supported
instruction against the existing local VFS target.

For KSC-04B the important bridge is:

```text
remote program file        local physical namespace
/host/DEMO.KSC  -------->  /dev/led/red
                            /dev/led/blue
                            /dev/buzzer
                            ...
```

## Build

```powershell
New-Item -ItemType Directory -Force .\build\KSC\12_KSC_04B_FileActionsLauncher | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\12_KSC_04B_FileActionsLauncher `
  .\projects\KSC-KonSol-Commander\sketches\12_KSC_04B_FileActionsLauncher
```

Record Flash and global SRAM. KSC-04A baseline was:

```text
Flash       28816 B
Global SRAM  1499 B
```

Because UNO is already at 89% Flash, code-size growth is an explicit acceptance
criterion.

## Physical test route

Upload the firmware and start the normal host:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4
```

Then:

```text
KSC
-> host
-> DEMO.KSC
-> ENTER
```

Expected action menu:

```text
KSC SCRIPT
> VIEW
  RUN
```

### Test A - VIEW

Select VIEW.

Expected:

- existing remote file viewer opens;
- script text is visible;
- BACK returns to the action menu or directory according to current route;
- no reset;
- HOST errors remain zero.

### Test B - RUN

Select RUN.

Expected physical sequence:

1. red discrete LED ON;
2. after about 400 ms red LED OFF;
3. blue discrete LED ON;
4. after about 400 ms blue LED OFF;
5. terminal shows:
   `KSC-04B RUN START`;
6. terminal shows:
   `KSC-04B RUN PASS`;
7. launcher reports `Status: OK`.

Final status target:

```text
IN drop 0
IR drop 0/0
HOST M/0
```

## Bounded claim if PASS

**KSC-04B demonstrates file-oriented actions in KonSol Commander and streamed
execution of a remote host-backed KSC Script against the Arduino UNO local VFS,
without loading the complete script into ATmega328P SRAM.**

Non-claims:

- KSC Script v0.1 is not a general-purpose programming language;
- there are no branches, loops, variables, or expressions yet;
- arbitrary binaries are not executable;
- remote script editing is not included;
- interrupted physical actions are not transactional;
- script authentication or trust policy is not yet implemented.

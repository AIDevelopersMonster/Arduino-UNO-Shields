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


## Build result

Physical build on Arduino UNO / ATmega328P:

```text
Sketch uses 31656 bytes (98%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1525 bytes (74%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 523 bytes.
```

Comparison with KSC-04A:

```text
                         KSC-04A      KSC-04B      Delta
Flash                    28816 B      31656 B      +2840 B
Global SRAM               1499 B       1525 B        +26 B
Compiler SRAM remainder    549 B        523 B        -26 B
Flash headroom             3440 B        600 B      -2840 B
```

Interpretation:

- the build technically fits;
- 600 bytes of remaining Flash headroom is too small for a comfortable physical
  certification baseline;
- SRAM growth is acceptable, but Flash has become the primary blocker;
- KSC-04B should undergo a Flash audit/optimization before upload and physical
  launcher testing.


## Build result after first flash refactor

After reusing the certified `streamWindow()` path for launcher input:

```text
Sketch uses 31118 bytes (96%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1537 bytes (75%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 511 bytes.
Arduino CLI warning: low memory may cause instability.
```

Comparison:

```text
                         Initial 04B   Refactor 1    Delta
Flash                    31656 B       31118 B       -538 B
Global SRAM               1525 B        1537 B        +12 B
Flash headroom              600 B        1138 B       +538 B
SRAM remainder              523 B         511 B        -12 B
```

Interpretation:

- first refactor recovered 538 bytes of Flash;
- Flash headroom improved to 1138 bytes but remains too small for the desired
  KSC-04B physical-certification baseline;
- persistent SRAM increased by 12 bytes due to the stream sink object layout;
- a second flash-focused refactor is required before upload.


## Build result after second flash refactor

After parser/UI slimming:

```text
Sketch uses 30630 bytes (94%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1537 bytes (75%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 511 bytes.
Arduino CLI warning: low memory may cause instability.
```

Comparison:

```text
                         KSC-04A    Initial 04B   Refactor 1   Refactor 2
Flash                    28816 B     31656 B       31118 B      30630 B
Global SRAM               1499 B      1525 B        1537 B       1537 B
Flash headroom             3440 B       600 B        1138 B       1626 B
SRAM remainder              549 B       523 B         511 B        511 B
```

Refactor-2 recovered another 488 bytes of Flash, for a total recovery of
1026 bytes relative to the first KSC-04B build.

Interpretation:

- KSC-04B now fits with 1626 bytes of Flash headroom;
- SRAM remains tight but is still above the runtime level already exercised by
  KSC-04A;
- further pre-test optimization is no longer mandatory;
- proceed to physical upload and launcher certification before adding any new
  feature.


## First physical runtime result

Observed after entering the launcher result screen:

```text
ENTER/BACK Return
RAM 399 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

Confirmed by this observation:

- Commander reached the post-launch result view without reset;
- runtime free RAM is 399 B;
- input drops remain zero;
- HY-M302 IR edge/frame drops remain 0/0;
- host mount/error status remains clean at M/0;
- no stream recovery was required (R0).

This is a strong runtime stability result, but the KSC-04B FULL PHYSICAL PASS
still additionally requires explicit confirmation of the script side effects
(red/blue LED sequence) and the visible RUN success markers/status.


## Physical launcher execution result

Observed on Arduino UNO + HY-M302:

```text
KSC Core 0.1 - KonSol Commander
Target: Arduino UNO + HY-M302
Path: /host/DEMO.KSC
----------------------------------------
ACTIONS
  VIEW
> RUN
----------------------------------------
UP/DOWN Select  ENTER Action
BACK Return
RAM 399 B   IN drop 0   IR drop 0/0   HOST M/0 R0

KSC-04B RUN START
KSC-04B RUN PASS

KSC Core 0.1 - KonSol Commander
Target: Arduino UNO + HY-M302
Path: /host/DEMO.KSC
----------------------------------------
RUN OK
ENTER/BACK Return
RAM 399 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

This confirms that:

- Commander recognized `DEMO.KSC` as an actionable script;
- the ACTIONS view exposed VIEW/RUN;
- RUN streamed and parsed the remote script;
- both script PRINT markers were reached;
- every preceding WRITE/WAIT instruction returned success, because any error
  would terminate execution before the final PASS marker;
- the launcher returned `RUN OK`;
- runtime free RAM remained 399 B;
- input drops remained zero;
- HY-M302 IR drops remained 0/0;
- host status remained clean at M/0 R0;
- no reset occurred.

The only remaining physical-side-effect acceptance item is direct human
confirmation that the expected red-then-blue LED sequence was visibly observed.

# KSC-02C - Core Extraction & Reference Target

Status: IMPLEMENTATION READY FOR PHYSICAL TEST

## Goal

Separate the KSC shell, ANSI Commander, TTY decoder, CHAR/KEY model, and VFS
navigation logic from any HY-M302-specific implementation.

The reference target must run on a bare Arduino UNO with USB Serial only.

No shield is required for this test.

## Architecture under test

```text
                    KSC_Core
          +------------+-------------+
          |            |             |
        Shell       Commander      Input
          |            |             |
          +------------+-------------+
                       |
                 KscTarget API
                       |
               Reference Target
                       |
                synthetic VFS
```

The core library source must not include HY_M302.h and must not contain
HY-M302 pin assignments or DHT/IR/device-specific code.

## Files

```text
projects/KSC-KonSol-Commander/
+-- libraries/
|   +-- KSC_Core/
|       +-- library.properties
|       +-- src/
|           +-- KSC_Core.h
|           +-- KSC_Core.cpp
|
+-- sketches/
    +-- 05_KSC_Core_Reference/
        +-- 05_KSC_Core_Reference.ino
        +-- KSC_ReferenceTarget.h
        +-- KSC_ReferenceTarget.cpp
```

## Reference namespace

```text
/
+-- demo/
|   +-- counter     RO dynamic
|   +-- value       RW 0..255
|   +-- flag        RW 0..1
|
+-- proc/
|   +-- mem         RO dynamic
|   +-- uptime      RO dynamic
|
+-- sys/
    +-- version     RO
    +-- target      RO
    +-- storage     RO
```

Expected values:

```text
/sys/version  -> KSC Core 0.1
/sys/target   -> UNO Reference Target
/sys/storage  -> SYNTHETIC ONLY
```

## Compile

From repository root:

```powershell
New-Item -ItemType Directory -Force .\build\KSC\05_KSC_Core_Reference | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --output-dir .\build\KSC\05_KSC_Core_Reference `
  .\projects\KSC-KonSol-Commander\sketches\05_KSC_Core_Reference
```

Record:

- Flash bytes and percentage;
- global SRAM bytes and percentage.

## Upload

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\05_KSC_Core_Reference
```

## Terminal

Use the already certified KSC Raw TTY 0.3:

```powershell
python `
  .\projects\KSC-KonSol-Commander\tools\ksc_raw_tty.py `
  -p COM4
```

Reset the UNO once after opening the terminal if the boot banner is not visible.

## Expected boot

```text
KSC Core 0.1
Hardware-independent ANSI VFS core
Target: UNO Reference Target
FREE RAM: <measured> B
Target backend: synthetic VFS only
External hardware: none required
Type KSC to open Commander, HELP for shell commands.

KSC:/>
```

## Shell checks

```text
LS /
LS /demo
CAT /demo/counter
CAT /demo/value
WRITE /demo/value 200
CAT /demo/value
WRITE /demo/flag 1
CAT /demo/flag
CAT /proc/mem
CAT /proc/uptime
CAT /sys/version
CAT /sys/target
CAT /sys/storage
```

Expected important results:

```text
/demo/value after WRITE -> 200
/demo/flag after WRITE  -> 1
/sys/version             -> KSC Core 0.1
/sys/target              -> UNO Reference Target
/sys/storage             -> SYNTHETIC ONLY
```

## Commander checks

At the shell:

```text
KSC
```

Expected root:

```text
Path: /
----------------------------------------
> [demo/]
  [proc/]
  [sys/]
----------------------------------------
UP/DOWN Select   ENTER/RIGHT Open
LEFT/BACK Parent HOME Root
F9/MENU Help     F10/POWER/Q Shell
RAM <measured> B   IN drop 0   REF
```

Navigate to:

```text
/demo/counter
```

The value should refresh while the node remains open.

Navigate to:

```text
/demo/value
```

Enter:

```text
128
ENTER
```

Expected:

```text
Value: 128
Status: APPLIED
```

Navigate to:

```text
/demo/flag
```

Set 1 and then 0.

Test:

- UP / DOWN;
- ENTER / RIGHT;
- LEFT / BACK;
- HOME;
- END;
- F9 Help;
- F10 or Q return to shell.

After Commander exit, verify shell state:

```text
CAT /demo/value
CAT /demo/flag
```

## PASS gate

KSC-02C Reference Target is FULL PHYSICAL PASS only when:

1. The sketch compiles for Arduino UNO.
2. The firmware uploads and boots on a bare UNO.
3. The banner identifies UNO Reference Target.
4. No HY-M302 shield is needed.
5. The synthetic root namespace lists demo, proc, and sys.
6. Dynamic RO nodes update.
7. RW value accepts 0..255.
8. RW flag accepts 0..1.
9. Shell and Commander observe the same target state.
10. ANSI navigation works through KSC Raw TTY 0.3.
11. Help and exit-to-shell work.
12. Flash, global SRAM, boot free RAM, and Commander free RAM are recorded.
13. Input drop counter remains zero during normal testing.
14. The KSC_Core library source remains free of HY_M302.h and target pin mappings.

## Non-claim

Passing the Reference Target proves that the extracted core can execute without
HY-M302 hardware. It does not yet, by itself, prove multi-target portability.

The stronger hardware-independence claim requires the same KSC_Core source to
pass with at least one physically different target adapter, beginning with the
existing HY-M302 hardware line.

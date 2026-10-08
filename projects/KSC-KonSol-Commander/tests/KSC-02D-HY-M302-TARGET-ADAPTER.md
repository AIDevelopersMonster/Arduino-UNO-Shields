# KSC-02D - HY-M302 Target Adapter on KSC_Core

Status: IMPLEMENTATION READY FOR PHYSICAL TEST

## Goal

Prove that the same extracted KSC_Core source used by the bare-UNO Reference
Target can also drive the physical HY-M302 target through a KscTarget adapter,
without moving HY-M302-specific code back into the core.

## Architecture under test

```text
                  SAME KSC_Core
                  /           \
                 /             \
Reference Target                 HY-M302 Target
bare UNO                         physical shield
synthetic VFS                    sensors/actuators
FULL PASS                        TEST PENDING
```

The KSC_Core source must remain unchanged from the Reference Target test.

## Files

```text
projects/KSC-KonSol-Commander/
+-- libraries/
|   +-- KSC_Core/
|       +-- src/
|           +-- KSC_Core.h
|           +-- KSC_Core.cpp
|
+-- sketches/
    +-- 06_KSC_Core_HY_M302/
        +-- 06_KSC_Core_HY_M302.ino
        +-- KSC_HYM302Target.h
        +-- KSC_HYM302Target.cpp
```

## Physical namespace

```text
/
+-- dev/
|   +-- sw1            RO
|   +-- sw2            RO
|   +-- pot            RO
|   +-- light          RO
|   +-- dht/
|   |   +-- temp       RO
|   |   +-- humidity   RO
|   |   +-- status     RO
|   |   +-- age        RO
|   +-- led/
|   |   +-- red        RW 0..1
|   |   +-- blue       RW 0..1
|   +-- rgb/
|   |   +-- red        RW 0..255
|   |   +-- green      RW 0..255
|   |   +-- blue       RW 0..255
|   +-- buzzer         RW 0..1
+-- proc/
|   +-- mem            RO
|   +-- uptime         RO
+-- sys/
    +-- version        RO
    +-- target         RO
    +-- storage        RO
```

Expected:

```text
/sys/version -> KSC Core 0.1
/sys/target  -> UNO + HY-M302
/sys/storage -> VIRTUAL ONLY
```

## Compile

From repository root:

```powershell
New-Item -ItemType Directory -Force .\build\KSC\06_KSC_Core_HY_M302 | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\06_KSC_Core_HY_M302 `
  .\projects\KSC-KonSol-Commander\sketches\06_KSC_Core_HY_M302
```

Record Flash and global SRAM.

## Build measurements

Arduino CLI compile result:

```text
Sketch uses 20806 bytes (64%) of program storage space.
Maximum is 32256 bytes.

Global variables use 1255 bytes (61%) of dynamic memory,
leaving 793 bytes for local variables.
Maximum is 2048 bytes.
```

Build status: PASS.

Runtime free RAM is measured at 734 B on the physical HY-M302 target.

## Upload

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\06_KSC_Core_HY_M302
```

## Terminal

```powershell
python `
  .\projects\KSC-KonSol-Commander\tools\ksc_raw_tty.py `
  -p COM4
```

## Expected boot

```text
KSC Core 0.1
Hardware-independent ANSI VFS core
Target: Arduino UNO + HY-M302
FREE RAM: <measured> B
IR INIT: OK
Target backend: physical HY-M302
Storage: virtual namespace only
Type KSC to open Commander, HELP for shell commands.

KSC:/>
```

## Physical boot measurement

The HY-M302 target adapter booted successfully through KSC Raw TTY 0.3 using
the same KSC_Core source as the previously certified Reference Target.

Observed startup:

```text
KSC Core 0.1
Hardware-independent ANSI VFS core
Target: Arduino UNO + HY-M302
FREE RAM: 734 B
IR INIT: OK
Target backend: physical HY-M302
Storage: virtual namespace only
Type KSC to open Commander, HELP for shell commands.

KSC:/>
```

Physical boot status: PASS.

Measured runtime free RAM after initialization: 734 B.

IR initialization status: PASS.

## Physical route

Repeat the KSC-02 certified behavior through the new adapter:

1. `LS /`, `LS /dev`, `CAT /sys/version`, `CAT /sys/target`.
2. Check `/dev/light` live change.
3. Check `/dev/pot`.
4. Check DHT temp/humidity/status.
5. Set `/dev/led/red` 1 then 0 and observe physical LED.
6. Set `/dev/rgb/red` to 128 and observe physical RGB output.
7. Set `/dev/buzzer` 1 then 0 and observe physical buzzer.
8. Enter Commander with `KSC`.
9. Repeat RO/RW operations through Commander.
10. Verify PC keyboard navigation.
11. Verify IR UP/DOWN/OK/RETURN/HOME/MENU/POWER.
12. Verify IR numeric entry on an RW node.
13. Exit to shell and verify shell still observes the current VFS state.
14. Record boot and Commander free RAM.
15. Confirm input/IR drops remain zero in normal operation.

## Strong PASS gate

KSC-02D is FULL PHYSICAL PASS only if:

- the same KSC_Core source used by the Reference Target is used unchanged;
- the physical HY-M302 target passes the same shell/Commander abstraction;
- live sensors and physical actuators work;
- PC keyboard and IR remote both drive the same core input model;
- shell and Commander share the same adapter state;
- no HY-M302-specific include, pin assignment, or driver logic exists in KSC_Core;
- Flash, SRAM, boot free RAM, Commander free RAM, and drop counters are recorded.

On this PASS, the project may make the stronger empirical claim that the
KonSol Commander core is hardware-independent with respect to the two tested
target adapters:

```text
Reference Target  <-> KSC_Core <-> HY-M302 Target
```

This does not claim universal portability to arbitrary MCUs or interfaces.

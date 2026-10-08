# KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for Resource-Constrained 8-bit Systems

**Subtitle:** Two-target physical validation on Arduino UNO using a synthetic Reference Target and the HY-M302 multifunction shield

**Author:** A. A. Malachevsky  
**ORCID:** 0009-0008-6009-3196  
**DOI:** https://doi.org/10.5281/zenodo.23232216  
**Manuscript status:** v0.2 - publication candidate  
**Experimental evidence snapshot:** `96487e66b159df75a1b3162098590cb23ab4af49`  
**Project:** KSC - KonSol Commander  
**Repository:** AIDevelopersMonster/Arduino-UNO-Shields  
**Validated platform:** Arduino UNO / ATmega328P  
**Date:** 2026-10-08

---

## Citation

Malachevsky, A. A. (2026). **KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for Resource-Constrained 8-bit Systems**. Zenodo. https://doi.org/10.5281/zenodo.23232216

ORCID: https://orcid.org/0009-0008-6009-3196

---

## Abstract

This paper presents **KSC_Core**, a small target-decoupled resident core for an
ATmega328P-class 8-bit system. The core exposes target resources through a
filesystem-like virtual namespace and provides two human interfaces over the
same namespace: a command shell and a one-panel ANSI-style terminal Commander.
Device-specific logic, pin mappings, sensor drivers, actuator drivers, and
target-specific input decoding are excluded from the core and supplied through a
compact `KscTarget` adapter interface.

The implementation was physically validated on two target configurations using
the **same unchanged KSC_Core source**. The first configuration was a bare
Arduino UNO using a synthetic Reference Target with dynamic and writable virtual
nodes but no shield. The second used an Arduino UNO with an HY-M302
multifunction shield, exposing physical buttons, potentiometer, light sensor,
DHT11 temperature/humidity data, discrete LEDs, RGB PWM channels, an active
buzzer, and an asynchronous NEC IR remote through the same core-facing
abstractions.

The Reference Target build used 13,658 bytes of sketch flash capacity and 669
bytes of global SRAM. Runtime observations showed 1,320 bytes free after
initialization, 1,279 bytes free during typical Commander operation, and 1,262
bytes as the minimum observed value on the recorded test route. The HY-M302
build used 20,806 bytes of sketch flash capacity and 1,255 bytes of global SRAM,
with 734 bytes free after initialization, 693 bytes free during typical
Commander operation, and 676 bytes as the minimum observed value on the recorded
route. No input-drop event was observed in either normal-operation test route;
the HY-M302 IR dropped-edge and dropped-frame counters remained 0/0 during the
recorded route.

The result supports a deliberately bounded statement: **KSC_Core is
target-backend-independent with respect to the two physically tested target
adapters on the Arduino UNO platform**. The experiment does not establish
universal portability to arbitrary MCUs, architectures, transports, displays,
storage systems, or peripherals.

**Keywords:** Arduino UNO, ATmega328P, AVR, virtual namespace, virtual
filesystem, VFS, embedded systems, terminal UI, ANSI-style terminal, hardware
abstraction, target adapter, resource-constrained systems, HY-M302, IR remote,
reproducible testing

---

## 1. Introduction

Small 8-bit microcontrollers are commonly implemented as tightly coupled
applications in which device drivers, hardware pin assignments, state
representation, and user-interface logic are combined in a single firmware
unit. That organization is practical for single-purpose systems but makes it
harder to determine which parts of the interaction model are genuinely reusable
when attached hardware changes.

KSC - KonSol Commander explores a different organization. Target resources are
presented as a navigable path namespace. A path may represent a live sensor
value, a writable actuator, a generated system value, or, in future work, a
storage or remote object. The shell and Commander interact with path semantics
rather than with concrete device classes.

The general idea of exposing system or device state through filesystem-shaped
interfaces is established prior art. Linux `procfs` and `sysfs`, for example,
expose process, kernel, and device-related state through path-oriented
interfaces. This work does **not** claim invention of a virtual filesystem, nor
does it claim invention of representing device attributes as files.

The contribution studied here is narrower:

1. extraction of a small reusable KSC core from a shield-centered
   implementation;
2. a compact target-adapter contract for directory, RO, RW, dynamic-refresh,
   and input semantics;
3. physical validation of the **same unchanged core source** against two
   different target backends under the memory constraints of an ATmega328P
   system;
4. explicit resource measurements and bounded non-claims.

The research question is therefore:

> Can the same small core provide the same shell, ANSI-style Commander, virtual
> namespace semantics, and CHAR/KEY input model when the target backend changes
> from a synthetic software target to a physically populated multifunction
> shield?

The experimental platform is Arduino UNO / ATmega328P. Arduino documents the
UNO Rev3 as using the ATmega328P with 32 KB total flash, 2 KB SRAM, 1 KB EEPROM,
and a 16 MHz clock. Arduino also notes that 0.5 KB of flash is used by the
bootloader. The toolchain used for the recorded builds therefore reported a
32,256-byte maximum sketch capacity. Microchip specifies the ATmega328P as an
8-bit AVR with 32 KB program memory, 2,048 bytes SRAM, and 1,024 bytes EEPROM.

---

## 2. Claim boundary

The primary experimental statement is:

> The same KSC_Core source was physically validated with two different target
> backends on the Arduino UNO platform: a synthetic bare-UNO Reference Target
> and a physical HY-M302 target adapter.

For the purposes of this paper, **target-decoupled** means that the shell,
Commander, path/navigation logic, terminal input parser, CHAR/KEY event model,
numeric RW editing, and dynamic-node refresh policy do not contain the
HY-M302-specific implementation and interact with target resources only through
the `KscTarget` contract.

The following stronger claims are not made:

- KSC_Core is not claimed to run unchanged on every MCU family.
- Portability to non-AVR architectures is not tested.
- Independence from Arduino runtime APIs is not claimed.
- Independence from every terminal or serial transport is not tested.
- The work does not establish a general-purpose or protected operating system.
- The virtual namespace is not claimed to implement POSIX filesystem semantics.
- No process isolation, memory protection, preemption, or multi-user security
  model is claimed.
- Byte-stream files, actions, event streams, remote storage, and the future
  `/host` mount are outside the validated result.
- Formal worst-case stack safety is not established.
- Lossless input under arbitrary event rates is not established.

The wording **hardware-independent** is used only in the bounded sense of
independence from the two tested target implementations on the same MCU/platform.
The preferred general term in this paper is **target-decoupled**.

---

## 3. Core design

### 3.1 Namespace is not storage

A path need not correspond to bytes stored on a block device. For example:

```text
/dev/light
/dev/led/red
/proc/mem
/sys/version
```

may resolve respectively to an ADC-backed value, an actuator state, a runtime
measurement, or a constant target/system identifier.

The validated experiments therefore require no SD card.

### 3.2 One target abstraction for Shell and Commander

The shell and Commander share the same target-facing operations.

```text
              Shell             Commander
                 \               /
                  \             /
                   KSC_Core
                      |
                 KscTarget API
                      |
                  target state
```

A target value modified through one interface is read back through the same
target object by the other.

### 3.3 Target-specific code remains outside the core

The target contract includes operations equivalent to:

```text
begin()
service()
pollInput(core)
name()
bootReport()

pathType(path)
dirCount(path)
dirEntry(path, index)

read(path)
write(path, value)
writableRange(path)
readWritableLong(path)
pathIsDynamic(path)

printStatus()
```

Direct inspection of the frozen core source found no `HY_M302` reference,
DHT-specific code, Arduino `String` use, explicit dynamic allocation call, or
direct Arduino GPIO/ADC operation.

This is a **source-separation property**, not a claim that the core is
independent of Arduino itself. KSC_Core still uses Arduino runtime facilities
such as `Arduino.h`, `Stream`, `millis()`, and flash-string helpers.

### 3.4 Semantic input model

KSC_Core distinguishes printable data from navigation/control input:

```text
INPUT EVENT
+-- CHAR
+-- KEY
```

The event also retains a source identifier. A PC terminal and a target adapter
may therefore generate equivalent logical events without requiring Commander
code to understand the physical source.

The current special-key set includes navigation, Enter, Back, Home, End,
Delete, Menu, and Power/exit semantics.

### 3.5 Fixed-size memory discipline

The resident core avoids Arduino `String` and dynamic allocation. Paths,
values, shell lines, and event queues use fixed-size buffers.

This design does not prove absence of every possible stack-exhaustion path, but
it makes memory use inspectable and permits direct runtime free-SRAM
measurements.

---

## 4. Runtime free-SRAM measurement

The runtime measurements reported in this paper use `kscFreeRam()`.

On the tested AVR build, the function creates a local stack variable and
computes the distance from its address to either:

- the current heap break, when a heap exists; or
- `__heap_start`, when no heap break has been established.

Thus values such as:

```text
Boot free RAM
Commander free RAM
Minimum observed free RAM
```

are **instantaneous runtime estimates** observed at specific points on the
recorded test route.

They are not:

- compiler-computed worst-case stack bounds;
- stack high-water measurements over all execution paths;
- formal guarantees that every future path has the same free-memory margin.

The phrase **minimum observed** means the smallest free-SRAM value visible in
the preserved physical proof for that test route.

---

## 5. Core source identity

The central same-core evidence is source identity, not merely behavioral
similarity.

The validated core blobs are:

```text
KSC_Core.h
9190ef5d231acc82498b46a897e32b6e4327ed92

KSC_Core.cpp
211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0
```

A repository comparison from
`d08e1632ed8154af7ad3d45462be883f8e9983c2` to the closed experimental
snapshot `96487e66b159df75a1b3162098590cb23ab4af49` adds the Reference Target,
HY-M302 target adapter, physical-test documents, and project documentation, but
does not modify either KSC_Core source file.

The experimental structure is therefore:

```text
Reference Target  <->  SAME KSC_Core  <->  HY-M302 Target
FULL PASS                                  FULL PASS
```

---

## 6. Experiment A: bare-UNO Reference Target

### 6.1 Purpose

The first experiment tests whether KSC_Core remains functional when HY-M302,
DHT, NEC remote decoding, and shield pin assignments are absent.

The target is a bare Arduino UNO connected over USB Serial.

### 6.2 Namespace

```text
/
+-- demo/
|   +-- counter     RO dynamic
|   +-- value       RW 0..255
|   +-- flag        RW 0..1
+-- proc/
|   +-- mem         RO dynamic
|   +-- uptime      RO dynamic
+-- sys/
    +-- version     RO
    +-- target      RO
    +-- storage     RO
```

Identity values:

```text
/sys/version -> KSC Core 0.1
/sys/target  -> UNO Reference Target
/sys/storage -> SYNTHETIC ONLY
```

### 6.3 Observed behavior

The firmware compiled, uploaded, and booted through KSC Raw TTY 0.3.

The dynamic `/demo/counter` node was left open in Commander and observed
changing from 174 through 221.

The `/demo/value` node was observed at 200, edited through CHAR input to 128,
and committed with `Status: APPLIED`. LEFT adjustment then changed it to 127
and 126.

The `/demo/flag` node was changed from 1 to 0 and back to 1, each time with
`Status: APPLIED`.

No input-drop event was observed on the recorded normal-operation route.

### 6.4 Resource envelope

```text
Sketch capacity used:       13658 / 32256 B = 42% (CLI report)
Global SRAM:                  669 / 2048 B  = 32% (CLI report)
Boot free SRAM:              1320 B
Commander free SRAM:         1279 B typical
Minimum observed free SRAM:  1262 B
Input drops observed:           0
External shield required:       no
```

The result establishes that the extracted core is not dependent on HY-M302 for
its basic shell, Commander, namespace, RO/RW, and terminal-input operation.

---

## 7. Experiment B: physical HY-M302 Target

### 7.1 Purpose

The second experiment asks whether the same core can operate with a target whose
namespace is backed by physical sensors, physical actuators, and an alternate
physical input source.

### 7.2 Namespace

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

### 7.3 Boot

Observed startup:

```text
KSC Core 0.1
Hardware-independent ANSI VFS core
Target: Arduino UNO + HY-M302
FREE RAM: 734 B
IR INIT: OK
Target backend: physical HY-M302
Storage: virtual namespace only
```

The legacy boot string still contains `Hardware-independent`; in this paper the
term is interpreted only according to the bounded claim in Section 2.

### 7.4 Live target-backed reads

Recorded shell observations included:

```text
/dev/light          328 -> 70 -> 616
/dev/pot            0 -> 582 -> 1023
/dev/dht/temp       26.3
/dev/dht/humidity   30.0
/dev/dht/status     OK
```

These values are functional evidence that reads reached live target-backed
objects. They are not a DHT11 calibration or metrological accuracy result.

### 7.5 Physical writes

Recorded successful shell operations included:

```text
WRITE /dev/led/red 1   -> OK
WRITE /dev/led/red 0   -> OK
WRITE /dev/rgb/red 128 -> OK
WRITE /dev/buzzer 1    -> OK
WRITE /dev/buzzer 0    -> OK
```

Physical effects were observed on the corresponding hardware.

In Commander, the red and blue discrete LED nodes were opened as RW nodes,
edited with exact CHAR input, and committed with `Status: APPLIED`.

### 7.6 IR input adapter

The HY-M302 target adapter maps decoded NEC remote events into the same semantic
input model used by the PC terminal:

```text
IR digit      -> CHAR '0'..'9'
IR UP         -> KEY_UP
IR DOWN       -> KEY_DOWN
IR LEFT       -> KEY_LEFT
IR RIGHT      -> KEY_RIGHT
IR OK         -> KEY_ENTER
IR RETURN     -> KEY_BACK
IR HOME       -> KEY_HOME
IR MENU       -> KEY_MENU
IR POWER      -> KEY_POWER
```

Thus the target-specific IR decoder is outside KSC_Core, while Commander
consumes only CHAR and KEY events.

No input-drop event was observed during the recorded normal-operation route, and
the target-reported NEC dropped-edge/dropped-frame counters remained 0/0 during
that route. This is not a throughput guarantee for arbitrary IR/input rates.

### 7.7 Resource envelope

```text
Sketch capacity used:       20806 / 32256 B = 64% (CLI report)
Global SRAM:                 1255 / 2048 B  = 61% (CLI report)
Boot free SRAM:               734 B
Commander free SRAM:          693 B typical
Minimum observed free SRAM:   676 B
Input drops observed:           0
IR drops observed:             0 / 0
```

Relative to the Reference Target build, the HY-M302 configuration adds exactly:

```text
Flash / sketch bytes: 7148 B
Global SRAM:           586 B
```

These are whole-build deltas. They include target adapter and physical driver
costs and are **not** a separately measured size of the adapter alone.

---

## 8. Comparative result

| Metric | Reference Target | HY-M302 Target |
|---|---:|---:|
| Sketch flash capacity used | 13,658 B | 20,806 B |
| CLI percentage | 42% | 64% |
| Global SRAM | 669 B | 1,255 B |
| CLI SRAM percentage | 32% | 61% |
| Boot free SRAM | 1,320 B | 734 B |
| Commander free SRAM, typical | 1,279 B | 693 B |
| Minimum observed free SRAM | 1,262 B | 676 B |
| Input drops observed | 0 | 0 |
| IR drop counters observed | n/a | 0 / 0 |
| External shield | none | HY-M302 |
| Dynamic RO nodes | yes | yes |
| RW nodes | yes | yes |
| Physical sensor backing | no | yes |
| Physical actuator backing | no | yes |
| Alternate target input | none | NEC IR remote |

The table shows that target decoupling is not a zero-cost abstraction. The
physical configuration has materially higher resource use. The engineering
claim is separation of core behavior from target implementation, not elimination
of target cost.

---

## 9. What the experiment demonstrates

The preserved two-target result supports the following statements.

### 9.1 Commander is decoupled from concrete node implementation

Commander renders DIR, RO, and RW semantics without device-specific branches for
LED, DHT, LDR, potentiometer, or buzzer.

### 9.2 Shell and Commander share the same target state

Both interfaces call the same target object through KSC_Core, so state is not
duplicated into separate shell and Commander device implementations.

### 9.3 The input semantic layer is decoupled from the physical input source

The core consumes CHAR and KEY events. PC terminal input and target-side IR
input can therefore control the same Commander state.

### 9.4 The tested namespace does not require physical mass storage

Both configurations run without an SD card. The current path namespace is
therefore independent of a block-storage implementation.

### 9.5 The source identity of the core survives the target change

This is the strongest direct evidence for the paper's central claim: the
Reference Target and HY-M302 Target were added and physically passed while the
two KSC_Core source blobs remained unchanged.

---

## 10. Limitations and non-claims

The result is deliberately limited.

First, both targets use the same Arduino UNO / ATmega328P platform. Cross-MCU
and cross-architecture portability are untested.

Second, KSC_Core still uses Arduino runtime facilities. The current experiment
therefore demonstrates target/backend decoupling, not framework independence.

Third, only DIR, RO, RW, dynamic refresh, and injected CHAR/KEY input semantics
are included in the validated interface. FILE, EVENT, ACTION, persistence, and
remote storage remain future work.

Fourth, the runtime SRAM values are instantaneous estimates and observed minima,
not formal worst-case stack bounds.

Fifth, zero observed drop counters apply only to the recorded physical test
routes.

Sixth, DHT values demonstrate live data transport through the namespace; they do
not establish sensor calibration.

Seventh, the host helper used in the certified route, `ksc_raw_tty.py`, imports
Windows `msvcrt`. The tested reproduction path is therefore Windows-specific.
This limitation belongs to the test harness, not to a demonstrated cross-OS
property of KSC_Core.

Eighth, the UI uses a small ANSI/ECMA-48-style control subset. Full ECMA-48
conformance is neither implemented nor claimed.

---

## 11. Reproducibility

The experimental evidence is tied to:

```text
96487e66b159df75a1b3162098590cb23ab4af49
```

### 11.1 Core

```text
projects/KSC-KonSol-Commander/libraries/KSC_Core/
```

### 11.2 Reference Target

```text
projects/KSC-KonSol-Commander/sketches/05_KSC_Core_Reference/
projects/KSC-KonSol-Commander/tests/KSC-02C-CORE-REFERENCE-TARGET.md
```

### 11.3 HY-M302 Target

```text
projects/KSC-KonSol-Commander/sketches/06_KSC_Core_HY_M302/
projects/KSC-KonSol-Commander/tests/KSC-02D-HY-M302-TARGET-ADAPTER.md
```

### 11.4 Terminal helper

```text
projects/KSC-KonSol-Commander/tools/ksc_raw_tty.py
```

The build and upload commands are recorded in the two physical test documents.

For exact long-term rebuild reproducibility, the Zenodo package also requires a
toolchain-capture file recording the exact Arduino CLI version, installed AVR
core version, Python version, and pyserial version used for the final
publication candidate. These metadata are intentionally treated as a
pre-deposit gate rather than guessed from the source tree.

---

## 12. Relation to established filesystem-shaped interfaces

The use of path-oriented interfaces for system and device state has established
precedent. Linux `procfs` exposes process and kernel-related information, while
`sysfs` exports kernel objects and attributes into a filesystem-shaped user
interface.

KSC_Core is not presented as an alternative implementation of procfs or sysfs
and does not attempt to reproduce their semantics.

The relevance of this prior art is conceptual: it demonstrates that a
filesystem-shaped interface can be useful independently of ordinary persistent
files. The contribution in this paper is the measured target-adapter separation
and physical validation of a deliberately small implementation under an
ATmega328P/2-KB-SRAM constraint.

---

## 13. Relation to KSC-03

KSC-03 `/host` is intentionally excluded from the experimental result.

A host filesystem mount introduces a different class of problems:

- multiplexing human terminal traffic and machine protocol traffic on one serial
  connection;
- framed request/response parsing;
- streamed byte data;
- remote directory enumeration;
- bounded transfer buffers;
- disconnection and recovery behavior.

Those mechanisms may require changes or extensions to the currently validated
target/VFS contract. They should therefore be tested as a separate result rather
than silently folded into the present article.

---

## 14. Conclusion

KSC_Core was extracted from an initially HY-M302-centered Commander
implementation into a target-decoupled shell, terminal Commander, virtual
namespace, and semantic input core.

The first physical configuration used a bare Arduino UNO and a synthetic
Reference Target. The second used Arduino UNO + HY-M302 with live sensors,
physical actuators, and an asynchronous IR input source. Both passed their
recorded shell and Commander routes while using identical KSC_Core source blobs.

The result supports the following precise statement:

> **On the tested Arduino UNO platform, KSC_Core is independent of the concrete
> implementation of the two physically validated target backends: the synthetic
> Reference Target and the HY-M302 Target.**

It does not establish universal hardware or platform independence.

The next experimental question is whether the same namespace discipline can
cross a transport boundary and represent streamed host files without violating
the memory constraints of a 2 KB SRAM system. That question belongs to KSC-03
and is deliberately left outside this publication candidate.

---

## References

1. Arduino. **Arduino UNO Rev3**. Official product documentation. ATmega328P,
   32 KB flash, 2 KB SRAM, 1 KB EEPROM, 16 MHz; 0.5 KB flash used by the
   bootloader.  
   https://store.arduino.cc/products/arduino-uno-rev3

2. Arduino. **Arduino UNO R3 Datasheet**, A000066.  
   https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf

3. Microchip Technology. **ATmega328P Product Page and Datasheet Resources**.
   8-bit AVR; 32 KB program memory; 2,048 B RAM; 1,024 B EEPROM.  
   https://www.microchip.com/en-us/product/ATmega328P

4. Ecma International. **ECMA-48: Control Functions for Coded Character Sets**,
   5th edition, June 1991.  
   https://ecma-international.org/publications-and-standards/standards/ecma-48/

5. Linux Kernel Documentation. **The /proc Filesystem**.  
   https://www.kernel.org/doc/html/latest/filesystems/proc.html

6. Linux Kernel Documentation. **sysfs - The filesystem for exporting kernel
   objects**.  
   https://www.kernel.org/doc/html/latest/filesystems/sysfs.html

7. AIDevelopersMonster. **Arduino-UNO-Shields / KSC - KonSol Commander**.
   Source, physical test protocols, and certification records.  
   https://github.com/AIDevelopersMonster/Arduino-UNO-Shields

---

## Appendix A. Compact certified statement

```text
Validated platform:
  Arduino UNO / ATmega328P

Core:
  KSC_Core 0.1

Core source identity:
  KSC_Core.h   9190ef5d231acc82498b46a897e32b6e4327ed92
  KSC_Core.cpp 211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0

Target A:
  bare UNO / synthetic Reference Target
  FULL PHYSICAL PASS

Target B:
  UNO + HY-M302 / physical target adapter
  FULL PHYSICAL PASS

Bounded conclusion:
  Same KSC_Core source passed both target backends on Arduino UNO.

Non-claim:
  Universal MCU, framework, transport, peripheral, or filesystem portability is
  not proven.
```

## Appendix B. Russian title

**KSC_Core: отделённое от целевого оборудования ядро виртуального пространства и ANSI Commander для ресурсно-ограниченных 8-битных систем**

Подзаголовок:

**Двухтаргетная физическая валидация на Arduino UNO: программный Reference Target и плата расширения HY-M302**

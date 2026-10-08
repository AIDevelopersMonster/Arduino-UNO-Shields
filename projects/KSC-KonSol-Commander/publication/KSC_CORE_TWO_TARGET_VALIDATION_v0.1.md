# KSC_Core: A Hardware-Independent Virtual Namespace and ANSI Commander Core for Resource-Constrained 8-bit Systems

**Subtitle:** Two-target physical validation on Arduino UNO using a synthetic reference target and the HY-M302 multifunction shield

**Manuscript status:** v0.1 - engineering preprint draft  
**Experimental source snapshot:** `96487e66b159df75a1b3162098590cb23ab4af49`  
**Project:** KSC - KonSol Commander  
**Repository:** AIDevelopersMonster/Arduino-UNO-Shields  
**Platform:** Arduino UNO / ATmega328P  
**Date:** 2026-10-08

---

## Abstract

This paper presents **KSC_Core**, a small target-neutral resident core for an
ATmega328P-class 8-bit system. The core exposes target resources through a
filesystem-like virtual namespace and provides two human interfaces over the
same namespace: a command shell and a one-panel ANSI terminal Commander. The
central architectural requirement is that device-specific logic, pin mappings,
sensor drivers, and target-specific input decoding remain outside the core and
are supplied through a compact `KscTarget` adapter interface.

The implementation was physically validated on two target configurations using
the **same unchanged KSC_Core source**. The first configuration was a bare
Arduino UNO using a synthetic Reference Target with dynamic and writable virtual
nodes but no shield. The second configuration used an Arduino UNO with the
HY-M302 multifunction shield, exposing physical buttons, potentiometer, light
sensor, DHT11 temperature/humidity data, discrete LEDs, RGB PWM channels, an
active buzzer, and an asynchronous NEC IR remote as target-backed virtual nodes
and input events.

The bare-UNO Reference Target used 13,658 bytes of flash and 669 bytes of global
SRAM, with 1,320 bytes free after initialization and 1,279 bytes free during
typical Commander operation. The HY-M302 target used 20,806 bytes of flash and
1,255 bytes of global SRAM, with 734 bytes free after initialization, 693 bytes
free during typical Commander operation, and a minimum observed value of 676
bytes in the recorded physical proof. Input drops remained zero in both target
tests; HY-M302 IR dropped-edge and dropped-frame counters remained 0/0.

The result supports a deliberately bounded claim: **KSC_Core is
hardware-independent with respect to the two physically tested target adapters**.
The result does not establish universal portability to arbitrary
microcontrollers, transports, displays, storage backends, or peripherals.

**Keywords:** Arduino UNO, ATmega328P, AVR, virtual filesystem, VFS, embedded
systems, terminal UI, ANSI terminal, hardware abstraction, target adapter,
resource-constrained systems, HY-M302, IR remote, reproducible testing

---

## 1. Introduction

Small 8-bit microcontrollers are commonly programmed as a single application
whose user interface, device drivers, state representation, and hardware pin
assignments are tightly coupled. This is often acceptable for one-purpose
firmware, but it becomes limiting when the same interaction model is expected
to survive changes in attached hardware.

KSC - KonSol Commander explores a different organization. Instead of treating
every sensor or actuator as a special case in the user interface, KSC exposes
resources through a navigable path namespace. A path may represent a live
sensor value, a writable actuator, a generated system value, or later a storage
or remote object. The human interface is therefore expressed against the
namespace rather than directly against hardware.

The work reported here isolates that concept into **KSC_Core** and asks a
narrow engineering question:

> Can the same small core provide the same shell, ANSI Commander, virtual
> namespace semantics, and input model when the target backend changes from a
> purely synthetic software target to a physically populated multifunction
> shield?

The purpose of the experiment is not to prove universal portability. The
purpose is to test whether the architecture survives a concrete change of
target while the core itself remains unchanged.

The experimental platform is the Arduino UNO / ATmega328P. The official Arduino
UNO Rev3 specification gives 32 KB of flash memory, 2 KB of SRAM, 1 KB of
EEPROM, and a 16 MHz clock. Microchip lists the ATmega328P as an 8-bit AVR MCU
with 32 KB program memory and 2 KB SRAM. These limits make source separation,
fixed-size buffers, and runtime memory measurements materially relevant rather
than cosmetic.

---

## 2. Claim boundary

The primary claim of this paper is intentionally finite:

> The same KSC_Core source was physically validated with two different target
> adapters: a synthetic bare-UNO Reference Target and a physical HY-M302 target.

The following stronger claims are **not** made:

- KSC_Core is not claimed to run unchanged on every microcontroller family.
- The work does not prove portability to non-AVR architectures.
- The work does not prove independence from every terminal or serial transport.
- The work does not establish a general-purpose operating system.
- The virtual namespace is not claimed to be a POSIX filesystem.
- No protected process model, memory protection, preemption, or multi-user
  security model is claimed.
- The current experiment does not include the future `/host` remote filesystem
  backend.
- The current experiment does not establish that all future target adapters can
  be implemented without modifying the core.

The bounded claim is stronger than an architectural intention because it is tied
to an exact source identity and two physical test routes.

---

## 3. Design principles

KSC_Core was extracted under six constraints.

### 3.1 Namespace before storage

A path does not imply that bytes exist on a physical disk. For example:

```text
/dev/light
/dev/led/red
/proc/mem
/sys/version
```

may be backed by an ADC read, a digital output state, a runtime memory
measurement, or a constant string.

This distinction allows the same shell and Commander to operate even when no SD
card or mass-storage device exists.

### 3.2 One VFS for Shell and Commander

The shell and Commander do not maintain separate device implementations.

Conceptually:

```text
                 KSC virtual namespace
                         |
                  KscTarget adapter
                    /           \
                 Shell       Commander
```

Both interfaces invoke the same target path operations. A value written through
the shell is therefore immediately visible through the Commander and vice
versa.

### 3.3 Target-specific code remains outside the core

KSC_Core contains no HY-M302 include, DHT implementation, NEC remote decoder,
or HY-M302 pin map. Device-specific behavior is implemented in a target adapter.

The target contract includes operations equivalent to:

```text
pathType(path)
dirCount(path)
dirEntry(path, index)
read(path)
write(path, value)
writableRange(path)
readWritableLong(path)
pathIsDynamic(path)
pollInput(core)
```

The core operates only on these abstract operations.

### 3.4 Character and special-key events are distinct

KSC uses a semantic input model:

```text
INPUT EVENT
+-- CHAR
+-- KEY
```

Printable terminal input is a character event. Navigation and control inputs are
special-key events. Target adapters may inject either class.

This permits physically different sources to produce the same logical action.
For example, a PC keyboard arrow and a decoded HY-M302 IR arrow both become the
same KSC navigation key, while a digit from either source remains a character
used for exact numeric entry.

### 3.5 Fixed memory policy

The resident core avoids Arduino `String` objects and does not require dynamic
allocation. Paths, values, shell lines, and event queues use bounded buffers.

This policy is motivated by the ATmega328P SRAM limit rather than by stylistic
preference.

### 3.6 Physical certification before claim expansion

A feature is not treated as established merely because it compiles. The project
records:

- flash usage;
- global SRAM usage;
- runtime free SRAM;
- navigation behavior;
- read/write behavior;
- input-drop counters;
- target-specific drop counters where available;
- direct physical effects for actuator nodes.

---

## 4. KSC_Core architecture

The extracted architecture is:

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
          +------------+-------------+
          |                          |
  Reference Target              HY-M302 Target
  synthetic namespace           physical hardware
```

The core owns:

- ANSI terminal rendering;
- directory and node navigation;
- shell command parsing;
- path construction;
- the CHAR/KEY event queue;
- terminal escape-sequence decoding;
- RW numeric editing;
- dynamic-node refresh policy.

A target adapter owns:

- node enumeration;
- node type and range semantics;
- physical or synthetic reads;
- physical or synthetic writes;
- target-specific periodic service;
- target-specific input acquisition;
- target identity and target status.

This separation is the main object under test.

---

## 5. Virtual namespace model

The current implementation uses the node classes:

```text
DIR
RO
RW
```

The wider KSC programme also anticipates event, action, and byte-stream nodes,
but those are outside the experimental claim of this paper.

For the current core, a directory is enumerated through the target adapter,
read-only nodes return textual values, and read/write nodes expose a numeric
range.

The Commander interprets this abstract information rather than device identity.
It does not contain code such as "if this is an LED" or "if this is DHT11".

---

## 6. Reference Target experiment

### 6.1 Purpose

The Reference Target was designed to answer the first separation question:

> Can the extracted KSC_Core execute and remain useful when the HY-M302 shield,
> its drivers, its IR decoder, and its pin assignments are absent?

The test used a bare Arduino UNO connected over USB Serial.

### 6.2 Namespace

The synthetic namespace was:

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

The important system identity values were:

```text
/sys/version -> KSC Core 0.1
/sys/target  -> UNO Reference Target
/sys/storage -> SYNTHETIC ONLY
```

### 6.3 Physical test

The firmware compiled, uploaded, booted, and was operated through KSC Raw TTY
0.3.

Observed startup free SRAM was:

```text
1320 B
```

The dynamic counter was left open in Commander and was observed continuously
changing from 174 through 221. During that run, the Commander reported 1279 B
free SRAM and zero input drops.

The writable node `/demo/value` was observed at 200, edited to 128 through
ordinary CHAR input, and committed with `Status: APPLIED`. LEFT adjustment was
then used to reduce it to 127 and 126.

The Boolean-like `/demo/flag` was changed from 1 to 0 and back to 1, with
`Status: APPLIED` after each write.

### 6.4 Resource result

```text
Flash:                    13658 / 32256 B = 42%
Global SRAM:               669 / 2048 B  = 32%
Boot free RAM:             1320 B
Commander free RAM:        1279 B typical
Minimum observed:          1262 B
Input drops:                  0
External shield required:    no
```

This established that the extracted core did not require the HY-M302 hardware
line to function.

---

## 7. HY-M302 target experiment

### 7.1 Purpose

The second experiment tested the stronger proposition:

> Can the same unchanged KSC_Core operate a physically different target whose
> namespace is backed by real sensors, actuators, and an alternate physical input
> source?

The target was Arduino UNO + HY-M302.

### 7.2 Physical namespace

The target adapter exposed:

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

The adapter used the existing HY_M302 driver library and the previously learned
iDroid/Orange Pi NEC remote profile.

### 7.3 Boot and target identification

Observed boot:

```text
KSC Core 0.1
Hardware-independent ANSI VFS core
Target: Arduino UNO + HY-M302
FREE RAM: 734 B
IR INIT: OK
Target backend: physical HY-M302
Storage: virtual namespace only
```

This established successful target initialization before higher-level
Commander tests.

### 7.4 Live reads

The physical shell test recorded:

```text
/dev/light          328 -> 70 -> 616
/dev/pot            0 -> 582 -> 1023
/dev/dht/temp       26.3
/dev/dht/humidity   30.0
/dev/dht/status     OK
```

The purpose of these values is not metrological characterization. Their purpose
is to demonstrate that the virtual nodes were backed by live physical inputs
and that the values changed under physical interaction.

### 7.5 Physical writes

The shell route produced successful target writes:

```text
WRITE /dev/led/red 1   -> OK
WRITE /dev/led/red 0   -> OK
WRITE /dev/rgb/red 128 -> OK
WRITE /dev/buzzer 1    -> OK
WRITE /dev/buzzer 0    -> OK
```

Physical effects were observed on the corresponding hardware.

In Commander, the red LED node was opened as an RW node with range 0..1. Exact
character entry was used to apply 1 and then 0. The blue LED was also repeatedly
changed through Commander. The same target state remained visible to the shell.

### 7.6 Alternate physical input

The HY-M302 adapter also supplied NEC IR remote events.

The mapping preserved the core semantic model:

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

PC terminal input and IR input therefore drove the same Commander state.

### 7.7 Resource result

```text
Flash:                    20806 / 32256 B = 64%
Global SRAM:              1255 / 2048 B  = 61%
Boot free RAM:             734 B
Commander free RAM:        693 B typical
Minimum observed:          676 B
Input drops:                 0
IR drops:                   0 / 0
```

The HY-M302 target has substantially higher resource cost than the synthetic
Reference Target because it contains physical drivers, DHT handling, asynchronous
IR acquisition, remote decoding, and actuator state.

The key architectural observation is that this extra cost resides outside the
unchanged core.

---

## 8. Same-core identity

The strongest evidence in the current experiment is not that the two builds
look similar; it is that the core source identity was explicitly checked.

The core blobs used during the two-target experiment were:

```text
KSC_Core.h
9190ef5d231acc82498b46a897e32b6e4327ed92

KSC_Core.cpp
211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0
```

The HY-M302 target adapter was added without changing these core files.

The experimental result can therefore be summarized as:

```text
Reference Target  <->  SAME KSC_Core  <->  HY-M302 Target
FULL PASS                                  FULL PASS
```

This is the basis for the bounded hardware-independence claim.

---

## 9. Comparative results

| Metric | Reference Target | HY-M302 Target |
|---|---:|---:|
| Flash | 13,658 B (42%) | 20,806 B (64%) |
| Global SRAM | 669 B (32%) | 1,255 B (61%) |
| Boot free SRAM | 1,320 B | 734 B |
| Commander free SRAM, typical | 1,279 B | 693 B |
| Minimum observed free SRAM | 1,262 B | 676 B |
| Input drops | 0 | 0 |
| Target-specific IR drops | n/a | 0 / 0 |
| Physical shield required | no | yes |
| Dynamic RO nodes | yes | yes |
| RW nodes | yes | yes |
| Physical sensor backing | no | yes |
| Physical actuator backing | no | yes |
| Alternate target input source | no | IR remote |

The approximately 7.1 KB flash and 586 B global-SRAM increase in the HY-M302
build should not be interpreted as core growth. It is target-side cost added by
the physical backend and its drivers.

---

## 10. What the experiment demonstrates

The two-target experiment demonstrates five concrete properties.

### 10.1 UI independence from node implementation

Commander renders directory, RO, and RW semantics without device-specific
rendering branches.

### 10.2 Shell/Commander state identity

The two interfaces observe the same target object state because both route
through the same target adapter.

### 10.3 Input-source independence at the semantic layer

The core consumes CHAR and KEY events. It does not require the Commander to
distinguish whether navigation came from a PC keyboard or the target IR remote.

### 10.4 Namespace independence from physical storage

Both targets operate without an SD card. The namespace therefore exists
independently of a block or file-storage backend.

### 10.5 Target-specific cost remains measurable

The target abstraction does not hide resource costs. The HY-M302 configuration
clearly consumes more flash and SRAM. Hardware independence here means
architectural separation, not zero-cost abstraction.

---

## 11. What the experiment does not demonstrate

The experiment has important limitations.

First, both configurations use the same MCU family and Arduino UNO platform.
Therefore the result does not test CPU-architecture portability.

Second, the current `KscTarget` contract covers directory enumeration,
string-valued reads, bounded numeric writes, dynamic refresh, and injected
input. Byte-stream files, asynchronous remote storage, actions, and event-stream
nodes are not yet part of the validated core claim.

Third, only one physical multifunction target was tested. A relay-only target,
W5100 target, or local LCD/keypad target would exercise different architectural
surfaces.

Fourth, the SRAM measurements are runtime observations from the tested paths,
not a formal stack-depth proof. The minimum observed values are therefore
empirical lower bounds on remaining SRAM for those runs, not global guarantees
over all possible execution traces.

Fifth, drop counters staying at zero during normal operation demonstrates that
the tested interaction did not overload the event paths. It is not a proof of
zero loss under arbitrary input rates.

---

## 12. Reproducibility

The experiment is reproducible from the repository snapshot associated with this
manuscript.

### 12.1 Core

```text
projects/KSC-KonSol-Commander/libraries/KSC_Core/
```

### 12.2 Reference Target

```text
projects/KSC-KonSol-Commander/sketches/05_KSC_Core_Reference/
projects/KSC-KonSol-Commander/tests/KSC-02C-CORE-REFERENCE-TARGET.md
```

### 12.3 HY-M302 Target

```text
projects/KSC-KonSol-Commander/sketches/06_KSC_Core_HY_M302/
projects/KSC-KonSol-Commander/tests/KSC-02D-HY-M302-TARGET-ADAPTER.md
```

### 12.4 Host terminal

```text
projects/KSC-KonSol-Commander/tools/ksc_raw_tty.py
```

### 12.5 Experimental snapshot

```text
96487e66b159df75a1b3162098590cb23ab4af49
```

The build and upload commands are recorded in the two test documents.

A separate reproducibility note accompanies this manuscript.

---

## 13. Relation to KSC-03

The next KSC milestone is intentionally excluded from this paper.

KSC-03 will investigate a host-computer filesystem mounted as `/host` while
the UNO remains interactive over the same serial connection. That problem adds
a new class of concerns:

- serial multiplexing;
- request/response framing;
- streaming byte data;
- remote directory enumeration;
- bounded transfer buffers;
- disconnect/reconnect behavior.

Those concerns are sufficiently different from the present target-abstraction
experiment that they should be evaluated separately.

The current paper therefore stops at the point where the local core has passed
two target backends.

---

## 14. Conclusion

KSC_Core was extracted from an initially HY-M302-centered implementation and
tested as a target-neutral shell, ANSI Commander, VFS-navigation, and input
core for an ATmega328P-class system.

A bare Arduino UNO Reference Target passed with a synthetic namespace and no
external shield. A second Arduino UNO + HY-M302 build passed with real sensors,
real actuators, and an IR remote input source. The same KSC_Core source blobs
were used for both configurations.

The result supports a precise engineering statement:

> **KSC_Core is hardware-independent with respect to the two physically tested
> target adapters.**

This statement is deliberately narrower than universal portability. Its value
is that it is tied to source identity, physical tests, measured resource
envelopes, explicit non-claims, and a reproducible repository snapshot.

The next research question is not whether the Commander can operate another
local node type, but whether the same namespace model can cross a transport
boundary and expose streamed host files without violating the memory discipline
of a 2 KB SRAM machine.

---

## References

1. Arduino, **Arduino UNO Rev3 - Technical Specifications**, official Arduino
   product documentation. Microcontroller: ATmega328P; 32 KB flash; 2 KB SRAM;
   1 KB EEPROM; 16 MHz.
   https://store.arduino.cc/products/arduino-uno-rev3

2. Arduino, **Arduino UNO R3 Datasheet**, A000066, official board datasheet.
   https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf

3. Microchip Technology, **ATmega328P Product Page and Datasheet Resources**.
   Program memory: 32 KB; RAM: 2048 bytes; EEPROM: 1024 bytes.
   https://www.microchip.com/en-us/product/atmega328p

4. Ecma International, **ECMA-48: Control Functions for Coded Character Sets**,
   5th edition, June 1991. The KSC terminal UI uses only a small subset of
   terminal control functions suitable for its ANSI-style screen interaction.
   https://ecma-international.org/publications-and-standards/standards/ecma-48/

5. AIDevelopersMonster, **Arduino-UNO-Shields / KSC - KonSol Commander**,
   project source, test protocols, and physical certification records.
   https://github.com/AIDevelopersMonster/Arduino-UNO-Shields

---

## Appendix A. Compact experimental statement

```text
Platform:
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
  Same KSC_Core source passed both targets.

Non-claim:
  Universal MCU, transport, peripheral, or filesystem portability is not proven.
```

## Appendix B. Suggested publication title in Russian

**KSC_Core: аппаратно-независимое ядро виртуального пространства и ANSI Commander для ресурсно-ограниченных 8-битных систем**

Подзаголовок:

**Двухтаргетная физическая валидация на Arduino UNO: программный Reference Target и плата расширения HY-M302**

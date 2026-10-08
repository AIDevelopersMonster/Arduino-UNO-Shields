# KSC_Core v0.1 - Pre-Publication Audit

**Audit type:** adversarial scientific-engineering review  
**Audited manuscript:** `KSC_CORE_TWO_TARGET_VALIDATION_v0.1.md`  
**Evidence snapshot:** `96487e66b159df75a1b3162098590cb23ab4af49`  
**Audit date:** 2026-10-08  
**Verdict:** PASS AFTER REQUIRED CORRECTIONS  
**Experimental result status:** no critical defect found in the two-target physical result

---

## 1. Executive verdict

The experimental core of the manuscript is publishable.

The strongest supported result is not universal hardware independence. It is a
narrower and reproducible result:

> The same unchanged KSC_Core source was physically exercised through two
> different target backends on Arduino UNO: a synthetic Reference Target and a
> physical HY-M302 target adapter.

The experiment is materially stronger than a design proposal because the
project preserved:

- two physical PASS routes;
- build resource measurements;
- runtime SRAM observations;
- input/IR drop counters;
- exact KSC_Core blob identities;
- an evidence snapshot commit;
- a commit comparison showing that the core files did not change while the
  second target adapter was added.

The manuscript should therefore be published only after replacing broad
`hardware-independent` wording in the title and front matter with a more
precise term such as **target-decoupled** or **target-backend-independent within
the tested Arduino UNO configuration**.

No evidence supports universal MCU independence.

---

## 2. Evidence integrity audit

### 2.1 Same-core identity - PASS

The frozen core blobs are:

```text
KSC_Core.h
9190ef5d231acc82498b46a897e32b6e4327ed92

KSC_Core.cpp
211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0
```

A repository comparison from the KSC_Core library-metadata commit
`d08e1632ed8154af7ad3d45462be883f8e9983c2` to the closed two-target evidence
snapshot `96487e66b159df75a1b3162098590cb23ab4af49` shows additions and documentation
changes for the Reference Target, HY-M302 target adapter, and tests, but no
change to either KSC_Core source file.

This is strong evidence for the central source-identity claim.

### 2.2 Static target-separation check - PASS

Direct inspection of both KSC_Core source files found no:

- `HY_M302` reference;
- DHT-specific code;
- Arduino `String` use;
- `malloc`, `calloc`, or `realloc`;
- explicit `new` expression;
- `pinMode`, `digitalWrite`, `digitalRead`, `analogRead`, or
  `analogWrite` call.

This supports the architectural statement that physical target details are
outside the core.

Important qualification: KSC_Core still depends on the Arduino programming
environment through `Arduino.h`, `Stream`, `millis()`, `F()`, and AVR-oriented
free-RAM instrumentation. Therefore it is **not** platform-independent software.

### 2.3 Reference Target result - PASS

Certified values:

```text
Flash:                    13658 / 32256 B = 42% reported
Global SRAM:               669 / 2048 B  = 32% reported
Boot free RAM:             1320 B
Commander free RAM:        1279 B typical
Minimum observed:          1262 B
Input drops observed:         0
External shield required:    no
```

The dynamic counter, writable scalar, Boolean-like flag, shell, Commander,
navigation, and shared target state were physically exercised.

### 2.4 HY-M302 Target result - PASS

Certified values:

```text
Flash:                    20806 / 32256 B = 64% reported
Global SRAM:              1255 / 2048 B  = 61% reported
Boot free RAM:             734 B
Commander free RAM:        693 B typical
Minimum observed:          676 B
Input drops observed:        0
IR drops observed:          0 / 0
```

Recorded live values included:

```text
/dev/light          328 -> 70 -> 616
/dev/pot            0 -> 582 -> 1023
/dev/dht/temp       26.3
/dev/dht/humidity   30.0
/dev/dht/status     OK
```

Actuator routes included discrete LED, RGB, and buzzer writes.

The purpose of these measurements is architectural/functional validation, not
sensor calibration or metrological accuracy.

---

## 3. Major findings requiring correction

### M1. Title-level `hardware-independent` wording is too broad

**Severity:** major  
**Status in v0.1:** overbroad  
**Required correction:** yes

The two builds use the same Arduino UNO / ATmega328P platform. One has no shield;
the other adds HY-M302. The core also directly depends on Arduino runtime types
and functions.

Therefore a title saying simply **Hardware-Independent Core** invites a reviewer
to read the claim as MCU/platform independence.

Recommended publication title:

> **KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for
> Resource-Constrained 8-bit Systems**

The phrase `hardware-independent` may remain only as a bounded result, e.g.:

> hardware-independent with respect to the two tested target adapters on the
> Arduino UNO platform.

### M2. Prior art / novelty boundary is underdeveloped

**Severity:** major  
**Required correction:** yes

Filesystem-like exposure of system and device state is not novel in itself.
Linux `/proc` and `sysfs` are obvious prior art for the general idea of exposing
internal state or device attributes through a filesystem-shaped interface.

The manuscript must explicitly state:

- it does not claim invention of virtual filesystems;
- it does not claim invention of device attributes as files;
- its contribution is the measured extraction and two-target physical
  validation of this model under a 2 KB SRAM-class constraint.

Add official Linux kernel documentation references for `procfs` and `sysfs`.

### M3. Runtime SRAM measurement method is not defined rigorously enough

**Severity:** major  
**Required correction:** yes

`kscFreeRam()` is an instantaneous estimate derived from the distance between a
stack-local variable and the heap break (or `__heap_start` when no heap break is
active).

Therefore:

- `Boot free RAM` and `Commander free RAM` are runtime observations;
- `minimum observed` is the smallest value seen in the recorded test route;
- none of these is a formal stack high-water mark;
- none proves safety for every possible execution path.

The v0.2 manuscript must define this measurement.

### M4. Reproducibility package lacks a frozen toolchain identity

**Severity:** major for exact rebuild reproducibility  
**Required before final Zenodo publication:** yes

The test documents preserve exact compile commands and compiler output, but the
evidence snapshot does not itself record:

- exact `arduino-cli version`;
- exact installed `arduino:avr` core version;
- exact Python version;
- exact pyserial version.

The project environment is known operationally, but the final publication
package should contain command output produced immediately before deposit.

Required capture commands are included in the v0.2 reproducibility package.

This is not a defect in the physical result; it is a reproducibility metadata
gap.

### M5. Zenodo creator and license metadata are not yet frozen

**Severity:** major publication blocker, not experimental blocker

Zenodo requires creators and a resource type, and requires licensing terms in
the deposit metadata. The repository currently does not contain a project
license that can safely be inferred for the article or source package.

Do not silently assign a license.

Before publication, explicitly confirm:

- creator display name(s);
- ORCID(s), if used;
- article/document license;
- whether code is deposited or only referenced;
- resource type (`Publication / Preprint` is recommended for the manuscript);
- whether a DOI will be reserved before final PDF generation.

---

## 4. Moderate findings

### R1. Clarify flash denominator

Arduino UNO Rev3 has 32 KB total flash, with 0.5 KB used by the bootloader.
The Arduino CLI build denominator in these tests is 32,256 bytes.

The manuscript should distinguish:

- MCU flash: 32 KB total;
- sketch capacity used by the toolchain: 32,256 B.

This prevents a reviewer from treating the two values as inconsistent.

### R2. Use exact byte delta instead of ambiguous `7.1 KB`

The HY-M302 build adds:

```text
20806 - 13658 = 7148 B flash
1255  - 669   = 586 B global SRAM
```

Report the exact byte deltas. Avoid `KB` versus `KiB` ambiguity.

### R3. Current host terminal helper is Windows-specific

`ksc_raw_tty.py` imports `msvcrt` and is therefore Windows-specific in the
tested form.

The core itself should not be presented as dependent on Windows, but the
**reproduction route used in this paper** does depend on the Windows Raw TTY
helper unless another compatible terminal route is separately tested.

Add this to limitations and reproducibility notes.

### R4. Drop-counter wording must remain empirical

Use:

> no input-drop event was observed in the recorded normal-operation route

rather than:

> the system cannot drop input

Likewise, IR 0/0 is an observed result, not a universal throughput guarantee.

### R5. DHT values are functional evidence only

The observed temperature and humidity values demonstrate target-backed reads.
They do not establish DHT11 accuracy, calibration, or environmental
characterization.

The v0.1 manuscript already partly protects this claim; v0.2 should make the
non-metrological purpose explicit.

### R6. `ANSI` should be scoped

The UI uses a practical subset of ECMA-48/ANSI-style terminal control sequences.
Do not imply full ECMA-48 conformance.

Use:

> ANSI/ECMA-48-style terminal control subset

or:

> ANSI-style terminal UI.

---

## 5. Reference audit

### Arduino UNO Rev3

**Verified official source:** Arduino official store/documentation.

Supported facts:

- ATmega328P;
- 32 KB flash;
- 2 KB SRAM;
- 1 KB EEPROM;
- 16 MHz;
- 0.5 KB bootloader reservation on UNO Rev3.

Recommended source:

```text
https://store.arduino.cc/products/arduino-uno-rev3
```

Official board datasheet:

```text
https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf
```

### ATmega328P

**Verified official source:** Microchip.

Supported facts:

- 8-bit AVR;
- 32 KB program memory;
- 2048 B RAM;
- 1024 B EEPROM.

Current Microchip product page also marks ATmega328P **Not Recommended for new
designs**. This lifecycle status is not necessary to the experiment, but may be
mentioned as historical/platform context if desired.

```text
https://www.microchip.com/en-us/product/ATmega328P
```

### ECMA-48

**Verified official source:** Ecma International.

The current official page identifies ECMA-48, fifth edition, June 1991, and
describes control functions for character-coded data and character-imaging
devices.

```text
https://ecma-international.org/publications-and-standards/standards/ecma-48/
```

### Filesystem-shaped system/device interfaces

Add official Linux kernel references:

```text
https://www.kernel.org/doc/html/latest/filesystems/proc.html
https://www.kernel.org/doc/html/latest/filesystems/sysfs.html
```

These references are used only to establish prior-art context, not equivalence.

---

## 6. Claim matrix

| Claim | Evidence | Audit result |
|---|---|---|
| KSC_Core runs without HY-M302 | Reference Target physical PASS | PASS |
| KSC_Core drives HY-M302 through adapter | KSC-02D physical PASS | PASS |
| Same core source used | core blob SHA identity + commit comparison | PASS |
| Core contains no HY-M302-specific implementation | static source inspection | PASS |
| Shell and Commander share target abstraction | source architecture + physical route | PASS |
| PC and IR can feed same semantic input layer | source mapping + KSC-02/KSC-02D physical PASS | PASS |
| No SD required for tested namespace | both physical configurations | PASS |
| Universal hardware independence | not tested | REJECT |
| Cross-MCU portability | not tested | REJECT |
| POSIX filesystem compatibility | not claimed/tested | REJECT |
| Formal worst-case SRAM safety | not tested | REJECT |
| Lossless input under arbitrary rate | not tested | REJECT |
| Sensor metrological accuracy | not tested | REJECT |

---

## 7. Required v0.2 changes

The publication candidate should:

1. change the title from `Hardware-Independent` to `Target-Decoupled`;
2. preserve the bounded two-target hardware-independence statement in the
   conclusion;
3. add prior-art context for procfs/sysfs and explicitly disclaim novelty of the
   filesystem-interface concept;
4. define `kscFreeRam()` measurement semantics;
5. clarify the 32 KB total-flash versus 32,256 B sketch-capacity denominator;
6. replace approximate target-cost differences with exact byte differences;
7. state that the test terminal helper is Windows-specific;
8. weaken drop-counter wording to observed-test wording;
9. add evidence provenance and exact source blob identifiers;
10. add a reproducibility section with toolchain-capture commands;
11. prepare Zenodo metadata without inventing creator or license information;
12. keep KSC-03 `/host` outside the experimental result.

---

## 8. Publication decision

**Scientific-engineering result:** ACCEPTABLE AFTER REVISION

**Reason:** the central experiment has a clear falsifiable architecture,
physical evidence, explicit resource limits, and exact source identity.

**Main publication risk:** terminology, not experiment.

The paper becomes defensible when it says:

> target-decoupled core, physically validated against two target backends on the
> same Arduino UNO platform

and avoids saying:

> universally hardware-independent core.

---

## 9. Final audit status

```text
Experimental integrity:        PASS
Source identity:               PASS
Claim discipline v0.1:        NEEDS REVISION
Resource table:                PASS WITH CLARIFICATIONS
Reference verification:       PASS WITH ADDITIONS
Reproducibility:               CONDITIONAL PASS
Zenodo metadata readiness:     BLOCKED ON CREATOR/LICENSE
KSC-03 separation:             PASS
Overall manuscript decision:   REVISE -> v0.2 PUBLICATION CANDIDATE
```


---

## 10. Post-audit metadata resolution

After the v0.1 audit, the following publication metadata were supplied:

```text
Creator:
A. A. Malachevsky

ORCID:
0009-0008-6009-3196

Zenodo DOI:
10.5281/zenodo.23232216
https://doi.org/10.5281/zenodo.23232216
```

Resolved audit blockers:

- creator identity: RESOLVED;
- creator order: RESOLVED (single creator);
- ORCID: RESOLVED;
- DOI: RESOLVED and inserted into v0.2 publication candidate.

Remaining pre-deposit/final-release blockers:

- publication/document license must be explicitly selected;
- final toolchain capture must be recorded;
- final PDF must be generated and visually audited;
- if source code is uploaded as a licensed software artifact, its software
  license must be decided separately.

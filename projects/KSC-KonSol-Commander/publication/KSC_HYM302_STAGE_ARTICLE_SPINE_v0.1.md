# KSC / KonSol Commander with HY-M302 — Stage Article Spine v0.1

## Publication status

This document defines an **intermediate/stage article**, not the final article of
the KSC programme.

It freezes the scientific and engineering meaning of the completed
**Arduino UNO / ATmega328P + HY-M302 target phase** while explicitly keeping
the wider KSC programme open for substantially different shields, MCU targets,
and single-board computers.

The article should not be framed as "the final KSC architecture". Its role is
to document one demanding physical realization of KSC, the architectural
mechanisms that were exercised, the bounded claims that were physically
certified, and the resource limits encountered on the ATmega328P.

---

# Working title

**KonSol Commander on Arduino UNO with HY-M302: A Resident Cooperative
Environment with a Unified Virtual Namespace, Streamed Host Files, and
Loadable Runtime Behavior**

Alternative shorter title:

**KonSol Commander on ATmega328P: Unified VFS, Streamed Remote Programs, and
Runtime-Reconfigurable Control with the HY-M302 Shield**

## Subtitle

**A stage report on KSC execution under 32 KB Flash and 2 KB SRAM**

---

# Core thesis

The article should be organized around the following experimentally supported
statement:

> On an ATmega328P with 32 KB Flash and 2 KB SRAM, a resident cooperative
> environment was built in which local physical devices, system state, and
> remote PC files are represented inside one VFS-like namespace; large host
> files are read by bounded streaming, remote `.KSC` programs are executed
> without loading the whole program into SRAM, and a program loaded from the PC
> can change the subsequent behavior of a physical input without recompilation,
> firmware upload, or reset.

This thesis is deliberately narrower than a claim of a general-purpose
operating system, general-purpose script language, or universal event-binding
system.

---

# Abstract — intended content

This work reports a physically validated implementation of KonSol Commander
(KSC) on an Arduino UNO / ATmega328P equipped with an HY-M302 multifunction
shield. The system operates under 32 KB of Flash and 2 KB of SRAM and uses a
resident cooperative execution model rather than a general-purpose operating
system model. KSC exposes local devices, system state, and PC-hosted files
through a single filesystem-like namespace containing `/dev`, `/proc`,
`/sys`, and `/host`. A target-decoupled core provides shell, ANSI Commander,
path handling, unified CHAR/KEY input, and navigation, while target adapters
provide physical-device semantics and remote host access.

A single serial link is multiplexed between terminal traffic and a framed
host-filesystem protocol. Host-backed files are accessed with bounded READ
chunks of at most 32 bytes. Explicit-offset reads allow idempotent retry after
a lost response and reopen/resume after an invalidated host handle. A
Commander file viewer adds 192-byte logical windows without allocating a
192-byte page buffer. Remote `.KSC` files can then be selected for VIEW or RUN.
The KSC Script v0.1 executor streams the file and evaluates one bounded command
line at a time using only PRINT, WRITE, WAIT, and STOP.

The HY-M302 target demonstrates the physical consequence of this architecture:
LEDs, RGB channels, an active buzzer, sensors, buttons, and an IR remote are
mapped into the VFS. In the final stage experiment, one remote script selects
`SW1 -> RED`, while another selects `SW1 -> BLUE`; the same physical SW1
button therefore performs a different action after a different program is
loaded, without recompiling, uploading, or resetting the Arduino. The final
certified image uses 31,024 of 32,256 bytes of program storage, 1,556 of 2,048
bytes of global SRAM, and retains approximately 380 bytes of free runtime RAM
during Commander operation. The result is presented as a stage validation of
the KSC architecture on one highly constrained physical target, not as closure
of the wider KSC programme.

---

# 1. Scope and status of this article

This article must open by stating exactly what it is and what it is not.

It is:

- a stage report for the Arduino UNO / ATmega328P + HY-M302 realization;
- a continuation beyond the previously published KSC_Core two-target result;
- an experimental account of a complete target phase from VFS through remote
  program execution and runtime reconfiguration;
- a resource-constrained architecture study.

It is not:

- the final KSC programme article;
- a claim that HY-M302 is the definitive KSC target;
- a claim that all future shields will reuse the same implementation unchanged;
- a claim of a general-purpose or protected operating system.

Recommended wording:

> The present paper closes one target phase, not the KSC programme. The
> HY-M302 realization is used because it combines heterogeneous sensors,
> actuators, local buttons, and IR input in a single resource-constrained
> platform, making it a useful stress target for the KSC abstraction boundary.

---

# 2. Research question and hardware constraints

State the concrete research question:

> Can a very small 8-bit resident cooperative environment expose physical
> devices, kernel/system state, persistent or remote resources, and executable
> host-side programs through one navigable namespace while remaining usable
> under the Flash and SRAM limits of the ATmega328P?

Hardware constraints:

- MCU: ATmega328P;
- board: Arduino UNO;
- Flash available to sketch: 32,256 bytes;
- SRAM: 2,048 bytes;
- one primary Serial/COM channel for terminal and host protocol;
- HY-M302 target hardware;
- no assumption that a whole host file or whole external program fits in SRAM.

The hardware section should explicitly explain why this is a meaningful
constraint experiment: the final implementation reached 96% of available
program Flash while still leaving a physically usable runtime RAM margin.

---

# 3. System model: one namespace for different kinds of objects

Introduce the KSC object model:

```text
/
+-- dev/
+-- proc/
+-- sys/
+-- host/
```

Explain the conceptual role:

- `/dev` maps physical inputs and outputs;
- `/proc` exposes live system state such as memory and uptime;
- `/sys` exposes configuration/state objects;
- `/host` maps files physically stored on the PC.

The central architectural claim here is not POSIX compatibility. It is
**response-equivalent navigability**: different physical and remote resources
can be addressed, inspected, and acted upon through the same path-oriented KSC
interface.

Use examples:

```text
/dev/sw1
/dev/led/red
/dev/rgb/green
/dev/dht/temp
/proc/mem
/sys/sw1
/host/DEMOS/SOS.KSC
```

---

# 4. KSC_Core and the target boundary

Summarize the reusable core:

- shell;
- ANSI Commander;
- TTY decoding;
- unified input queue;
- path helpers;
- directory navigation;
- node rendering;
- bounded file-viewer state;
- launcher/action semantics.

Explain the `KscTarget` boundary and the reason for keeping target logic
outside the core.

Connect conservatively to the earlier KSC_Core publication:

- the earlier paper physically validated the same core concept using a synthetic
  Reference Target and HY-M302 target;
- the current article extends the physical realization substantially;
- it does not retroactively change the bounded claims of the earlier publication.

Do not use "platform independent" without qualification. Preferred wording:

> target-decoupled within the tested Arduino UNO / AVR implementation model.

---

# 5. Unified human input: terminal and IR

Describe the input abstraction:

```text
INPUT EVENT
+-- CHAR
+-- KEY
```

Sources:

- TTY/PC keyboard;
- HY-M302 IR remote.

Important distinction:

- printable terminal bytes become CHAR;
- arrows and control functions become KEY;
- IR digits become CHAR;
- IR navigation buttons become KEY.

Explain why this matters: Commander logic is driven by semantic input events,
not by a particular keyboard transport.

Include the final coexistence result:

```text
IN drop 0
IR drop 0/0
```

Also document the cooperative servicing fix: host exchanges and script WAIT
periods must continue to service the IR decoder and drain completed input
events.

---

# 6. ANSI Commander

Describe Commander as the primary user-facing navigator over the VFS.

Directory example:

```text
Path: /host/DEMOS
----------------------------------------
  ALARM.KSC  [RO]
  BEEP.KSC   [RO]
  POLICE.KSC [RO]
  ...
----------------------------------------
UP/DOWN Select
ENTER/RIGHT Open
LEFT/BACK Parent
```

Describe:

- RO and RW nodes;
- direct physical-node editing;
- inverse selection;
- node refresh;
- shell/Commander coexistence;
- low persistent state.

The article should stress that Commander is not just a cosmetic UI. It is the
interactive projection of the same VFS used by the shell and launcher.

---

# 7. One serial link: TTY + host filesystem multiplexing

Describe the host framing:

```text
+------+------+------+------+------+---------+------+
| SOF1 | SOF2 | TYPE | SEQ  | LEN  | PAYLOAD | CRC8 |
+------+------+------+------+------+---------+------+
  0x1B   0x5D    1 B    1 B    1 B   0..48 B  1 B
```

Protocol operations:

- PING;
- MOUNT;
- LS;
- STAT;
- OPEN;
- READ;
- CLOSE;
- ERROR.

CRC:

- CRC-8/ATM;
- polynomial 0x07;
- bounded payload;
- maximum frame size 54 bytes.

Explain the multiplexing idea:

- ordinary serial bytes remain terminal traffic;
- framed traffic is intercepted as HOSTFS;
- host protocol frames do not leak into terminal text.

---

# 8. Remote `/host` mount

Explain how the PC-side host directory becomes a remote KSC directory.

The PC remains the storage owner. Arduino does not mirror the whole directory
or whole files into SRAM.

Responsibilities:

```text
PC host
  -> directory enumeration
  -> file metadata
  -> file handles
  -> byte ranges

Arduino
  -> VFS projection
  -> navigation
  -> bounded read requests
  -> rendering/execution
```

This section should introduce the distinction between **namespace residency**
and **data residency**: a file can exist as a visible KSC object without its
content being resident in AVR memory.

---

# 9. Bounded streaming and recovery

Describe the decisive KSC-03 result.

Transport read size:

```text
READ <= 32 B
```

Physical proof file:

```text
BIG.TXT ~12 KB
```

The complete file cannot fit in 2 KB SRAM, yet it is emitted completely through
the namespace.

Then describe the recovery refinement:

Original form:

```text
READ(handle, length)
```

Certified form:

```text
READ(handle, offset, length)
```

Recovery cases:

1. lost READ response:
   - retry the same handle + same offset;
2. BAD_HANDLE:
   - reopen the same path;
   - resume at the same explicit offset.

State the experimental fault-injection results and distinguish deliberate
recovery counters from final errors.

Representative final KSC-03D status:

```text
RAM 463 B   IN drop 0   IR drop 0/0   HOST M/0 R2
```

The `R2` corresponds to deliberately injected recovered faults.

---

# 10. Commander File Viewer

Describe KSC-04A.

Logical viewer page:

```text
192 B
```

Physical transport chunk:

```text
<= 32 B
```

Important memory property:

> the viewer does not allocate a 192-byte page buffer.

Navigation:

- previous;
- next;
- HOME;
- END;
- BACK.

Physical example:

```text
FILE 0/12445
FILE 192/12445
...
FILE 12288/12445
```

Final KSC-04A runtime result:

```text
RAM 437 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

Explicit limitation:

- byte-window boundaries can split text lines;
- line-aware beautification was intentionally deprioritized because the viewer
  is a preview/action layer, not the final purpose of KSC.

---

# 11. File Actions and streamed KSC programs

Introduce KSC-04B.

For ordinary files:

```text
VIEW
```

For `.KSC` files:

```text
ACTIONS
> VIEW
  RUN
```

KSC Script v0.1 instructions:

```text
PRINT
WRITE
WAIT
STOP
```

Execution model:

```text
/host/program.KSC
      |
      v
bounded host stream
      |
      v
one bounded command line
      |
      v
execute immediately against VFS
```

The whole program is never loaded into SRAM.

The article should emphasize the consequence:

> program storage and program execution are separated: the PC can hold the
> external program while the AVR keeps only the resident interpreter/state
> necessary to consume it incrementally.

Physical KSC-04B result:

```text
RUN OK
RAM 399 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

---

# 12. HY-M302 specialization

This is the first of only two sections that should focus heavily on this
particular shield.

Describe the physical mapping used in the article:

Inputs:

- SW1;
- SW2;
- potentiometer;
- LDR/light input;
- DHT11;
- IR receiver.

Outputs:

- red discrete LED;
- blue discrete LED;
- RGB red/green/blue channels;
- active/self-oscillating buzzer.

Representative VFS nodes:

```text
/dev/sw1
/dev/sw2
/dev/pot
/dev/light
/dev/dht/temp
/dev/dht/humidity
/dev/led/red
/dev/led/blue
/dev/rgb/red
/dev/rgb/green
/dev/rgb/blue
/dev/buzzer
```

Explain that the active buzzer supports rhythm by on/off timing but should not
be described as a frequency-accurate melody synthesizer in the certified
configuration.

List the demonstration programs:

```text
BEEP.KSC
SOS.KSC
RGB.KSC
TRAFFIC.KSC
ALARM.KSC
POLICE.KSC
SHOW.KSC
RESET.KSC
SW1_0.KSC
SW1_1.KSC
```

This section should show that multiple behaviors are produced by changing
external programs rather than rebuilding the firmware.

---

# 13. Physical results on the HY-M302 target

This is the second and final shield-specialization-heavy section.

## 13.1 Runtime-reconfigurable SW1 behavior

Resident configuration node:

```text
/sys/sw1
```

Certified meanings:

```text
/sys/sw1 = 0 -> SW1 press: RED ON, BLUE OFF
/sys/sw1 = 1 -> SW1 press: BLUE ON, RED OFF
```

Remote programs:

`SW1_0.KSC`

```text
PRINT SW1_0 LOAD RED PROFILE
WRITE /sys/sw1 0
PRINT PRESS SW1 FOR RED
STOP
```

`SW1_1.KSC`

```text
PRINT SW1_1 LOAD BLUE PROFILE
WRITE /sys/sw1 1
PRINT PRESS SW1 FOR BLUE
STOP
```

Certified physical sequence:

```text
RUN SW1_0.KSC
PRESS SW1
=> RED ON, BLUE OFF

RUN SW1_1.KSC
PRESS same SW1
=> BLUE ON, RED OFF
```

No recompilation, upload, or reset occurs between the two loaded behaviors.

This is the strongest stage-specific experiment and should be highlighted in
the abstract, results, and conclusion.

## 13.2 Demo-program regression

Record that the complete demo set was physically exercised.

The long-comment parser bug found in SOS/POLICE should be mentioned as a useful
engineering failure:

- command-line buffer is bounded;
- non-executable comments should not consume the command buffer;
- parser was corrected to discard the remainder of a comment line;
- IR frame draining was also corrected during long WAIT/RUN periods.

Final representative observation:

```text
SOS START
SOS PASS
RUN OK
RAM 380 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

## 13.3 Final target resource point

Final certified image:

```text
Flash       31024 / 32256 B = 96%
Global SRAM  1556 / 2048 B = 75%
Flash headroom              = 1232 B
Compiler SRAM remainder     = 492 B
Observed Commander free RAM ~ 380 B
```

Interpretation:

- the specific HY-M302 KSC image is feature-saturated;
- that fact is a result of this target realization, not a closure of KSC;
- no further features should be added to this image merely to improve UI
  polish.

---

# 14. Resource evolution across the stage

Include a compact table similar to the following.

| Stage | Flash (B) | Globals (B) | Typical runtime free RAM |
|---|---:|---:|---:|
| KSC-01A VFS/TTY | 10,932 | 995 | ~1,047 |
| KSC-02 Commander | 18,026 | 1,172 | ~782 |
| KSC-02D HY-M302 adapter | 20,806 | 1,255 | ~693 |
| KSC-03A Host transport | 22,986 | 1,396 | ~593 |
| KSC-03B Host mount | 25,630 | 1,479 | ~469 |
| KSC-03C Stream CAT | 26,330 | 1,483 | ~465 |
| KSC-03D Recovery | 26,712 | 1,485 | ~463 |
| KSC-04A File Viewer | 28,816 | 1,499 | ~437 |
| KSC-04B Launcher | 30,630 | 1,537 | ~399 |
| KSC-04C final HY-M302 | **31,024** | **1,556** | **~380** |

Explain why this table matters:

- it shows the cost of each architectural layer;
- it makes the physical resource boundary reproducible;
- it documents that features were added incrementally and measured rather than
  assumed to fit;
- it gives later targets a baseline for comparing the cost of the same
  abstractions.

---

# 15. Claim discipline and explicit non-claims

The paper should contain a dedicated non-claims section.

## Supported claims

The physically supported claims include:

1. a unified VFS-like namespace can project both local physical nodes and
   remote host files on this target;
2. a single serial link can coexist as terminal and framed host transport;
3. host files larger than AVR SRAM can be consumed through bounded streaming;
4. explicit-offset reads support bounded retry and reopen/resume recovery for
   the tested lost-response and BAD_HANDLE faults;
5. Commander can preview remote files without a whole-file or whole-page
   buffer;
6. remote `.KSC` programs can execute incrementally without whole-program
   SRAM residency;
7. a remote program can modify a resident runtime profile that changes the
   later behavior of the same physical input;
8. the complete tested target remains operational near the ATmega328P Flash
   limit.

## Explicit non-claims

Do not claim:

- a general-purpose OS;
- memory protection;
- preemptive multitasking;
- arbitrary native AVR binary loading;
- arbitrary binary-safe file viewing;
- a general-purpose programming language;
- arbitrary conditions, loops, variables, or expressions in KSC Script v0.1;
- a general event-binding engine;
- persistent runtime profiles across reset;
- concurrent arbitrary host-file opens;
- universal hardware independence;
- that all future KSC targets will fit the same UNO image.

Preferred overall term:

> **resident cooperative environment with separately stored external
> programs/resources**

---

# 16. Reproducibility package

The stage article should end its technical body with a reproducibility section.

Repository:

```text
AIDevelopersMonster/Arduino-UNO-Shields
```

Project:

```text
projects/KSC-KonSol-Commander
```

Important components:

```text
libraries/KSC_Core
libraries/KSC_HostTransport
libraries/KSC_Target_HostMount
libraries/KSC_Target_HY_M302
tools/ksc_host.py
host-share/DEMOS
tests/
```

Final HY-M302 launcher firmware:

```text
sketches/12_KSC_04B_FileActionsLauncher
```

Final demonstration video:

```text
https://youtu.be/pqV5DG1o-WA
```

The article should give exact Arduino CLI compile/upload commands and identify
the physical test documents for each stage.

A reproducibility appendix should distinguish:

- build-time measurements;
- compiler SRAM remainder;
- runtime free-RAM readings;
- deliberate recovery counters;
- actual error counters.

---

# 17. Programme continuation — one section only

Future work should be deliberately confined to this single section so that the
article remains a report of the completed HY-M302 stage rather than becoming a
roadmap document.

The wider KSC programme remains open.

The next scientific question is not "how many more features can be forced into
the saturated HY-M302 image?" It is:

> How much of the KSC abstraction survives when the target interaction model
> changes substantially?

Priority target classes:

## LCD Keypad Shield

Scientific value:

- local character display instead of PC terminal as the visible UI;
- local analog-keypad input instead of IR/PC keyboard;
- test whether Commander semantics survive when rendering and navigation are
  both local to the shield.

Possible question:

> Can the same namespace/Commander semantics be projected onto a 16x2-style
> local display and a small keypad without making the PC terminal the primary
> human interface?

## Relay / control shield

Scientific value:

- many homogeneous writable nodes;
- control profiles;
- event/action mappings;
- bridge toward PLC-like semantics and later RS-485/Modbus work.

## W5100 + SD

Scientific value:

- physical local filesystem;
- network transport;
- comparison of local SD, remote host, and network resources inside one KSC
  namespace.

## Other MCU and SBC targets

Later targets may include ESP32, RP2040, STM32, or Linux SBC systems.

The purpose is not merely to obtain more memory. The purpose is to test which
parts of KSC are architectural invariants and which are artifacts of the AVR
implementation.

The final cross-target KSC article should be written only after several such
substantially different target phases exist.

---

# Suggested figures

1. **KSC layered architecture**
   ```text
   Commander / Shell
          |
       KSC_Core
          |
    KscTarget boundary
      /          \
   HY-M302     HostMount
   ```

2. **Unified namespace tree**
   ```text
   /dev /proc /sys /host
   ```

3. **Single-COM multiplexing diagram**
   terminal bytes + framed HOSTFS protocol.

4. **Explicit-offset recovery sequence**
   normal READ, lost response retry, BAD_HANDLE reopen/resume.

5. **Streamed launcher diagram**
   PC file -> 32 B chunks -> bounded line -> VFS action.

6. **Runtime behavior reconfiguration**
   ```text
   SW1_0.KSC -> /sys/sw1=0 -> SW1 -> RED
   SW1_1.KSC -> /sys/sw1=1 -> SW1 -> BLUE
   ```

7. **Resource growth chart**
   KSC-01A through KSC-04C.

---

# Suggested tables

1. Hardware and software constraints.
2. Namespace classes and representative nodes.
3. Host protocol message types.
4. Physical certification milestones.
5. Resource evolution.
6. Supported claims vs explicit non-claims.
7. Reproducibility files and test documents.

---

# Conclusion — intended wording

The conclusion should close only the target phase.

Recommended conclusion logic:

1. The experiment demonstrates that a coherent VFS/Commander/remote-program
   architecture can be realized on ATmega328P under severe memory limits.
2. Large external resources need not be resident in SRAM to become usable KSC
   objects.
3. Remote programs can be streamed and can modify later physical behavior.
4. The HY-M302 image reached a practical Flash saturation point, making further
   feature accumulation on this image scientifically less valuable.
5. The result therefore justifies moving to structurally different targets,
   not declaring the KSC programme complete.

Recommended final sentence:

> The HY-M302 realization should therefore be read as a completed constrained
> target experiment inside an open KSC programme: it establishes that the core
> abstractions survive one demanding 8-bit physical realization, while the
> next stage must test whether those abstractions remain stable when the
> display, input, storage, network, and control backends change.

---

# Publication discipline

Before producing the manuscript:

- do not call the paper "final KSC";
- do not broaden HY-M302 results into unsupported universal claims;
- preserve exact build/runtime numbers from physical tests;
- separate previously published KSC_Core claims from new stage claims;
- use the final KSC-04 video as evidence of the stage realization;
- keep future work to Section 17 only;
- keep HY-M302-specific description/results concentrated primarily in Sections
  12 and 13;
- let the majority of the article explain the architecture and mechanisms that
  were exercised by the target.

# KSC - KonSol Commander

## Status

**KSC-02D MULTI-TARGET FULL PHYSICAL PASS.**

Certified milestones:

- KSC-01A - virtual VFS / TTY core: FULL PHYSICAL PASS;
- KSC-01B - unified physical keyboard sources: FULL PHYSICAL PASS;
- KSC-01C - CHAR vs KEY semantic input model: FULL PHYSICAL PASS;
- KSC-02 - one-panel ANSI KonSol Commander: FULL PHYSICAL PASS;
- KSC-02C Reference Target - bare UNO synthetic VFS: FULL PHYSICAL PASS;
- KSC-02D - HY-M302 adapter on unchanged KSC_Core: FULL PHYSICAL PASS;

Core-independence experiment status: **COMPLETE FOR TWO TESTED TARGETS**.

Both the bare-UNO Reference Target and the physical HY-M302 Target are **FULL PHYSICAL PASS** on the same unchanged KSC_Core. KSC-03 - Host Filesystem Mount is now the next implementation milestone. The two-target result is sufficient to support a bounded hardware-independence article claim, while explicitly excluding universal portability.

KSC-03 - Host Filesystem Mount remains the next transport/backend milestone after the core-independence experiment.

Publication status: **KSC_Core v0.2 PUBLICATION CANDIDATE** after adversarial
pre-publication audit. The article now uses the narrower term
**target-decoupled** in its title and preserves the bounded two-target
hardware-independence claim only for the physically tested Arduino UNO target
backends.

Publication package:

~~~text
projects/KSC-KonSol-Commander/publication/
+-- KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md
+-- audit/
|   +-- KSC_CORE_v0.1_PREPUBLICATION_AUDIT.md
+-- zenodo/
    +-- README.md
    +-- REPRODUCIBILITY.md
    +-- ZENODO_METADATA_TEMPLATE.md
    +-- SOURCE_MANIFEST.txt
~~~


KSC is a new experimental branch of the Arduino UNO & Shields project.

It is not a continuation of the frozen KonSol 0.8 firmware and does not promise
binary compatibility with KAP1/KAP2, HOST1, the MAR2406 UI, or the KonSol 0.8
filesystem implementation.

KSC may reuse ideas, measurements, engineering lessons, and selected code from
earlier KonSol work, but its architecture is free to change.

Initial physical target:

- Arduino UNO / ATmega328P;
- HY-M302 multi-purpose shield;
- USB Serial terminal at 115200 baud;
- HY-M302 IR receiver as an alternate keyboard;
- no SD card required.

Working name:

**KSC - KonSol Commander**

The intended user interface is a deliberately small terminal file commander,
inspired by the interaction model of classic file managers such as Norton
Commander, but reduced to what is useful and measurable on an ATmega328P.

---

## Video demonstration

**KSC_Core on Arduino UNO - one core for a software Target and HY-M302: VFS, Commander and IR remote**

https://youtu.be/QRReKMaaRMk

The video demonstrates the two-target KSC_Core result: the same core operating
with the synthetic Reference Target and with the physical HY-M302 target
adapter, including the VFS, ANSI Commander, PC keyboard, and IR remote path.

---

## 1. Core idea

KSC treats hardware, kernel information, configuration, and optional external
storage through one navigable namespace.

The first important distinction is:

> a path does not have to represent bytes physically stored in a disk file.

Examples:

```text
/dev/light
/dev/pot
/dev/dht/temp
/dev/dht/humidity
/dev/rgb/red
/dev/led/red
/dev/buzzer
/dev/ir/last

/proc/mem
/proc/tasks
/proc/uptime

/sys/version
/sys/drivers
/sys/resources
```

Reading:

```text
CAT /dev/light
```

may call the HY-M302 light-sensor driver.

Writing:

```text
WRITE /dev/led/red 1
```

may switch a physical LED.

The path is therefore a stable system interface. The backing object may be:

- a live hardware value;
- a writable actuator;
- a kernel-generated value;
- an event source;
- EEPROM-backed persistent data;
- a real SD file;
- a file streamed from an external host computer;
- later, a remote object reached through Ethernet.

---

## 2. KSC is terminal-first

The first KSC does not require a local graphical display.

Primary output:

```text
USB Serial -> ANSI terminal
```

Primary input:

```text
PC keyboard -> terminal escape/key sequences
```

Secondary input on HY-M302:

```text
IR remote -> NEC decoder -> logical key events
```

Both input sources feed the same logical keyboard layer:

```text
PC UP arrow ---------+
                     |
IR UP ---------------+--> KEY_UP

PC DOWN arrow -------+
                     |
IR DOWN -------------+--> KEY_DOWN

PC Enter ------------+
                     |
IR OK ----------------+--> KEY_ENTER

PC Backspace/Esc ----+
                     |
IR RETURN ------------+--> KEY_BACK
```

The UI must not care which physical source generated the key.

Suggested logical keys:

```text
KEY_UP
KEY_DOWN
KEY_LEFT
KEY_RIGHT
KEY_ENTER
KEY_BACK
KEY_HOME
KEY_MENU
KEY_0 .. KEY_9
KEY_POWER
```

The existing learned HY-M302 remote already provides a useful physical set:

```text
0 1 2 3 4 5 6 7 8 9
OK HOME RETURN MENU
UP DOWN LEFT RIGHT
POWER
```

---

## 3. KSC Commander UI

The first Commander should be one-panel, not a full two-panel clone.

Example:

```text
KSC 0.1 - KonSol Commander
Path: /dev

  [..]
> dht/
  rgb/
  led/
  ir/
  buzzer
  light
  pot
  sw1
  sw2

UP/DOWN Select   OK Open
LEFT Back        MENU Actions
HOME /           Q Shell
RAM: 14xx B
```

Opening `dht/`:

```text
Path: /dev/dht

> temp
  humidity
  status
  age
```

Opening a read-only value:

```text
/dev/dht/temp

30.2 C

RETURN Back
```

Opening a writable value:

```text
/dev/rgb/red

Value: 128

LEFT/RIGHT Change
0..9 Enter value
OK Apply
RETURN Cancel
```

A two-panel mode may be evaluated later, but it is not required for the first
architecture proof.

---

## 4. Virtual filesystem model

The first VFS should expose object semantics explicitly.

Candidate node types:

```text
DIR      directory
RO       readable value
RW       readable/writable value
EVENT    event stream or queued event
ACTION   executable system action
FILE     ordinary byte stream
```

Examples:

```text
/dev/light          RO
/dev/pot            RO
/dev/dht/temp       RO
/dev/led/red        RW
/dev/rgb/red        RW
/dev/buzzer         RW
/dev/ir/last        RO
/dev/ir/event       EVENT
/sys/reboot         ACTION
/host/readme.txt    FILE
```

The KSC core should use one VFS dispatch layer rather than hard-coding each
device into the shell or Commander.

Conceptual API:

```text
vfsResolve(path)
vfsList(path)
vfsRead(path)
vfsWrite(path, value/data)
vfsAction(path)
```

On AVR the implementation may use compact numeric node IDs internally. Human
path strings are an interface, not a requirement that every lookup use large
dynamic strings.

---

## 5. Initial namespace

Proposed first tree:

```text
/
+-- dev/
|   +-- sw1
|   +-- sw2
|   +-- pot
|   +-- light
|   +-- dht/
|   |   +-- temp
|   |   +-- humidity
|   |   +-- status
|   |   +-- age
|   +-- led/
|   |   +-- red
|   |   +-- blue
|   +-- rgb/
|   |   +-- red
|   |   +-- green
|   |   +-- blue
|   +-- buzzer
|   +-- ir/
|       +-- last
|       +-- event
|       +-- stats
|
+-- proc/
|   +-- mem
|   +-- tasks
|   +-- uptime
|
+-- sys/
|   +-- version
|   +-- drivers
|   +-- resources
|
+-- cfg/
|
+-- eeprom/
|
+-- host/       optional remote mount
```

The first three trees, `/dev`, `/proc`, and `/sys`, require no physical
filesystem at all.

---

## 6. HY-M302 is the first target, not the definition of KSC

HY-M302 is useful because the repository already contains physically tested
drivers for multiple different I/O classes.

The first adapter can expose:

- SW1 / SW2;
- potentiometer;
- LDR;
- DHT11;
- RGB LED;
- discrete LEDs;
- active buzzer;
- asynchronous NEC IR;
- expansion GPIO where appropriate.

KSC itself should not depend on HY-M302-specific pin numbers.

Target separation:

```text
                KSC core
                   |
          VFS + TTY + Commander
                   |
             target adapter
                   |
      +------------+-------------+
      |            |             |
   HY-M302      W5100       Relay shield
                                  ...
```

This is a new architecture line. Existing HY_M302 library code may be used as a
driver/HAL source, but KSC is not required to preserve the library's public API.

---

## 7. No SD card is required

Lack of SD is not considered a defect in the KSC architecture.

KSC distinguishes the namespace from the storage backend.

Possible backends:

```text
virtual        /dev /proc /sys
EEPROM         /eeprom or /cfg
host computer  /host
SD card        /sd
network        /net or remote mount
```

This allows the first KSC to be fully useful on a board with no local mass
storage.

---

## 8. External host as a file source

The PC connected by USB Serial can act as a remote filesystem server.

Concept:

```text
Arduino UNO / KSC
        |
        | serial framed requests
        v
KSC Host
        |
        +-- terminal frontend
        +-- keyboard input
        +-- exported PC directory
        +-- optional file editor/tooling
```

Example mount:

```text
/host
  README.TXT
  CONFIG/
  APPS/
  DATA/
```

From KSC:

```text
LS /host
CAT /host/README.TXT
```

The bytes stay on the PC and are streamed only when required.

The UNO must not load a whole host file into SRAM.

Target transfer model:

```text
open
 -> read small chunk
 -> consume/render
 -> request next chunk
 -> close
```

This turns a normal PC directory into optional external storage for a machine
that has no SD card.

---

## 9. Serial multiplexing requirement

A normal serial terminal owns the COM port, so a remote `/host` filesystem
cannot simultaneously be implemented by an unrelated second process using the
same port.

KSC therefore needs a deliberate host architecture.

Preferred model:

```text
                one COM port
                    |
                 KSC Host
               /          \
      terminal/TUI      file server
           |                |
       keyboard        exported folder
```

KSC Host owns the serial port and multiplexes logical channels:

- terminal output;
- keyboard input;
- remote filesystem requests/responses;
- optional diagnostics.

A plain terminal such as `arduino-cli monitor` remains usable when the
`/host` mount is disabled.

This preserves a simple fallback:

```text
plain serial terminal -> KSC local virtual namespace
KSC Host              -> KSC local namespace + /host
```

---

## 10. Host filesystem protocol

The exact framing is not yet frozen.

Requirements:

- deterministic parsing on ATmega328P;
- fixed-size buffers;
- no dynamic allocation;
- bounded record size;
- streaming reads/writes;
- explicit error responses;
- no assumption that files fit in SRAM;
- terminal traffic and filesystem traffic must be distinguishable.

Candidate logical operations:

```text
HOST MOUNT
HOST LS <path>
HOST STAT <path>
HOST OPEN <path>
HOST READ <handle> <offset> <length>
HOST WRITE <handle> <offset> <data>
HOST CLOSE <handle>
```

The first implementation should prefer a small binary or escaped framed
protocol internally rather than long verbose commands if measurements show a
meaningful AVR cost.

Human users still see ordinary paths such as `/host/README.TXT`.

---

## 11. Future W5100 direction

W5100 is especially interesting because the same remote-mount concept can
survive while the transport changes.

Today:

```text
/host -> USB Serial -> PC KSC Host
```

Possible future:

```text
/host -> Ethernet/TCP -> KSC Host
```

or:

```text
/net/server/path -> W5100 -> TCP service
```

The VFS and Commander should ideally not care whether a remote file came through
USB Serial or Ethernet.

That makes W5100 a transport/storage expansion rather than a new user interface.

---

## 12. Future relay-board direction

A relay shield is a natural KSC target because each channel maps cleanly to a
device node.

Example:

```text
/dev/relay/1
/dev/relay/2
/dev/relay/3
/dev/relay/4
/dev/relay/5
/dev/relay/6
/dev/relay/7
/dev/relay/8
```

Typical operations:

```text
CAT /dev/relay/1
WRITE /dev/relay/1 1
WRITE /dev/relay/1 0
```

The Commander can render each RW node as ON/OFF and allow ENTER/LEFT/RIGHT to
change it.

This requires no change to the user-facing filesystem model.

---

## 13. Future LCD Keypad Shield direction

LCD Keypad Shield can provide a local KSC console.

Possible mapping:

```text
LCD       -> local text viewport
UP/DOWN   -> selection
LEFT      -> parent/back
RIGHT     -> open/change
SELECT    -> enter/apply
```

The same Commander navigation model can therefore run with:

- ANSI terminal + PC keyboard;
- ANSI terminal + HY-M302 IR remote;
- local LCD + keypad.

The rendering backend changes; the logical navigation and VFS do not.

The small 16x2 LCD would require a more compact viewport than the normal ANSI
terminal, but that becomes a renderer problem rather than a filesystem problem.

---

## 14. Shell and Commander share one VFS

The shell and KSC Commander must never become separate implementations.

Shell examples:

```text
PWD
LS /
LS /dev
CAT /dev/light
WRITE /dev/led/red 1
CD /dev/dht
CAT temp
KSC
```

Commander invokes the same VFS operations through navigation.

Architecture:

```text
             VFS
            /   \
        Shell   KSC Commander
```

No device-specific logic belongs in the Commander renderer.

---

## 15. First physical milestone

### KSC-01 - Virtual Device Filesystem + TTY shell

Goal:

Prove that real HY-M302 hardware can be represented as virtual paths without SD
and without a graphical display.

Minimum nodes:

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
/dev/ir/last
/proc/mem
/proc/uptime
/sys/version
```

Minimum shell commands:

```text
PWD
CD
LS
CAT
WRITE
KSC
HELP
```

Physical PASS gate:

- namespace lists correctly;
- live sensors return real values;
- writable device nodes change physical outputs;
- IR keys are decoded into logical KSC keys;
- PC arrow keys and IR arrows produce the same navigation events;
- no SD is required;
- memory usage is measured and recorded.

---

## 16. Second physical milestone

### KSC-02 - KonSol Commander ANSI navigator

Status: **FULL PHYSICAL PASS**

KSC-02 certified resource envelope:

```text
Flash:              18026 / 32256 B = 55%
Global SRAM:         1172 / 2048 B  = 57%
Boot free RAM:       818 B
Commander free RAM:  782 B
IR drops:             0 / 0 observed
```


Goal:

Navigate the same VFS through a one-panel ANSI interface.

PASS gate:

- redraws a directory view in the terminal;
- UP/DOWN changes selection;
- OK/ENTER opens a directory or node;
- LEFT/RETURN navigates to parent;
- RO values can be viewed;
- RW values can be changed;
- both PC keyboard and IR remote control the same UI state;
- exit returns cleanly to the shell.

---

## 16A. KSC-02C - Core Extraction & Reference Target

Reference Target status: **FULL PHYSICAL PASS**

Measured Reference Target envelope:

```text
Flash:                    13658 / 32256 B = 42%
Global SRAM:               669 / 2048 B  = 32%
Boot free RAM:             1320 B
Commander free RAM:        1279 B typical
Minimum observed:          1262 B
Input drops:                  0
External shield required:    no
```

The KSC_Core source contains no HY_M302 dependency, DHT implementation, or
target pin mapping. Stronger multi-target hardware-independence certification
remains pending until the HY-M302 line is reimplemented as a second KscTarget
adapter using the same unchanged KSC_Core.


Goal:

Extract the certified KSC-02 shell, ANSI Commander, CHAR/KEY input semantics,
and VFS navigation into a reusable KSC_Core library that has no dependency on
HY_M302.h or shield pin mappings.

Reference architecture:

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

The first KSC-02C implementation adds the core library and a bare-UNO Reference
Target. The certified KSC-02 HY-M302 sketch is intentionally left unchanged
while extraction is tested.

Reference target namespace:

```text
/
+-- demo/
|   +-- counter     RO dynamic
|   +-- value       RW 0..255
|   +-- flag        RW 0..1
+-- proc/
|   +-- mem
|   +-- uptime
+-- sys/
    +-- version
    +-- target
    +-- storage
```

Important claim discipline:

Passing the bare-UNO Reference Target demonstrates that KSC_Core can execute
without HY-M302 hardware. A stronger hardware-independence claim requires the
same KSC_Core source to pass with a second physical target adapter.

---

## 16B. KSC-02D - HY-M302 Target Adapter on KSC_Core

Goal:

Use the same extracted KSC_Core that passed the bare-UNO Reference Target with a
second, physically different target adapter for the HY-M302 shield.

Status: **FULL PHYSICAL PASS**

Architecture:

```text
                  SAME KSC_Core
                  /           \
                 /             \
Reference Target                 HY-M302 Target
bare UNO                         physical shield
synthetic VFS                    sensors/actuators
FULL PASS                        FULL PASS
```

The HY-M302-specific code lives only in the target adapter and the existing
HY_M302 libraries. KSC_Core remains free of HY_M302 includes, DHT code, IR
decoder code, and physical pin mappings.

Physical certification repeated the already certified KSC-02 routes through
the new adapter: live sensors, LED/RGB/buzzer RW nodes, PC keyboard, HY-M302 IR
remote, shared shell/Commander state, memory measurements, and drop counters.

Certified HY-M302 envelope:

```text
Flash:                    20806 / 32256 B = 64%
Global SRAM:              1255 / 2048 B  = 61%
Boot free RAM:             734 B
Commander free RAM:        693 B typical
Minimum observed:          676 B
Input drops:                 0
IR drops:                   0 / 0
```

The same KSC_Core source blobs were used for both targets. This establishes the
bounded empirical two-target hardware-independence result. It does not claim
universal portability to arbitrary MCUs or interfaces.

---

## 17. Third physical milestone

### KSC-03 - Host Filesystem Mount

Goal:

Mount a PC directory as `/host` without adding SD hardware.

PASS gate:

- KSC Host owns the COM port;
- terminal remains interactive;
- `LS /host` returns real PC directory entries;
- `CAT /host/<file>` streams a file without loading it into UNO SRAM;
- transfer errors are explicit;
- disconnect/reconnect does not corrupt local VFS state;
- plain terminal mode still works when the host mount is unavailable.

Only after this milestone should remote program/script execution be considered.

---

## 18. Resource policy

Target MCU:

```text
ATmega328P
Flash: 32 KB
SRAM:   2 KB
EEPROM: 1 KB
Clock:  16 MHz
```

Rules:

- no Arduino String in resident core;
- no malloc/new in resident core;
- fixed buffers;
- cooperative work units;
- no full terminal framebuffer unless measurements justify it;
- no full remote file buffered in SRAM;
- PROGMEM for static tables where useful;
- every milestone records Flash, global SRAM, and runtime free RAM;
- external tooling should carry complexity when it can do so without weakening
  the physical-device result.

---

## 19. Research question

KSC begins with a different question from KonSol 0.8:

> Can a very small 8-bit resident system expose physical devices, kernel state,
> persistent data, and remote host files through one navigable filesystem-like
> namespace, while using a terminal and interchangeable key sources as its main
> human interface?

HY-M302 is the first physical testbed.

W5100, relay hardware, and LCD Keypad Shield are candidate later targets because
they test different parts of the same model:

```text
HY-M302
 -> diverse sensors/actuators + IR keyboard

Relay shield
 -> many uniform writable device nodes

LCD Keypad Shield
 -> local text renderer + local keyboard

W5100
 -> network transport + remote filesystem
```

If the same KSC core survives those changes, the result is stronger than a
single shield-specific utility.

---

## 20. Relationship to frozen KonSol 0.8

KonSol 0.8 remains frozen and published.

KSC may study or reuse:

- static-memory discipline;
- cooperative scheduling lessons;
- terminal parsing experience;
- physical certification methodology;
- streaming techniques;
- host/device protocol lessons.

KSC is not required to reuse:

- KAP1/KAP2 bytecode;
- HOST1 syntax;
- MAR2406 TFT/Touch code;
- SD library architecture;
- application launcher design.

The two lines should remain independently reproducible.

# KSC-03 - Host Filesystem Mount

Status: OPEN - IMPLEMENTATION STARTED

## Goal

Mount a host-computer directory as `/host` while preserving the existing KSC
shell, ANSI Commander, local virtual namespace, and bounded-memory discipline on
Arduino UNO / ATmega328P.

The UNO must never require a complete host file to fit in SRAM.

## Starting point

KSC-02D established:

```text
Reference Target  <->  SAME KSC_Core  <->  HY-M302 Target
FULL PASS                                  FULL PASS
```

Certified HY-M302 runtime envelope:

```text
Boot free RAM:             734 B
Commander free RAM:        693 B typical
Minimum observed:          676 B
Input drops observed:        0
IR drops observed:          0 / 0
```

KSC-03 must therefore use compact fixed-size transport buffers and must record
the memory cost of every new stage.

## Architecture

```text
                   one USB serial link
                           |
                        KSC Host
                  +--------+--------+
                  |                 |
              terminal           hostfs
                  |                 |
                  +--------+--------+
                           |
                         KSC
                           |
                  local VFS + /host
```

KSC Host owns COM while host mounting is active.

A plain serial terminal remains a fallback when host mounting is disabled.

## Logical channels

The serial stream must distinguish:

```text
TTY      human CHAR/KEY input and terminal output
HOSTFS   machine filesystem requests/responses
CONTROL  mount state, diagnostics, protocol control
```

HOSTFS bytes must never enter the KSC keyboard parser.

## Milestones

### KSC-03A - Framed Transport Coexistence

Prove that machine frames and human terminal input can share one COM link
without cross-contamination.

PASS gate:

- KSC Host opens COM;
- terminal remains interactive;
- PC keyboard reaches the existing CHAR/KEY path;
- a framed host ping request reaches a dedicated parser;
- frame bytes never appear as shell text or Commander keys;
- malformed/partial frames fail explicitly;
- fixed transport buffers are measured;
- runtime free RAM is recorded.

### KSC-03B - Remote Directory Enumeration

Expose a host directory as `/host`.

Minimum operations:

```text
MOUNT
LS
STAT
```

PASS gate:

- `LS /host` lists real host entries;
- Commander can enter `/host`;
- local `/dev`, `/proc`, and `/sys` remain functional;
- path traversal is confined to the configured exported root;
- host disconnect returns an explicit error rather than corrupting local VFS.

### KSC-03C - Streamed Read

Add:

```text
OPEN
READ
CLOSE
```

PASS gate:

- `CAT /host/<file>` streams a file larger than available UNO SRAM;
- only bounded chunks are buffered;
- no complete host file is retained in AVR RAM;
- transfer errors are explicit;
- terminal remains interactive between chunks where practical;
- memory and throughput are recorded.

### KSC-03D - Recovery and Final Mount Certification

PASS gate:

- host disconnect/reconnect is handled;
- local VFS remains valid while host is unavailable;
- plain-terminal fallback still works when host mounting is disabled;
- no normal-operation input/frame drops are observed in the certified route;
- final Flash, global SRAM, boot RAM, Commander RAM, and transport-buffer sizes
  are recorded.

## Protocol requirements

The first implementation must prefer deterministic parsing over convenience.

Requirements:

- fixed-size AVR buffers;
- no dynamic allocation in resident KSC code;
- bounded frame length;
- explicit operation IDs;
- explicit status/error response;
- request identifier or equivalent correlation;
- partial-frame timeout/recovery;
- malformed-frame resynchronization;
- streamed reads;
- no assumption that host filenames or file contents fit in SRAM.

## Security / containment requirement

The host service exports one configured directory.

Remote KSC paths must not escape that root through `..`, absolute paths,
drive-letter paths, symlink tricks where preventable, or equivalent host-path
construction.

This is a local engineering containment requirement, not a claim of hardened
multi-user security.

## Non-claims

KSC-03 does not initially claim:

- POSIX filesystem semantics;
- arbitrary concurrent host clients;
- encrypted transport;
- authentication;
- network transparency;
- crash-proof write transactions;
- cross-platform host implementation.

The first certified host implementation may be Windows-specific, matching the
current physically tested environment.

## Final KSC-03 claim target

If all stages pass, the intended bounded result is:

> KSC can extend its existing local virtual namespace across a serial transport
> to a streamed host-backed `/host` tree without requiring host files to fit in
> ATmega328P SRAM and without replacing the existing shell/Commander interface.


## KSC-03A implementation

Firmware sketch:

```text
projects/KSC-KonSol-Commander/sketches/07_KSC_03A_HostTransport/
```

Reusable transport:

```text
projects/KSC-KonSol-Commander/libraries/KSC_HostTransport/
```

Reusable HY-M302 adapter package:

```text
projects/KSC-KonSol-Commander/libraries/KSC_Target_HY_M302/
```

Host:

```text
projects/KSC-KonSol-Commander/tools/ksc_host.py
```

The packaged HY-M302 adapter source is copied from the physically certified
KSC-02D target adapter. KSC_Core itself is not modified for KSC-03A.

### Build

From the repository root:

```powershell
New-Item -ItemType Directory -Force .\build\KSC\07_KSC_03A_HostTransport | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\07_KSC_03A_HostTransport `
  .\projects\KSC-KonSol-Commander\sketches\07_KSC_03A_HostTransport
```

Record both flash and global SRAM.

### Upload

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\07_KSC_03A_HostTransport
```

### Start KSC Host

Do not open Arduino Serial Monitor or another COM4 client at the same time.

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py -p COM4 --ping-on-start
```

Expected boot includes:

```text
KSC-03A Host Transport
TTY + framed HOSTFS on one serial link
KSC Core 0.1
Target: Arduino UNO + HY-M302
IR INIT: OK
TRANSPORT RX payload: 48 B
TRANSPORT TTY queue: 32 B
FREE RAM AFTER TRANSPORT INIT: <measured>
```

Expected host result:

```text
[HOSTFS] PING_RESP seq=<n> PASS
[HOSTFS] PING seq=<n> round-trip PASS
```

### KSC-03A physical route

1. At the KSC shell, type:
   ```text
   MEM
   PWD
   LS /
   CAT /dev/light
   ```
2. Press Ctrl-P several times. Each request must return PING PASS and must not
   insert visible garbage into the shell command line.
3. Open Commander with `KSC`.
4. Navigate with PC arrows.
5. Navigate with HY-M302 IR arrows/OK/RETURN.
6. Press Ctrl-P while Commander is active. The response must be parsed by KSC
   Host rather than becoming a Commander key.
7. Return to shell and run `MEM` again.
8. Press Ctrl-B. Expected:
   ```text
   ERROR_RESP ... BAD_CRC
   ```
9. Press Ctrl-T. Expected after firmware parser timeout:
   ```text
   ERROR_RESP ... TIMEOUT
   ```
10. Press Ctrl-U. Expected:
    ```text
    ERROR_RESP ... UNSUPPORTED
    ```
11. Type ordinary shell text immediately after each malformed-frame test to
    verify parser resynchronization.

### KSC-03A certification data to return

Return the complete compile output and the terminal evidence containing:

```text
Flash:
Global SRAM:
Boot/free RAM after transport init:
MEM at shell:
MEM after Commander route:
PING round trips:
BAD_CRC response:
TIMEOUT response:
UNSUPPORTED response:
Input drops:
IR drops:
Any visible frame leakage into TTY:
```


### KSC-03A build result

Physical build on Arduino UNO / ATmega328P:

```text
Sketch uses 22986 bytes (71%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1396 bytes (68%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 652 bytes.
```

Comparison with the certified KSC-02D HY-M302 build:

```text
                         KSC-02D      KSC-03A       Delta
Flash                    20806 B      22986 B      +2180 B
Global SRAM               1255 B       1396 B       +141 B
Compiler SRAM remainder    793 B        652 B       -141 B
```

Interpretation:

- build PASS;
- transport + reusable target packaging fit within UNO flash/SRAM limits;
- the SRAM margin is materially smaller than KSC-02D and must be verified at
  runtime before KSC-03A can be certified;
- no runtime free-RAM claim is made from compiler globals alone.

Next gate: upload and measure boot/free RAM through KSC Host while testing
TTY/HOSTFS coexistence.


### KSC-03A first physical transport result

Status: **TRANSPORT CORE PHYSICAL PASS - COMMANDER/IR ROUTE STILL PENDING**

Observed host startup and protocol result:

```text
KSC HOST 0.1 - KSC-03A
PORT=COM4 BAUD=115200

PING_RESP seq=1 PASS
PING seq=1 round-trip PASS

KSC-03A Host Transport
TTY + framed HOSTFS on one serial link

KSC Core 0.1
Target: Arduino UNO + HY-M302
FREE RAM: 593 B
IR INIT: OK

TRANSPORT RX payload: 48 B
TRANSPORT TTY queue: 32 B
FREE RAM AFTER TRANSPORT INIT: 593 B
```

Shell coexistence was physically verified:

```text
MEM
593

PWD
/

LS /
D dev
D proc
D sys

CAT /dev/light
273
```

Five framed PING round trips succeeded while ordinary shell traffic remained
functional:

```text
PING seq=1 PASS
PING seq=2 PASS
PING seq=3 PASS
PING seq=4 PASS
PING seq=5 PASS
```

Negative protocol routes also passed:

```text
bad CRC     -> ERROR_RESP ... BAD_CRC
partial     -> ERROR_RESP ... TIMEOUT
unsupported -> ERROR_RESP ... UNSUPPORTED
```

No HOSTFS frame bytes were visibly injected into the shell command stream in
the recorded route.

Runtime RAM result:

```text
KSC-02D boot free RAM:        734 B
KSC-03A boot/free RAM:        593 B
Runtime delta:               -141 B

KSC-02D global SRAM:         1255 B
KSC-03A global SRAM:         1396 B
Global SRAM delta:           +141 B
```

The observed runtime free-RAM loss exactly matches the compiler-reported global
SRAM increase for this route.

This is strong evidence that the first bounded transport implementation has not
introduced an additional hidden persistent RAM loss beyond the added globals.

Remaining KSC-03A certification items:

- Commander navigation through the multiplexed host;
- framed PING while Commander is active;
- HY-M302 IR navigation while host owns COM;
- shell/TTY resynchronization after malformed frame tests;
- post-Commander MEM observation;
- input-drop and IR-drop observation on the final route.


## KSC-03B implementation

Status: **IMPLEMENTED - FIRST PHYSICAL BUILD/TEST PENDING**

Firmware:

```text
projects/KSC-KonSol-Commander/sketches/08_KSC_03B_HostMount/
```

New target wrapper:

```text
projects/KSC-KonSol-Commander/libraries/KSC_Target_HostMount/
```

Host tool:

```text
projects/KSC-KonSol-Commander/tools/ksc_host.py
```

Default exported PC directory:

```text
projects/KSC-KonSol-Commander/host-share/
```

Expected remote namespace:

```text
/
+-- dev/
+-- proc/
+-- sys/
+-- host/
    +-- DOCS/
    |   +-- HELLO.TXT
    +-- DATA.TXT
    +-- README.TXT
```

KSC_Core remains unchanged. The new KscHostMountTarget wraps the existing
physical HY-M302 target and delegates all non-`/host` paths to it.

### Build

```powershell
New-Item -ItemType Directory -Force .\build\KSC\08_KSC_03B_HostMount | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\08_KSC_03B_HostMount `
  .\projects\KSC-KonSol-Commander\sketches\08_KSC_03B_HostMount
```

Record Flash and global SRAM before upload.

### Upload

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\08_KSC_03B_HostMount
```

### Start host service

Default sample export:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --ping-on-start
```

Explicit arbitrary export directory:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --export C:\Some\Folder
```

Do not run another COM4 client at the same time.

### KSC-03B shell route

Expected:

```text
LS /
D dev
D proc
D sys
D host

LS /host
D DOCS
F DATA.TXT  RO
F README.TXT  RO

LS /host/DOCS
F HELLO.TXT  RO
```

Also verify:

```text
CAT /dev/light
MEM
```

to prove the local target remains functional.

Opening a remote file in KSC-03B returns the explicit placeholder:

```text
REMOTE FILE: KSC-03C
```

Actual byte streaming is reserved for KSC-03C.

### Commander route

1. Enter `KSC`.
2. Confirm the root now shows `host/` beside `dev/`, `proc/`, and `sys/`.
3. Open `host/`.
4. Confirm `DOCS/`, `DATA.TXT`, and `README.TXT`.
5. Open `DOCS/` and confirm `HELLO.TXT`.
6. Return to root and open a local `/dev` node to confirm local navigation still
   works.
7. Use the HY-M302 IR remote for at least UP, DOWN, OK, RETURN, and HOME.
8. Run a Ctrl-P transport PING while the remote directory view is active.
9. Return to shell and run `MEM`.

### KSC-03B containment checks

Run the host against the default export and verify that no parent directory is
visible.

The KSC path layer itself truncates paths to 23 characters in this core version;
the host additionally rejects traversal and paths outside the configured export
root.

### KSC-03B data to return

```text
Flash:
Global SRAM:
Boot/free RAM after host-mount init:
MEM after /host shell route:
MEM after Commander /host route:
LS /:
LS /host:
LS /host/DOCS:
Commander /host visible:
PC navigation:
IR navigation:
PING while /host active:
Input drops:
IR drops:
Host errors shown in status:
Any TTY/frame corruption:
```


### KSC-03B build result

Physical build on Arduino UNO / ATmega328P:

```text
Sketch uses 25630 bytes (79%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1479 bytes (72%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 569 bytes.
```

Comparison:

```text
                         KSC-02D      KSC-03A      KSC-03B
Flash                    20806 B      22986 B      25630 B
Global SRAM               1255 B       1396 B       1479 B
Compiler SRAM remainder    793 B        652 B        569 B
```

Increment from KSC-03A to KSC-03B:

```text
Flash:       +2644 B
Global SRAM:   +83 B
Remainder:     -83 B
```

Interpretation:

- build PASS;
- KSC-03B still fits Arduino UNO flash/SRAM limits;
- SRAM headroom is now tight enough that runtime measurements are mandatory
  before certification;
- no safety claim is made from compiler global usage alone.

Next gate: upload, start KSC Host 0.2, measure runtime free RAM, and test real
`LS /host` plus Commander navigation.


### KSC-03B first physical host mount result

Status: **REMOTE /host SHELL ROUTE PHYSICAL PASS - COMMANDER/IR ROUTE STILL PENDING**

Observed boot:

```text
KSC-03B Host Filesystem Mount
Local VFS + remote /host over one COM link

KSC Core 0.1
Target: Arduino UNO + HY-M302
FREE RAM: 510 B
IR INIT: OK
Host mount: /host lazy remote

FREE RAM AFTER HOST MOUNT INIT: 510 B
```

Runtime memory:

```text
KSC-03A free RAM: 593 B
KSC-03B free RAM: 510 B
Delta:             -83 B

KSC-03A global SRAM: 1396 B
KSC-03B global SRAM: 1479 B
Delta:               +83 B
```

The runtime free-RAM reduction exactly matches the compiler-reported global SRAM
increase from KSC-03A to KSC-03B.

Root namespace:

```text
LS /
D dev
D proc
D sys
D host
```

First lazy mount:

```text
MOUNT /host -> configured host-share directory
```

Remote host directory enumeration:

```text
LS /host
D DOCS
F DATA.TXT  RO
F README.TXT  RO
```

Nested enumeration:

```text
LS /host/DOCS
F HELLO.TXT  RO
```

Local HY-M302 path remained functional after remote enumeration:

```text
CAT /dev/light
253
49
80
568
```

A framed PING response was also observed after the /host route:

```text
PING_RESP seq=1 PASS
```

This proves the first shell-level remote directory mount:

```text
PC directory
   |
KSC Host 0.2
   |
HOSTFS MOUNT/STAT/LS
   |
COM4
   |
KSC HostMountTarget
   |
/host visible through ordinary KSC shell paths
```

Remaining KSC-03B certification items:

- open /host in Commander;
- enumerate /host and /host/DOCS through Commander;
- verify local /dev navigation still works after remote navigation;
- verify PC key navigation;
- verify HY-M302 IR UP/DOWN/OK/RETURN/HOME;
- complete a framed PING round trip while the remote Commander view is active;
- record post-Commander MEM;
- observe input-drop, IR-drop, and HOST error counters;
- verify no TTY/frame corruption on the final route.


### KSC-03B Commander physical route result

Status: **COMMANDER REMOTE-NAMESPACE ROUTE PHYSICAL PASS - FINAL INPUT-SOURCE/PING-IN-VIEW CHECK PENDING**

Observed Commander root:

```text
Path: /
> [dev/]
  [proc/]
  [sys/]
  [host/]

RAM 469 B   IN drop 0   IR drop 0/0   HOST M/0
```

Remote directory opened through Commander:

```text
Path: /host
> [DOCS/]
  DATA.TXT  [RO]
  README.TXT  [RO]
```

Remote file placeholder opened as designed for KSC-03B:

```text
Path: /host/DATA.TXT
Type: RO
Value: REMOTE FILE: KSC-03C
```

Nested remote directory and file route also passed:

```text
Path: /host/DOCS
> HELLO.TXT  [RO]

Path: /host/DOCS/HELLO.TXT
Type: RO
Value: REMOTE FILE: KSC-03C
```

Return navigation back through `/host` to the root was observed.

Runtime observations during Commander:

```text
Commander free RAM: 469 B
IN drop:             0
IR drop:             0/0
HOST:                M/0
```

After Commander exit:

```text
KSC Commander closed.
FREE RAM: 508 B

MEM
510
```

The 2-byte difference between the immediate post-Commander print and the
subsequent shell MEM observation is recorded as an instantaneous free-RAM
observation, not a leak claim. The shell returned to the established 510 B
baseline.

The log proves:

- `/host` is visible and navigable in Commander;
- remote file nodes are exposed as read-only entries;
- nested `/host/DOCS/HELLO.TXT` is navigable;
- the KSC-03B placeholder is shown instead of pretending that file streaming
  already exists;
- return to local root succeeds;
- no input drops, IR drops, or host errors were reported in the captured route;
- free RAM returns to the shell baseline after Commander exit.

Not yet attributable from the captured transcript alone:

- which navigation actions were performed from the PC keyboard versus HY-M302 IR;
- a framed PING round trip while the Commander remote-directory view itself was
  active.

Those two checks remain the final gate before declaring **KSC-03B FULL PHYSICAL PASS**.


### KSC-03B framed PING inside remote Commander view

Status: **PHYSICAL PASS**

While Commander was actively displaying the remote directory:

```text
Path: /host
> [DOCS/]
  DATA.TXT  [RO]
  README.TXT  [RO]

RAM 469 B   IN drop 0   IR drop 0/0   HOST M/1
```

the host transport hotkey produced:

```text
PING_RESP seq=2 PASS
PING seq=2 round-trip PASS
```

This verifies that a framed HOSTFS transport exchange can complete while the
Commander UI is active on the remote `/host` namespace without visibly
corrupting the directory view.

The captured status showed `HOST M/1`, i.e. one host-side error had been
accumulated somewhere in the session. The PING itself passed. The source of that
single accumulated host error is not inferred from this transcript and should
not be silently treated as zero.

Remaining gate for **KSC-03B FULL PHYSICAL PASS**:

- explicit attribution that PC keyboard navigation was tested successfully;
- explicit attribution that HY-M302 IR navigation was tested successfully.

If both are confirmed, KSC-03B may be closed as FULL PHYSICAL PASS, with the
single observed accumulated `HOST M/1` retained in the record unless a clean
rerun establishes `M/0`.

# KSC-04A - Commander File Viewer

Status: **IMPLEMENTED - FIRST PHYSICAL BUILD/TEST PENDING**

## Goal

Add a real remote-file viewer to KonSol Commander on top of the already
certified KSC-03 remote `/host` transport, mount, streaming, and recovery
foundation.

KSC-04A is deliberately a byte-window viewer, not yet a line-aware text viewer.

## Architecture

```text
Commander
  -> /host
  -> remote RO file
  -> ENTER
  -> VIEW_FILE
       |
       +-> streamWindow(path, offset, 192 B)
               |
               +-> OPEN
               +-> READ <= 32 B
               +-> immediate Print output
               +-> repeated READ
               +-> CLOSE
```

The logical viewer page is 192 bytes, but the AVR transport buffer remains
bounded at 32 bytes.

No 192-byte page buffer is allocated in SRAM.

## Core API extension

KSC-04A adds the optional target hook:

```cpp
streamWindow(
  path,
  offset,
  maxBytes,
  Print &out,
  bytesRead,
  totalSize,
  handled
)
```

Targets that do not implement this hook remain on the existing node-view path.

## Commander controls

For a remote host file:

```text
ENTER/RIGHT   open file viewer
UP/LEFT       previous 192-byte window
DOWN/RIGHT    next 192-byte window
ENTER         next 192-byte window
HOME          first window
END           final window
BACK          return to directory
F10/POWER/Q   shell
```

The viewer header shows:

```text
FILE <offset>/<total-size>
```

## Important scope boundary

KSC-04A pages by byte offset. A page boundary may split a text line.

That is intentional and is not treated as a defect in 04A.

Line-aware navigation and rendering belong to KSC-04C.

## Build

```powershell
New-Item -ItemType Directory -Force .\build\KSC\11_KSC_04A_CommanderFileViewer | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\11_KSC_04A_CommanderFileViewer `
  .\projects\KSC-KonSol-Commander\sketches\11_KSC_04A_CommanderFileViewer
```

Record Flash and global SRAM before upload.

## Physical route after upload

Start KSC Host normally:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4
```

Then:

```text
KSC
-> host
-> BIG.TXT
-> ENTER
```

Verify:

1. opening BIG.TXT enters a real file view, not `CAT STREAM AVAILABLE`;
2. first page shows the beginning of the actual host file;
3. DOWN or RIGHT advances the offset by 192 bytes;
4. UP or LEFT returns by 192 bytes;
5. HOME returns to offset 0;
6. END reaches the final page;
7. BACK returns to `/host`;
8. PC keyboard works;
9. HY-M302 IR navigation works;
10. no reset occurs;
11. `IN drop 0`;
12. `IR drop 0/0`;
13. `HOST M/0`.

## KSC-04A certification claim if PASS

**KSC-04A demonstrates bounded-window interactive viewing of a remote PC-hosted
file inside KonSol Commander on Arduino UNO, using the certified remote
`/host` transport and 32-byte streamed reads without storing the complete
viewer page or complete file in ATmega328P SRAM.**

Non-claims:

- page boundaries are not line-aware;
- editing is not provided;
- arbitrary binary-safe rendering is not certified;
- multi-pane Commander is not part of this stage.


## Build result

Physical build on Arduino UNO / ATmega328P:

```text
Sketch uses 28816 bytes (89%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1499 bytes (73%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 549 bytes.
```

Comparison with KSC-03D:

```text
                         KSC-03D      KSC-04A      Delta
Flash                    26712 B      28816 B      +2104 B
Global SRAM               1485 B       1499 B        +14 B
Compiler SRAM remainder    563 B        549 B        -14 B
```

Interpretation:

- build PASS;
- the first Commander file-view layer costs 2104 bytes of flash;
- persistent SRAM cost is 14 bytes;
- SRAM remains tight but within the same operational envelope as KSC-03;
- runtime free-RAM and navigation counters remain mandatory for physical
  certification.


## First physical viewer result

Status: **PARTIAL PHYSICAL PASS - IR DROP REGRESSION FOUND**

Observed Commander directory status before opening the file:

```text
Path: /host
> BIG.TXT  [RO]
RAM 437 B   IN drop 0   IR drop 181/0   HOST M/0 R0
```

Opening BIG.TXT entered the real streaming viewer:

```text
Path: /host/BIG.TXT
FILE 0/12445
KSC-03C BIG FILE - STREAMING PROOF
LINE 000: ...
LINE 001: ...
RAM 437 B   IN drop 0   IR drop 181/0   HOST M/0 R0
```

Confirmed:

- real remote file viewer opens from Commander;
- file size is reported;
- first window contains real host-file bytes;
- runtime RAM observed at 437 B;
- KSC input drops remain 0;
- HOST errors remain 0.

Blocking issue:

```text
IR drop 181/0
```

The first counter is HY-M302 async IR edge-buffer overflow. The host exchange
path waits synchronously for serial responses, and before the fix the local
HY-M302 service routine was not called inside those wait loops. That can allow
captured IR edges to accumulate faster than they are decoded.

KSC-04A is therefore not certified yet.

A cooperative transport wait hook has been added so the local target service
runs while an OPEN/READ/CLOSE exchange is waiting for its host response. A clean
rebuild/reflash and fresh IR navigation test is required. A reset/reflash is
important because the dropped-edge counter is cumulative within a boot.


## Clean IR coexistence retest

Status: **PASS**

After the cooperative host-wait service fix and a clean restart, Commander at
`/host` reported:

```text
RAM 437 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

This clears the previously observed IR edge-buffer overflow regression at the
directory-view level.

The remaining KSC-04A certification gate is to exercise the actual remote file
viewer navigation after the fix, preferably with the HY-M302 IR remote:

```text
BIG.TXT -> ENTER
DOWN
DOWN
UP
HOME
END
BACK
```

and confirm that the file offsets move as expected while the final counters
remain `IN drop 0`, `IR drop 0/0`, and `HOST M/0`.

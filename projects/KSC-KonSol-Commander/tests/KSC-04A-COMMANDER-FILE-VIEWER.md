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

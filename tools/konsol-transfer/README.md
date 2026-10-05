# KonSol SD Writer — CLI + GUI

This host-side tool writes a local `.KAP` file to the Arduino UNO microSD
**through the existing KonSol Serial Shell**.

It intentionally reproduces the same operation previously performed by hand:

```text
WRITE /MULTI.KAP 4B415032
APPEND /MULTI.KAP 1000
APPEND /MULTI.KAP ...
...
```

The tool does not access the SD card directly and does not change the KonSol
filesystem model.

## Why this tool exists

At the current KonSol stage, a PC-side program cannot mount the shield microSD
through the UNO as a normal USB disk. The reliable path already verified on the
bench is the resident shell:

```text
PC -> USB Serial -> KonSol WRITE/APPEND -> microSD
```

KonSol SD Writer automates that exact path, waits for `OK n B` after every
line/chunk, and can read the completed file back with `TYPE`.

## Requirements

- Python 3
- pyserial

Install:

```powershell
python -m pip install pyserial
```

Close Arduino IDE Serial Monitor and `arduino-cli monitor` before using the
writer because only one process can own the COM port.

## GUI

From repository root:

```powershell
python .\tools\konsol-transfer\konsol_transfer.py gui
```

or simply:

```powershell
python .\tools\konsol-transfer\konsol_transfer.py
```

Select:

1. COM port;
2. local `.KAP` file;
3. destination on microSD;
4. **Write KAP to microSD**.

The GUI shows every generated `WRITE` / `APPEND` command and every KonSol
reply.

## CLI

List ports:

```powershell
python .\tools\konsol-transfer\konsol_transfer.py ports
```

Write TEST-08 application:

```powershell
python .\tools\konsol-transfer\konsol_transfer.py write `
  --port COM4 `
  --file .\labs\05-UNO-KON-OS\apps\KAP2\MULTI.KAP `
  --dest /MULTI.KAP
```

Default verification is:

```text
TYPE /MULTI.KAP
local normalized KAP byte stream == remote normalized KAP byte stream
```

A successful transfer ends with:

```text
VERIFY: PASS
TRANSFER PASS: /MULTI.KAP
```

## Command-length handling

KonSol uses an 88-byte command buffer, so a complete shell line may contain at
most 87 characters before the terminating NUL.

The writer automatically splits long hexadecimal KAP records at byte boundaries.
Inserted line breaks are safe because the KAP decoder already ignores
whitespace between encoded bytes.

## 8.3 filenames

For maximum compatibility with the AVR Arduino SD stack, the writer enforces
8.3-style path components. TEST-08 therefore uses:

```text
/MULTI.KAP
```

rather than a long SD filename.

## Serial behavior

Opening a classic UNO serial connection can reset the board. The writer waits
for boot, discards the boot banner, sends an empty line, synchronizes on the
KonSol prompt:

```text
A:/>
```

and only then begins the transfer.

## Scope

This version is intentionally a KAP transfer utility, not a general filesystem
protocol. Empty-line-preserving arbitrary text transfer is outside the current
scope. For KAP1/KAP2 files this is lossless at the decoded byte-stream level.

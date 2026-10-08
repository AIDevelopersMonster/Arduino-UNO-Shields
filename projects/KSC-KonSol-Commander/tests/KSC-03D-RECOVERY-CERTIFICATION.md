# KSC-03D Recovery and Final Remote /host Certification

Status: **IMPLEMENTED - PHYSICAL BUILD/TEST PENDING**

## Purpose

KSC-03D hardens the KSC-03C host-file stream against recoverable transport and
host-session faults before the remote `/host` line is treated as complete.

The key protocol change is an explicit 32-bit byte offset in every READ request.
A repeated READ for the same offset is therefore idempotent: if a response is
lost, the AVR can request the same chunk again without duplicating or skipping
file bytes.

## READ v0.2

Request:

```text
TYPE = READ_REQ
PAYLOAD:
  [0]    handle
  [1..4] offset, uint32 little-endian
  [5]    requested byte count, 1..32
```

Response:

```text
TYPE = READ_RESP
PAYLOAD:
  [0]    handle
  [1..4] echoed offset
  [5]    EOF flag
  [6..]  data bytes
```

## Recovery policy

The AVR retries a chunk up to two times for:

- transport TIMEOUT;
- BAD_HANDLE.

For TIMEOUT it repeats the same handle + offset.

For BAD_HANDLE it reopens the same path and retries the same offset with the new
handle.

Recovered faults increment a separate recovery counter shown as:

```text
HOST M/<errors> R<recoveries>
```

A recovered fault is not counted as a final HOST error.

## Deterministic host fault injection

KSC Host 0.4 adds:

```text
--drop-read-at N
--invalidate-read-at N
```

The first suppresses the Nth READ response to force an AVR-side timeout and
same-offset retry.

The second invalidates all open file handles at the Nth READ so the AVR receives
BAD_HANDLE, reopens the file, and continues from the same explicit offset.

## Build

```powershell
New-Item -ItemType Directory -Force .\build\KSC\10_KSC_03D_RecoveryCertification | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\10_KSC_03D_RecoveryCertification `
  .\projects\KSC-KonSol-Commander\sketches\10_KSC_03D_RecoveryCertification
```

Record Flash and global SRAM before upload.

## Upload

```powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\10_KSC_03D_RecoveryCertification
```

## Baseline test

Run KSC Host without faults:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4
```

Then:

```text
MEM
LS /host
CAT /host/BIG.TXT
MEM
KSC
```

Expected:

- complete file from first marker through final marker;
- no reset;
- shell MEM returns to baseline;
- Commander status ends with `HOST M/0 R0`.

## Recovery test A - dropped READ response

Restart KSC Host:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --drop-read-at 10
```

Run:

```text
CAT /host/BIG.TXT
MEM
KSC
```

Expected host diagnostic:

```text
FAULT drop READ response at request #10 offset=...
```

Expected result:

- CAT still reaches `END OF KSC-03C BIG FILE`;
- no missing or duplicated numbered line is visible;
- no reset;
- final Commander status has `HOST M/0 R1`.

## Recovery test B - invalidated handle

Restart KSC Host:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --invalidate-read-at 10
```

Run:

```text
CAT /host/BIG.TXT
MEM
KSC
```

Expected host diagnostic:

```text
FAULT invalidate handles at READ #10 offset=...
```

Expected result:

- AVR receives BAD_HANDLE;
- target reopens the same path;
- streaming resumes from the same explicit offset;
- CAT reaches the final marker;
- no reset;
- final Commander status has `HOST M/0 R1`.

## Combined recovery test

Optional final stress:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py `
  -p COM4 `
  --drop-read-at 10 `
  --invalidate-read-at 40
```

Expected:

```text
HOST M/0 R2
```

and complete BIG.TXT output.

## Final KSC-03D certification target

If baseline + both independent fault tests pass, the bounded claim is:

**KSC-03D demonstrates recoverable sequential text-file streaming over the
remote `/host` namespace on Arduino UNO, with explicit-offset idempotent READ,
retry after a lost READ response, and reopen/resume after a lost host file
handle, without requiring the complete file to fit in ATmega328P SRAM.**

Non-claims:

- arbitrary binary-safe transport is not yet certified;
- host-process restart during an active CAT is not yet transparently recovered;
- multiple concurrent AVR-side open files are not certified;
- Commander still has only bounded node preview, not a full file viewer.


## Build result

Physical build on Arduino UNO / ATmega328P:

```text
Sketch uses 26712 bytes (82%) of program storage space.
Maximum: 32256 bytes.

Global variables use 1485 bytes (72%) of dynamic memory.
Maximum: 2048 bytes.
Compiler-reported space left for locals: 563 bytes.
```

Comparison with KSC-03C:

```text
                         KSC-03C      KSC-03D      Delta
Flash                    26330 B      26712 B      +382 B
Global SRAM               1483 B       1485 B        +2 B
Compiler SRAM remainder    565 B        563 B        -2 B
```

Interpretation:

- build PASS;
- recovery hardening adds only 2 bytes of persistent SRAM;
- flash remains below the Arduino UNO sketch limit;
- runtime free-RAM and recovery-path measurements remain mandatory before
  certification.


## Physical recovery test A - dropped READ response

Status: **PHYSICAL PASS**

Fault injection:

```text
--drop-read-at 10
```

Observed host diagnostic:

```text
FAULT drop READ response at request #10 offset=288
```

The stream then continued from the same explicit offset and completed the large
test file through:

```text
LINE 095: ...
END OF KSC-03C BIG FILE
```

Post-stream observations:

```text
MEM
504

Commander:
RAM 463 B   IN drop 0   IR drop 0/0   HOST M/0 R1
```

Interpretation:

- the deliberately lost READ response was recovered;
- the stream reached the final marker without reset;
- the recovery counter increased to R1;
- the final host-error counter remained M/0;
- input and IR drop counters remained zero;
- shell free RAM returned to the 504 B runtime baseline.

The captured terminal display shows the host diagnostic line interleaved into
the human-readable text around LINE 001. This is console-output interleaving
between the host diagnostic and the TTY stream, not evidence of a skipped file
offset: the numbered file sequence continues through LINE 095 and the final
marker is present.

Result: **lost READ-response recovery route PASS**.

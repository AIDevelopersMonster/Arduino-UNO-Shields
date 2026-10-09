# KSC Host Protocol - Framing v0.1 + Recoverable READ v0.2

Status: **PHYSICALLY VALIDATED THROUGH KSC-03D**

The original v0.1 framing, message type allocation, CRC-8/ATM rule, 48-byte
payload bound and TTY/HOSTFS coexistence are physically certified on Arduino
UNO / ATmega328P. KSC-03D retains that frame format while upgrading READ to the
explicit-offset v0.2 payload documented below.

## Objective

Provide an AVR-friendly framed machine channel that can coexist with normal KSC
terminal traffic on one serial connection.

The framing is deliberately small and binary.

## Frame format

Certified v0.1 frame:

```text
+------+-----+------+-----+------+---------+-------+
| SOF1 | SOF2| TYPE | SEQ | LEN  | PAYLOAD | CRC8  |
+------+-----+------+-----+------+---------+-------+
  0x1B  0x5D   1 B    1 B   1 B    0..48 B   1 B
```

Where:

- `SOF1=0x1B`
- `SOF2=0x5D`
- `TYPE` identifies request/response/control operation;
- `SEQ` correlates one request and response;
- `LEN` is payload length, maximum 48 bytes in v0.1;
- `CRC8` covers TYPE, SEQ, LEN, and PAYLOAD.

Maximum complete v0.1 frame size:

```text
2 + 1 + 1 + 1 + 48 + 1 = 54 bytes
```

The SOF pair was subsequently physically validated through KSC-03A..03D while
sharing the same serial connection with ordinary terminal traffic.

## Initial message types

```text
0x01 PING_REQ
0x81 PING_RESP

0x02 MOUNT_REQ
0x82 MOUNT_RESP

0x03 LS_REQ
0x83 LS_RESP

0x04 STAT_REQ
0x84 STAT_RESP

0x05 OPEN_REQ
0x85 OPEN_RESP

0x06 READ_REQ
0x86 READ_RESP

0x07 CLOSE_REQ
0x87 CLOSE_RESP

0x7F ERROR_RESP
```

KSC-03A initially exercised PING/error handling; KSC-03B..03D subsequently
activated MOUNT, STAT, LS, OPEN, READ, and CLOSE on the same message type map.

## Parser policy

Firmware parser states:

```text
IDLE
SEEN_SOF1
HEADER
PAYLOAD
CRC
```

Rules:

- no heap allocation;
- one bounded payload buffer;
- timeout resets an incomplete frame;
- invalid LEN resets parser;
- invalid CRC produces no filesystem operation;
- parser must resynchronize at the next valid SOF;
- frame bytes consumed by HOSTFS must not be passed to the TTY CHAR/KEY parser.

## Terminal coexistence

Normal terminal bytes remain ordinary TTY input unless a valid HOSTFS frame
start is recognized.

The KSC Host process owns the serial port and provides the human terminal
frontend while host mounting is active.

## Buffer target

Initial AVR budget:

```text
RX payload buffer: 48 B
small parser/header state: < 16 B target
TX response constructed/streamed without a second full 48 B copy where possible
```

KSC-03A must measure the actual global SRAM and runtime free-RAM delta.

## CRC

KSC-03A freezes CRC-8/ATM (CRC-8/ITU) for protocol v0.1:

```text
width:       8
polynomial:  0x07
init:        0x00
refin:       false
refout:      false
xorout:      0x00
coverage:    TYPE, SEQ, LEN, PAYLOAD
```

Update rule:

```text
crc ^= byte
repeat 8 times:
  if crc & 0x80:
    crc = (crc << 1) ^ 0x07
  else:
    crc = crc << 1
keep crc to 8 bits
```

Reference vectors:

```text
TYPE SEQ LEN [PAYLOAD] -> CRC

01 01 00             -> 7E
81 01 00             -> 75
01 2A 00             -> 47
7F 01 01 03          -> 97
```

The host and AVR implementations use the same rule.

## Why not textual commands

Human-readable commands are convenient but make unambiguous multiplexing with a
normal shell harder and increase parsing/state cost.

The machine protocol is therefore binary internally while users continue to see
ordinary paths and shell/Commander operations.


## Implemented KSC-03A transport

Firmware library:

```text
projects/KSC-KonSol-Commander/libraries/KSC_HostTransport/
```

Host tool:

```text
projects/KSC-KonSol-Commander/tools/ksc_host.py
```

The firmware transport is implemented as a `Stream` wrapper. KSC_Core reads and
writes through that wrapper, so the existing core source does not need a HOSTFS
parser.

Inbound bytes are classified as:

```text
ordinary byte
  -> bounded TTY queue
  -> existing KSC_Core terminal parser

ESC ]
  -> HOSTFS frame parser
  -> frame handler
  -> never forwarded as CHAR/KEY bytes
```

Current fixed AVR storage added by the transport object includes:

```text
HOSTFS payload buffer: 48 B
TTY queue:              32 B
parser/counters/state:  measured by final build/runtime test
```

KSC-03A physical certification measured the whole-build Flash/global SRAM delta
and runtime free SRAM; later KSC-03 stages reused the same bounded transport.


## KSC-03B directory protocol

KSC-03B activates the previously reserved MOUNT, STAT, and LS message types.

### MOUNT

Request:

```text
TYPE = 0x02 MOUNT_REQ
LEN  = 0
```

Success response:

```text
TYPE = 0x82 MOUNT_RESP
PAYLOAD:
  [0] = 1
```

The mount is lazy: the firmware exposes `/host` in the root namespace and
performs MOUNT on the first remote operation.

### STAT

Request:

```text
TYPE = 0x04 STAT_REQ
PAYLOAD = ASCII KSC path
example: /host/DOCS
```

Success response:

```text
TYPE = 0x84 STAT_RESP
PAYLOAD:
  [0] = host node type

0 = NONE
1 = DIR
2 = FILE
```

Remote files are exposed to KSC_Core as read-only nodes. KSC-03B does not stream
their contents; opening such a node displays a KSC-03C placeholder.

### LS

Request payload:

```text
[0]     entry index
[1..N]  ASCII KSC directory path
```

Special index:

```text
0xFF = count-only query
```

Count response:

```text
TYPE = 0x83 LS_RESP
PAYLOAD:
  [0] = visible entry count
```

Entry response:

```text
TYPE = 0x83 LS_RESP
PAYLOAD:
  [0] = total visible entry count
  [1] = host node type
  [2] = name length
  [3..] ASCII name
```

KSC-03B v0.1 limits a visible remote basename to 15 ASCII bytes. Longer or
non-ASCII host names are omitted from the exported view. This is an explicit
first-version constraint matching the current Commander entry buffer.

### Host containment

The host tool exports exactly one configured directory.

The service rejects:

- paths outside `/host`;
- `.` and `..` components;
- drive-letter/colon components;
- backslash path injection;
- resolved paths that escape the configured export root.

The default exported test directory is:

```text
projects/KSC-KonSol-Commander/host-share/
```

It contains:

```text
/host
+-- DOCS/
|   +-- HELLO.TXT
+-- DATA.TXT
+-- README.TXT
```


## KSC-03C streamed file protocol

KSC-03C activates the previously reserved OPEN, READ, and CLOSE message types.

The design is intentionally sequential and bounded. The AVR never receives the
whole host file at once.

### OPEN

Request:

```text
TYPE = 0x05 OPEN_REQ
PAYLOAD = ASCII KSC path
example: /host/BIG.TXT
```

Success response:

```text
TYPE = 0x85 OPEN_RESP
PAYLOAD:
  [0]    handle, 1..255
  [1..4] file size, unsigned 32-bit little-endian
```

The current AVR target uses the handle and does not need to retain the complete
file size in SRAM.

### READ

Request:

```text
TYPE = 0x06 READ_REQ
PAYLOAD:
  [0] handle
  [1] requested byte count
```

KSC-03C AVR requests at most 32 bytes per transaction.

Success response:

```text
TYPE = 0x86 READ_RESP
PAYLOAD:
  [0]    handle
  [1]    EOF flag, 0 or 1
  [2..]  0..32 file bytes
```

READ is sequential. The host owns the file position for the open handle.

### CLOSE

Request:

```text
TYPE = 0x07 CLOSE_REQ
PAYLOAD:
  [0] handle
```

Success response:

```text
TYPE = 0x87 CLOSE_RESP
PAYLOAD:
  [0] handle
```

### New host error codes

```text
0x14 NOT_FILE
0x15 BAD_HANDLE
```

### Bounded-memory property

For shell `CAT /host/<file>`, the AVR repeats:

```text
OPEN
READ 32 B
READ 32 B
...
CLOSE
```

and forwards each received chunk to the terminal before requesting the next
chunk.

The test file `/host/BIG.TXT` is intentionally about 12 KB, far larger than
the ATmega328P 2 KB SRAM. A complete successful CAT therefore demonstrates that
the implementation is chunk-streaming rather than buffering the whole file in
AVR SRAM.

### Text-stream scope

KSC-03C certifies text-oriented CAT over the existing mixed TTY/HOSTFS serial
link. Arbitrary binary files containing byte sequences that collide with the
TTY/frame discriminator are not yet claimed as a certified binary-safe CAT
channel.


## KSC-03D recoverable READ v0.2

KSC-03D changes READ from implicit host-position state to an explicit-offset
request.

Request payload:

```text
[0]    handle
[1..4] offset, uint32 little-endian
[5]    requested byte count, 1..32
```

Response payload:

```text
[0]    handle
[1..4] echoed offset
[5]    EOF flag
[6..]  data bytes
```

This makes a READ idempotent with respect to response loss: retrying the same
handle + offset requests the same file region instead of advancing an implicit
cursor.

KSC-03D recovery policy:

- TIMEOUT: retry same handle + offset;
- BAD_HANDLE: reopen the same path, then retry the same offset;
- at most two retries for a recoverable chunk failure;
- successful recoveries increment a dedicated recovery counter;
- unrecovered failures increment the host-error counter.

This protocol change is the recovery basis for final remote `/host`
certification.


## Final certification summary

The protocol line was physically exercised through KSC-03D with:

- one Serial/COM link carrying both TTY and HOSTFS traffic;
- maximum framed payload 48 bytes;
- CRC-8/ATM over TYPE, SEQ, LEN and PAYLOAD;
- remote `/host` mount and directory enumeration;
- host file streaming in requests of at most 32 data bytes;
- explicit 32-bit offset in recoverable READ v0.2;
- successful retry after one deliberately lost READ response;
- successful reopen/resume after a deliberately invalidated file handle.

Representative KSC-03D status:

```text
RAM 463 B   IN drop 0   IR drop 0/0   HOST M/0 R2
```

`R2` is the cumulative count of the two deliberately injected and recovered
faults. `HOST M/0` indicates no final host error remained.

The protocol is intentionally bounded to the certified implementation. This
document does not claim arbitrary binary-safe terminal multiplexing, general
distributed transactions, or unlimited concurrent remote handles.

Stage preprint:
https://doi.org/10.5281/zenodo.23251546

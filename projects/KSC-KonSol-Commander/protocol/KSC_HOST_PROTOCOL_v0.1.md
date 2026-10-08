# KSC Host Protocol v0.1 - Framing Draft

Status: KSC-03A DESIGN BASELINE

## Objective

Provide an AVR-friendly framed machine channel that can coexist with normal KSC
terminal traffic on one serial connection.

The framing is deliberately small and binary.

## Frame format

Candidate v0.1 frame:

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

The exact SOF choice is provisional until KSC-03A proves that it coexists
cleanly with the existing terminal parser.

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

KSC-03A needs only PING_REQ/PING_RESP plus explicit malformed-frame handling.

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

KSC-03A physical certification must measure the actual whole-build flash/global
SRAM delta and runtime free SRAM.


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

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

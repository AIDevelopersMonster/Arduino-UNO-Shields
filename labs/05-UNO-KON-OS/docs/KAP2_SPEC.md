# KAP2 bytecode — KonSol 0.5 specification

KAP2 is the first control-flow extension of the external KonSol application
format.

Status: **physically verified by TEST-06 on Arduino UNO / ATmega328P**.
The host-side KASM source-to-bytecode path was reproducibly verified by TEST-07.

It keeps the KAP1 storage model:

- application remains a separate file on microSD;
- file is ASCII hexadecimal;
- VM streams and decodes the application directly from SD;
- whitespace between encoded bytes is ignored;
- resident KonSol services continue to own TFT, Touch, Serial and scheduling.

KAP1 compatibility is retained. The file header selects the VM level:

```text
4B415031 = KAP1
4B415032 = KAP2
```

## Registers

KAP2 introduces four unsigned 16-bit VM registers:

```text
R0 R1 R2 R3
```

They are initialized to zero at application start.

## Flags

KAP2 currently has one VM condition flag:

```text
Z = zero/equal flag
```

`CMPI` sets Z when the selected register equals the immediate value.
`INC`, `DEC`, `GET_TOUCH_X` and `GET_TOUCH_Y` also update Z according
to whether the resulting register value is zero.

## Common KAP1/KAP2 opcodes

| Opcode | Encoding | Meaning |
| --- | --- | --- |
| `10` | `10 cc` | clear TFT |
| `11` | `11 xx yy ss cc nn <data>` | draw text |
| `20` | `20 ll hh` | cooperative WAIT |
| `21` | `21` | WAIT_TOUCH |
| `30` | `30 nn <data>` | Serial output |
| `FF` | `FF` | EXIT |

## KAP2-only opcodes

| Opcode | Encoding | Meaning |
| --- | --- | --- |
| `40` | `40 rr ll hh` | MOVI Rr, imm16 |
| `41` | `41 rr` | INC Rr |
| `42` | `42 rr` | DEC Rr |
| `43` | `43 rr ll hh` | CMPI Rr, imm16 |
| `44` | `44` | MARK current stream position |
| `45` | `45` | JNZ MARK |
| `46` | `46 rr` | GET_TOUCH_X -> Rr |
| `47` | `47 rr` | GET_TOUCH_Y -> Rr |
| `48` | `48 xx yy ss cc rr` | draw decimal register value |
| `49` | `49` | JZ MARK |

Register index must be 0..3.

Immediate 16-bit values are little-endian.

## MARK / branch model

KAP2 deliberately starts with one compact branch target rather than a general
address space.

`MARK` records the current raw SD file position immediately after the opcode.
`JNZ` and `JZ` seek back to that saved position.

This is enough for a first externally stored loop while keeping SRAM overhead
very small.

## First KAP2 application

`COUNTER.KAP` demonstrates:

- persistent application state in R0;
- Touch event loop;
- Touch X/Y capture;
- conditional compare and branch;
- decimal register rendering;
- clean EXIT to the resident system.

The application waits for five touches. After every touch it increments R0,
captures Touch X/Y into R1/R2, redraws the counter and coordinates, and loops.
At count 5 it displays DONE and waits for one final touch before EXIT.

## Non-claims

KAP2 is not a general-purpose CPU ISA and is not intended to emulate AVR
machine code.

Version 0.5 is an experimental compact VM control layer designed to prove that
an external KonSol application can hold state and make its own control-flow
decisions while remaining inside the resident service model.


## KonSol 0.6 indexed-label extension

TEST-08 extends KAP2 without changing the `KAP2` file header.

New opcodes:

| Opcode | Encoding | Meaning |
| --- | --- | --- |
| `4A` | `4A ii` | define indexed LABEL `ii` |
| `4B` | `4B ii` | unconditional JMP to label `ii` |
| `4C` | `4C ii` | JZ to label `ii` |
| `4D` | `4D ii` | JNZ to label `ii` |

The current implementation provides eight label slots, IDs 0..7.

KASM keeps symbolic names in source and encodes them as compact IDs. On
application start, KonSol scans the KAP2 stream once, records the raw SD-file
position immediately after each LABEL opcode, then rewinds to the application
start. Branch instructions subsequently use direct SD `seek()`.

The persistent label table costs 17 bytes of SRAM:

```text
8 x uint16_t = 16 B
label mask   =  1 B
```

The KonSol 0.6 build confirms this exactly: global SRAM increased from 1239 B
in KonSol 0.5 to 1256 B in KonSol 0.6, a measured compile-time delta of
**+17 B**. Flash increased by 770 B.

The multi-label implementation limits raw KAP2 file size to 65535 bytes so
stored SD positions fit in `uint16_t`.

Legacy `MARK`, `JNZ MARK`, and `JZ MARK` remain supported for backward
compatibility with KonSol 0.5 applications.


## Current application I/O model

The detailed current interface is documented in
[KAP2_IO_MODEL.md](KAP2_IO_MODEL.md).

In summary, external KAP2 applications currently receive Touch events and Touch
X/Y coordinates and can output through resident TFT and Serial services.
microSD stores and streams applications, but arbitrary application-level
filesystem syscalls are not yet exposed.

Generic GPIO, ADC, PWM, I2C, controlled SPI-device access and additional UART
are future System-API work, not current KAP2 claims. KON-Boot/native Flash
programming is also a separate execution model and is intentionally outside the
KonSol 0.6 multi-label result.

# KASM - KonSol KAP assembler

KASM removes the need to hand-write hexadecimal KAP bytecode.

It is a small dependency-free Python command-line assembler for the KAP1/KAP2
formats used by KonSol.

## Example

Human-readable source:

    KAP2
    CLS BLACK
    MOVI R0 0
    MARK LOOP
    WAIT_TOUCH
    INC R0
    CMPI R0 5
    JNZ LOOP
    EXIT

Build from the repository root in PowerShell:

    python .\tools\kasm\kasm.py `
      .\labs\05-UNO-KON-OS\apps\KAP2\COUNTER.kasm `
      -o .\COUNTER.KAP

Expected result:

    KASM PASS: KAP2 -> COUNTER.KAP

The output remains the same ASCII-hex format that KonSol streams directly from
microSD.

## Supported syntax

Common instructions:

    KAP1
    KAP2
    CLS color
    TEXT x y scale color "text"
    WAIT milliseconds
    WAIT_TOUCH
    SERIAL "text"
    EXIT

KAP2 instructions:

    MOVI Rn value
    INC Rn
    DEC Rn
    CMPI Rn value
    MARK name
    JNZ name
    JZ name
    GET_TOUCH_X Rn
    GET_TOUCH_Y Rn
    DRAW_REG x y scale color Rn

Current VM limitation: there is one active MARK target. KASM enforces this so
source code cannot imply branch capabilities that KonSol 0.5 does not have.

Colors:

    BLACK WHITE CYAN YELLOW GREEN RED BLUE GREY

TFT X coordinates must be even because the KAP encoding stores X divided by 2.

## Design goal

KASM is deliberately a host-side tool. It does not consume UNO Flash or SRAM.
The resident KonSol VM stays small while applications can be written in a
readable source form and compiled on the PC.


## Verified result

The project reference source:

    labs\05-UNO-KON-OS\apps\KAP2\COUNTER.kasm

was assembled on Windows / PowerShell with the normal Python environment.

Observed:

    KASM PASS: KAP2 -> COUNTER.generated.KAP
    Instructions/records: 24

The generated output was compared with the physically certified
COUNTER.KAP reference and the comparison returned:

    True

This closes TEST-07: KASM reproduces the certified KAP2 bytecode exactly.

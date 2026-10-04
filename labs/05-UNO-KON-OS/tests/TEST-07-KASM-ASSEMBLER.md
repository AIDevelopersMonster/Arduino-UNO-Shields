# LAB-05 / TEST-07 - KASM source-to-KAP reproducibility

## Goal

Replace manual hexadecimal authoring with a readable host-side source language
without changing the KonSol 0.5 firmware or KAP2 bytecode format.

## Tool

[tools/kasm/kasm.py](../../../tools/kasm/kasm.py)

No third-party Python packages are required.

## Reference source

[apps/KAP2/COUNTER.kasm](../apps/KAP2/COUNTER.kasm)

The source expresses the same program already physically exercised as
COUNTER.KAP, but in readable form.

## Build

From the repository root:

    python .\tools\kasm\kasm.py `
      .\labs\05-UNO-KON-OS\apps\KAP2\COUNTER.kasm `
      -o .\COUNTER.generated.KAP

Expected:

    KASM PASS: KAP2 -> COUNTER.generated.KAP
    Instructions/records: 24

## Equivalence check

Compare the generated ASCII-hex program with the repository reference:

    $A = (Get-Content -Raw .\COUNTER.generated.KAP).Replace("`r","").Trim()
    $B = (Get-Content -Raw .\labs\05-UNO-KON-OS\apps\KAP2\COUNTER.KAP).Replace("`r","").Trim()
    $A -eq $B

Expected:

    True

## Physical check

Copy the generated KAP to the same microSD and run it with the existing KonSol
0.5 firmware.

No firmware rebuild is part of TEST-07.

## PASS criteria

1. KASM runs with the user's normal Python environment.
2. COUNTER.kasm assembles without error.
3. Generated .KAP is bytecode-equivalent to the known COUNTER.KAP.
4. KonSol 0.5 runs the generated program without firmware changes.
5. No manual opcode or ASCII-to-hex conversion is required for normal KAP2
   application development.

Status: **READY FOR CLI TEST**.

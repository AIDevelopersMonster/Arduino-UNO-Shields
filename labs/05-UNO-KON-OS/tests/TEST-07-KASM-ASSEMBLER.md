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

Observed on Windows / PowerShell:

    KASM PASS: KAP2 -> COUNTER.generated.KAP
    Instructions/records: 24

Result: **PASS**.

## Equivalence check

Compare the generated ASCII-hex program with the repository reference:

    $A = (Get-Content -Raw .\COUNTER.generated.KAP).Replace("`r","").Trim()
    $B = (Get-Content -Raw .\labs\05-UNO-KON-OS\apps\KAP2\COUNTER.KAP).Replace("`r","").Trim()
    $A -eq $B

Observed:

    True

Result: **EXACT SOURCE-TO-BYTECODE EQUIVALENCE PASS**.

## Physical check policy

A second hardware execution is not required when the generated file is proven
identical to the already physically verified COUNTER.KAP reference.

TEST-06 physically executed that reference on KonSol 0.5. TEST-07 then produced
the same normalized ASCII-hex bytecode stream from COUNTER.kasm and PowerShell
reported exact equality.

Therefore a duplicate hardware run would test file transfer rather than the
assembler transformation itself. A physical rerun remains useful whenever a
future KASM output differs from an already certified reference program.

## PASS criteria

1. KASM runs with the user's normal Python environment. **PASS**
2. COUNTER.kasm assembles without error. **PASS**
3. Generated .KAP is bytecode-equivalent to the known COUNTER.KAP. **PASS**
4. The equivalent reference COUNTER.KAP is already physically certified on
   unchanged KonSol 0.5 by TEST-06. **PASS BY IDENTICAL BYTECODE**
5. No manual opcode or ASCII-to-hex conversion is required for normal KAP2
   application development. **PASS**

Final result:

**TEST-07 FULL PASS — readable KASM source reproduces the physically certified
KAP2 bytecode exactly.**

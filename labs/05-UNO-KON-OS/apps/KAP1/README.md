# KonSol KAP1 applications

This directory contains external applications for the resident KonSol 0.4
environment.

A KAP1 file is **not an Arduino sketch and not AVR machine code**. It is
ASCII-hex encoded bytecode stored on microSD and interpreted by the resident
KonSol KAP1 VM.

## Applications

### HELLO.KAP

Physically verified first external application.

Behavior:

```text
clear screen
draw "HELLO FROM SD"
draw "TOUCH TO EXIT"
Serial -> "HELLO KAP1"
WAIT_TOUCH
EXIT
```

### ABOUT.KAP

Second independent application for TEST-05.

Physical bench result: **RUN / TFT / SERIAL PASS** on the same resident KonSol
0.4 firmware. The display showed `KONSOL 0.4`, `EXTERNAL APP`,
`RUNNING FROM SD`, and `TOUCH TO EXIT`; Serial produced `ABOUT KAP1`.
Final WAIT_TOUCH -> EXIT confirmation for this run remains to be recorded.

Behavior:

```text
clear screen
draw "KONSOL 0.4"
draw "EXTERNAL APP"
draw "RUNNING FROM SD"
draw "TOUCH TO EXIT"
Serial -> "ABOUT KAP1"
WAIT_TOUCH
EXIT
```

### DEMO.KAP

Third independent application for TEST-05. It also exercises cooperative
`WAIT`.

Physical bench result: **RUN / TFT timing / Serial / repeated EXIT / Touch
File Browser PASS**. The file was created from KonSol with
`WRITE`/`APPEND`, verified with `TYPE`, and executed twice. Both executions
produced `DEMO KAP1` followed by `APP EXIT 0`. The TFT sequence
`KAP1 DEMO -> ~1500 ms -> PROGRAM ON SD -> ~1500 ms -> TOUCH TO EXIT` was
confirmed, and the second launch was explicitly confirmed as a TFT Touch File
Browser launch.

Behavior:

```text
clear screen
draw "KAP1 DEMO"
WAIT 1500 ms
clear screen
draw "PROGRAM ON SD"
WAIT 1500 ms
draw "TOUCH TO EXIT"
Serial -> "DEMO KAP1"
WAIT_TOUCH
EXIT
```

## KAP1 header and opcodes

Every application starts with the decoded signature `KAP1`:

```text
4B415031
```

Initial KonSol 0.4 opcodes:

| Opcode | Meaning |
| --- | --- |
| `10 cc` | clear TFT with color index |
| `11 xx yy ss cc nn <data>` | draw text |
| `20 ll hh` | cooperative WAIT in milliseconds, little-endian |
| `21` | WAIT_TOUCH |
| `30 nn <data>` | Serial output |
| `FF` | EXIT |

The VM ignores spaces, TAB, CR and LF between encoded hexadecimal bytes.

## Run from KonSol

Copy the `.KAP` files to the root of the microSD card, then:

```text
A:/> DIR /
A:/> RUN /HELLO.KAP
A:/> RUN /ABOUT.KAP
A:/> RUN /DEMO.KAP
```

The same resident KonSol firmware must execute all applications. No Arduino
rebuild or reflash is part of TEST-05.

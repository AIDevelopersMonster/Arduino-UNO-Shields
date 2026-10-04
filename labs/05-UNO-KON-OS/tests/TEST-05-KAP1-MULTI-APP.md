# LAB-05 / TEST-05 — KAP1 multi-application certification

## Purpose

Strengthen the KonSol 0.4 external-application result without changing the
resident firmware.

TEST-04 proved one external application lifecycle:

```text
KonSol -> HELLO.KAP -> RUN -> resident services -> EXIT -> KonSol
```

TEST-05 asks a stronger question:

> Can one unchanged KonSol 0.4 firmware execute multiple independently stored
> KAP1 applications from microSD and return cleanly after each one?

No firmware modification, rebuild or upload is allowed between applications.

## Test applications

Repository:

`apps/KAP1/`

Applications:

```text
HELLO.KAP
ABOUT.KAP
DEMO.KAP
```

### HELLO.KAP

Already physically verified in TEST-04.

```text
HELLO FROM SD
TOUCH TO EXIT
```

Exercises TFT TEXT, Serial output, WAIT_TOUCH and EXIT.

### ABOUT.KAP

Physical bench status: **RUN / TFT / SERIAL PASS**. WAIT_TOUCH/EXIT confirmation
is still pending for this run.

Observed from the resident KonSol 0.4 shell:

```text
A:/> TYPE /ABOUT.KAP
-----
4B415031
1000
110A2802020A4B4F4E534F4C20302E34
110A5002030C45585445524E414C20415050
110A7802020F52554E4E494E472046524F4D205344
110A9B02030D544F55434820544F2045584954
300A41424F5554204B415031
21
FF
-----
A:/> RUN /ABOUT.KAP
APP RUN /ABOUT.KAP
A:/> ABOUT KAP1
```

The physical TFT displayed the expected independent application content:

```text
KONSOL 0.4
EXTERNAL APP
RUNNING FROM SD
TOUCH TO EXIT
```

This verifies that the unchanged resident KonSol firmware can parse and execute
a second bytecode stream with different TFT and Serial output.

Independent application:

```text
KONSOL 0.4
EXTERNAL APP
RUNNING FROM SD
TOUCH TO EXIT
```

Exercises multiple TFT TEXT instructions, Serial output, WAIT_TOUCH and EXIT.

### DEMO.KAP

Timed application:

```text
KAP1 DEMO
  -> WAIT 1500 ms
PROGRAM ON SD
  -> WAIT 1500 ms
TOUCH TO EXIT
```

Exercises repeated screen clear, TFT TEXT, cooperative WAIT, Serial output,
WAIT_TOUCH and EXIT.

## Create the files from KonSol itself

The applications can be copied from the repository, or created directly with
the resident shell.

### HELLO.KAP

```text
WRITE /HELLO.KAP 4B415031
APPEND /HELLO.KAP 1000
APPEND /HELLO.KAP 110A3C03030D48454C4C4F2046524F4D205344
APPEND /HELLO.KAP 110A7802030D544F55434820544F2045584954
APPEND /HELLO.KAP 300A48454C4C4F204B415031
APPEND /HELLO.KAP 21
APPEND /HELLO.KAP FF
```

### ABOUT.KAP

```text
WRITE /ABOUT.KAP 4B415031
APPEND /ABOUT.KAP 1000
APPEND /ABOUT.KAP 110A2802020A4B4F4E534F4C20302E34
APPEND /ABOUT.KAP 110A5002030C45585445524E414C20415050
APPEND /ABOUT.KAP 110A7802020F52554E4E494E472046524F4D205344
APPEND /ABOUT.KAP 110A9B02030D544F55434820544F2045584954
APPEND /ABOUT.KAP 300A41424F5554204B415031
APPEND /ABOUT.KAP 21
APPEND /ABOUT.KAP FF
```

### DEMO.KAP

```text
WRITE /DEMO.KAP 4B415031
APPEND /DEMO.KAP 1000
APPEND /DEMO.KAP 110A320302094B4150312044454D4F
APPEND /DEMO.KAP 20DC05
APPEND /DEMO.KAP 1000
APPEND /DEMO.KAP 110A4602030D50524F4752414D204F4E205344
APPEND /DEMO.KAP 20DC05
APPEND /DEMO.KAP 110A7802030D544F55434820544F2045584954
APPEND /DEMO.KAP 300944454D4F204B415031
APPEND /DEMO.KAP 21
APPEND /DEMO.KAP FF
```

For `20DC05`, KAP1 interprets `DC 05` as little-endian `0x05DC = 1500`
milliseconds.

## Pre-test

Use the already published and physically certified KonSol 0.4 firmware.

Do **not** recompile or upload another firmware for this test.

Check:

```text
INFO
MEM
PS
DIR /
```

Expected baseline:

```text
KonSol 0.4
TASKS: 5
APP VM: KAP1 streamed from SD
```

Record the actual `MEM` value.

## Serial launch sequence

Run each application independently:

```text
RUN /HELLO.KAP
APP
RUN /ABOUT.KAP
APP
RUN /DEMO.KAP
APP
MEM
DIR /
```

For applications containing `WAIT_TOUCH`, touch the TFT to allow execution to
reach `FF`.

Expected after every program:

```text
APP EXIT 0
APP: IDLE
LAST EXIT: 0
```

## Touch browser launch sequence

Open FILES and launch each `.KAP` from the TFT browser:

```text
FILES
 -> HELLO.KAP -> Touch -> EXIT
 -> ABOUT.KAP -> Touch -> EXIT
 -> DEMO.KAP  -> waits -> Touch -> EXIT
```

Return to the browser or dashboard between applications as appropriate.

## PASS criteria

TEST-05 passes only when all of the following are physically verified:

1. The **same KonSol 0.4 resident firmware** executes all three KAP1 files.
2. No Arduino compile/upload occurs between application runs.
3. All three applications are separate files on microSD.
4. HELLO.KAP displays its own content and exits.
5. ABOUT.KAP displays different content and exits.
6. DEMO.KAP demonstrates cooperative WAIT timing and exits.
7. Serial output is correct for all three programs.
8. WAIT_TOUCH is serviced by the resident Touch task.
9. Every `FF` returns to resident KonSol without reset.
10. The shell and SD filesystem remain usable after the sequence.
11. `MEM` remains stable enough to show no progressive application-lifecycle leak.
12. The Touch File Browser can launch the applications without reflashing.

## Meaning of a PASS

A TEST-05 PASS demonstrates that KAP1 is not a one-file special case. One
unchanged resident KonSol system can execute multiple external programs that
have different bytecode streams and behavior.

That strengthens the KonSol OS boundary:

```text
                 HELLO.KAP
                    |
                 ABOUT.KAP
                    |
resident KonSol ----+---- DEMO.KAP
                    |
             same kernel/services
                    |
                  EXIT
                    |
             return to KonSol
```

Status: **PARTIAL PHYSICAL PASS — HELLO.KAP and ABOUT.KAP execution confirmed; DEMO.KAP and final lifecycle checks pending**.

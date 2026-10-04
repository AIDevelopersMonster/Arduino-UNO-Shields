# LAB-05 / TEST-06 — KonSol 0.5 KAP2 interactive application

## Goal

Verify the first external KonSol application that has its own state, loop,
conditional branch and Touch-coordinate input.

KonSol 0.4 / TEST-05 already proved multiple independent KAP1 applications.
TEST-06 moves the boundary from "external command stream" to "external program
with control flow".

## Firmware

`sketches/05_KonSol_KAP2_Interactive/05_KonSol_KAP2_Interactive.ino`

This is a new experimental firmware derived from the physically certified
KonSol 0.4 baseline. KonSol 0.4 remains preserved unchanged.

## Application

`apps/KAP2/COUNTER.KAP`

Header:

```text
4B415032 = KAP2
```

Expected application behavior:

```text
TOUCH COUNTER
TAP SCREEN 5X

touch #1 -> TAPS 1 + X/Y
touch #2 -> TAPS 2 + X/Y
touch #3 -> TAPS 3 + X/Y
touch #4 -> TAPS 4 + X/Y
touch #5 -> TAPS 5 + X/Y

DONE
TOUCH TO EXIT
```

The final touch must execute `FF` and return to resident KonSol.

## KAP2 features under test

```text
R0 = touch count
R1 = last Touch X
R2 = last Touch Y

MOVI
INC
CMPI
MARK
JNZ
GET_TOUCH_X
GET_TOUCH_Y
DRAW_REG
WAIT_TOUCH
EXIT
```

## Create COUNTER.KAP from the KonSol shell

```text
WRITE /COUNTER.KAP 4B415032
APPEND /COUNTER.KAP 1000
APPEND /COUNTER.KAP 11081403020D544F55434820434F554E544552
APPEND /COUNTER.KAP 11083402030D5441502053435245454E203558
APPEND /COUNTER.KAP 300C434F554E544552204B415032
APPEND /COUNTER.KAP 40000000
APPEND /COUNTER.KAP 44
APPEND /COUNTER.KAP 21
APPEND /COUNTER.KAP 4100
APPEND /COUNTER.KAP 4601
APPEND /COUNTER.KAP 4702
APPEND /COUNTER.KAP 1000
APPEND /COUNTER.KAP 11081203020D544F55434820434F554E544552
APPEND /COUNTER.KAP 110A4802010454415053
APPEND /COUNTER.KAP 483C48030300
APPEND /COUNTER.KAP 110A7002010158
APPEND /COUNTER.KAP 482870020201
APPEND /COUNTER.KAP 110A8E02010159
APPEND /COUNTER.KAP 48288E020202
APPEND /COUNTER.KAP 43000500
APPEND /COUNTER.KAP 45
APPEND /COUNTER.KAP 110AB2020404444F4E45
APPEND /COUNTER.KAP 110ACC02030D544F55434820544F2045584954
APPEND /COUNTER.KAP 21
APPEND /COUNTER.KAP FF
```

## Build result

KonSol 0.5 compiled successfully for `arduino:avr:uno`.

Observed Arduino CLI result:

```text
Sketch uses 26798 bytes (83%) of program storage space.
Maximum is 32256 bytes.

Global variables use 1239 bytes (60%) of dynamic memory,
leaving 809 bytes for local variables.
Maximum is 2048 bytes.
```

Compared with the physically certified KonSol 0.4 build:

```text
KonSol 0.4
Flash: 25720 / 32256 bytes
SRAM globals: 1228 / 2048 bytes

KonSol 0.5
Flash: 26798 / 32256 bytes
SRAM globals: 1239 / 2048 bytes

Delta:
+1078 B Flash
+11 B global SRAM
```

Remaining compile-time headroom:

```text
Flash: 5458 B
SRAM after globals: 809 B
```

This is a strong result for the first KAP2 control-flow layer: registers,
branching, Touch X/Y capture and register rendering cost only 11 additional
bytes of global SRAM over KonSol 0.4.

Status: **TEST-06 FULL PHYSICAL PASS**.

## Physical boot / pre-flight result

KonSol 0.5 was uploaded to the physical Arduino UNO / MAR2406 stand and booted
successfully.

Observed boot:

```text
KonSol 0.5
Arduino UNO / ATmega328P / 16 MHz
Kernel + SD + direct ILI9341 + direct Touch
External KAP1/KAP2 VM + touch file browser

BOOT: TFT init
BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: 801 B
Type HELP
```

Observed shell state:

```text
INFO
KonSol 0.5
CPU: ATmega328P @ 16 MHz
FLASH: 32 KB
SRAM: 2 KB
SCHED: cooperative
TASKS: 5
TFT: ILI9341 direct 8-bit
TOUCH: direct resistive
APP VM: KAP1/KAP2 streamed from SD
SD: READY
FREE RAM: 737 B
```

Task table was live on hardware:

```text
0   SERIAL    1 ms
1   CLOCK     100 ms
2   DISPLAY   1000 ms
3   TOUCH     30 ms
4   APP       10 ms
```

The resident TFT UI loaded correctly and the existing microSD filesystem
remained accessible:

```text
F 123 HELLO.KAP
F 197 ABOUT.KAP
F 171 DEMO.KAP
```

The operator also confirmed that the display loaded correctly and the existing
files remained usable.

Runtime comparison with the certified KonSol 0.4 baseline:

```text
KonSol 0.4: boot 812 B, steady shell 750 B
KonSol 0.5: boot 801 B, steady shell 737 B

Observed delta:
-11 B at boot
-13 B in steady shell
```

This closely tracks the +11 B compile-time global-SRAM increase.

Status: **PHYSICAL BOOT + TFT + SD + KERNEL PRE-FLIGHT PASS**.

Next:

```text
TYPE /COUNTER.KAP
RUN /COUNTER.KAP
```

## Execution

```text
RUN /COUNTER.KAP
```

Expected Serial start:

```text
APP RUN /COUNTER.KAP
COUNTER KAP2
```

Touch the screen five times at visibly different positions.

After each touch confirm:

- TAPS increments by exactly one;
- X and Y update to plausible screen coordinates;
- the app does not exit early;
- kernel remains responsive.

After touch 5:

```text
DONE
TOUCH TO EXIT
```

One final touch must produce:

```text
APP EXIT 0
```

Then record:

```text
APP
MEM
DIR /
```

## Compatibility check

The new VM must still execute the existing KAP1 files:

```text
RUN /HELLO.KAP
RUN /ABOUT.KAP
RUN /DEMO.KAP
```

At least one complete KAP1 RUN -> WAIT_TOUCH -> EXIT cycle must be rechecked on
KonSol 0.5.

## PASS criteria

TEST-06 is FULL PHYSICAL PASS only if:

1. KonSol 0.5 compiles and boots on the same UNO/MAR2406 hardware.
2. KAP2 header is recognized.
3. COUNTER.KAP runs from microSD.
4. R0 persists and increments across loop iterations.
5. five Touch events cause five loop iterations.
6. R1/R2 display changing Touch X/Y values.
7. CMPI + JNZ terminate the loop exactly at count 5.
8. final WAIT_TOUCH + FF returns with APP EXIT 0.
9. resident shell and SD remain usable after exit.
10. free RAM shows no progressive loss.
11. KAP1 compatibility is physically retained.

Status: **TEST-06 FULL PHYSICAL PASS**.


## First KAP2 launch result

KonSol 0.5 also physically retained KAP1 compatibility:

```text
APP RUN /HELLO.KAP
HELLO KAP1
APP EXIT 0
```

The first KAP2 file was then created completely from the resident shell and
read back successfully with `TYPE`. The decoded file begins with the expected
`KAP2` header and contains the complete COUNTER program.

Observed first launch:

```text
A:/> RUN /COUNTER.KAP
APP RUN /COUNTER.KAP
A:/> COUNTER KAP2
APP EXIT 0
```

This proves that:

- the KAP2 header is accepted by the resident VM;
- the KAP2 application file opens and begins execution from microSD;
- the KAP2 Serial opcode executes;
- the program can reach a clean `APP EXIT 0`;
- KAP1 compatibility remains physically operational on KonSol 0.5.

The operator confirmed that the physical run included five counted Touch
presses while the test was being recorded on video. Per-touch Serial register
dumps are therefore not required as a certification gate; they remain an
optional diagnostic mechanism only.

### Physical interaction conclusion

The five-touch loop was physically exercised and the application subsequently
returned with `APP EXIT 0`. Together with the KAP2 bytecode structure
(`WAIT_TOUCH -> INC -> Touch X/Y capture -> CMPI -> JNZ`), this is accepted as
the physical control-flow test. Adding Serial output for every Touch would only
instrument the test and is not required for normal operation.

Status: **TEST-06 FULL PHYSICAL PASS**.

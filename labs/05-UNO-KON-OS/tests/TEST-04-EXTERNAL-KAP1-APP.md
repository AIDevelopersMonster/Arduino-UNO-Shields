# LAB-05 / TEST-04 — KonSol 0.4 External KAP1 Application VM

## Purpose

Cross the application-boundary milestone: run software stored on microSD through
resident KonSol services and return to the system without reflashing the UNO.

KonSol 0.4 keeps the resident cooperative kernel and adds a tiny streamed
application VM named **KAP1**.

## Firmware

`sketches/04_KonSol_External_APP_VM/04_KonSol_External_APP_VM.ino`

## Model

The application remains on microSD and is interpreted by the resident kernel.
The bytecode is streamed directly from the file, so the complete application is
not copied into SRAM.

```text
microSD .KAP
    |
    v
KAP1 VM task
    |
    +--> KonSol TFT service
    +--> KonSol Touch event
    +--> KonSol Serial service
    |
    v
EXIT -> resident KonSol dashboard
```

This is deliberately different from KON-Boot native-image loading. TEST-04 is
the resident-OS application path.

## Scheduler

KonSol 0.4 adds a fifth cooperative task:

```text
0 SERIAL    1 ms
1 CLOCK     100 ms
2 DISPLAY   1000 ms
3 TOUCH     30 ms
4 APP       10 ms
```

## KAP1 format

For the first certification version, the application file is ASCII hexadecimal.
Whitespace between encoded bytes is ignored. This makes an application easy to
create using the existing KonSol WRITE/APPEND shell without a PC-side binary
builder.

Decoded file header:

```text
4B 41 50 31   = "KAP1"
```

Opcodes:

```text
10 cc
  clear screen with color cc

11 xx yy ss cc nn <nn bytes>
  draw text
  x = xx * 2 pixels
  y = yy pixels
  scale = ss
  color = cc
  nn = text length

20 ll hh
  cooperative WAIT in milliseconds, little-endian

21
  WAIT_TOUCH

30 nn <nn bytes>
  print text through KonSol Serial service

FF
  EXIT back to resident KonSol
```

Color indices:

```text
0 BLACK
1 WHITE
2 CYAN
3 YELLOW
4 GREEN
5 RED
6 BLUE
7 GREY
```

## First external app

Create the first standalone application on SD as `/HELLO.KAP`.

The program:

1. clears the TFT;
2. prints `HELLO FROM SD`;
3. prints `TOUCH TO EXIT`;
4. writes `HELLO KAP1` to Serial;
5. waits for a Touch event;
6. exits back to KonSol.

Encoded KAP1 app:

```text
4B415031
1000
110A3C03030D48454C4C4F2046524F4D205344
110A7802030D544F55434820544F2045584954
300A48454C4C4F204B415031
21
FF
```

Create it from the existing shell:

```text
WRITE /HELLO.KAP 4B415031
APPEND /HELLO.KAP 1000
APPEND /HELLO.KAP 110A3C03030D48454C4C4F2046524F4D205344
APPEND /HELLO.KAP 110A7802030D544F55434820544F2045584954
APPEND /HELLO.KAP 300A48454C4C4F204B415031
APPEND /HELLO.KAP 21
APPEND /HELLO.KAP FF
TYPE /HELLO.KAP
```

## Build

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull

arduino-cli compile --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\04_KonSol_External_APP_VM
```

Verified build:

```text
Sketch: 25720 / 32256 bytes Flash (79%)
Globals: 1228 / 2048 bytes SRAM (59%)
Linker-reported SRAM remaining: 820 bytes
```

Build status: **PASS**.

Compared with KonSol 0.3:

```text
KonSol 0.3: 23666 B Flash / 1155 B globals
KonSol 0.4: 25720 B Flash / 1228 B globals

Delta: +2054 B Flash
       +73 B global SRAM
```

Flash headroom after the build is 6536 bytes. Runtime free RAM must still be
measured on physical hardware after TFT, Touch, SD and the KAP1 VM are active.

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\04_KonSol_External_APP_VM
```

## Serial test

```text
INFO
MEM
PS
DIR /
TYPE /HELLO.KAP
APP
RUN /HELLO.KAP
```

Expected launch:

```text
APP RUN /HELLO.KAP
HELLO KAP1
```

While the app waits for touch:

```text
APP
APP: RUNNING
```

After touching the display:

```text
APP EXIT 0
A:/>
```

Then:

```text
APP
MEM
PS
DIR /
```

The application must exit without reset and KonSol must still be operational.

## Touch browser launch

A `.KAP` file selected in the KonSol file browser is launched rather than
opened as a text document. This gives both shell and touch launch paths.

## PASS criteria

TEST-04 is **FULL PHYSICAL PASS** only when all of these are observed on the
real UNO:

1. resident KonSol 0.4 boots and mounts SD;
2. five cooperative tasks remain active;
3. HELLO.KAP exists only on SD;
4. `RUN /HELLO.KAP` launches it without reflashing;
5. external app renders through the KonSol TFT service;
6. external app emits Serial output;
7. WAIT_TOUCH yields cooperatively while kernel tasks continue;
8. touching the screen resumes the application;
9. `FF` exits the app and returns to the resident dashboard;
10. Shell, SD browser and filesystem still work after app exit;
11. repeated RUN → TOUCH → EXIT cycles do not reset or corrupt the system;
12. MEM remains stable enough for continued operation.

This test is the first strong OS-boundary certification for KonSol: application
content is separate from the resident kernel and can be changed on SD without
rebuilding or reflashing the firmware.


## Serial bench result

Physical Arduino UNO boot and shell verification:

```text
KonSol 0.4
BOOT: TFT init
BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: 812 B

INFO
TASKS: 5
TFT: ILI9341 direct 8-bit
TOUCH: direct resistive
APP VM: KAP1 streamed from SD
SD: READY
FREE RAM: 750 B

PS
0   SERIAL    1 ms
1   CLOCK     100 ms
2   DISPLAY   1000 ms
3   TOUCH     30 ms
4   APP       10 ms
```

HELLO.KAP was created entirely through the resident KonSol shell on microSD.
TYPE confirmed the expected encoded KAP1 contents.

The external application then executed from SD without reflashing:

```text
RUN /HELLO.KAP
APP RUN /HELLO.KAP
HELLO KAP1
APP EXIT 0
```

Repeated RUN cycles also returned with APP EXIT 0.

This certifies the Serial-side resident-kernel -> external-SD-app -> EXIT ->
resident-kernel path.

Final physical visual confirmation was then obtained on the real Arduino UNO +
MAR2406 display. The externally stored /HELLO.KAP application rendered:

```text
HELLO FROM SD
TOUCH TO EXIT
```

on the TFT after launch from the resident KonSol environment. Together with the
already verified Touch launch, WAIT_TOUCH/EXIT lifecycle, repeated RUN/EXIT
cycles, resident-shell recovery and stable memory measurements, this closes the
remaining visual gate for TEST-04.

Observed memory:

```text
Boot free RAM: 812 B
Shell INFO/MEM free RAM: 750 B
```

The shell prompt can appear before application Serial output because RUN starts
the APP task cooperatively and the command parser returns immediately. This is
a presentation-order issue, not an application lifecycle failure.


## Final physical certification

Physical TFT evidence confirmed the external KAP1 application executing from
microSD through resident KonSol services:

```text
HELLO FROM SD
TOUCH TO EXIT
```

The certification chain is now complete:

```text
resident KonSol 0.4
  -> microSD HELLO.KAP
  -> KAP1 APP task
  -> KonSol TFT service
  -> WAIT_TOUCH
  -> Touch event
  -> FF / EXIT
  -> resident KonSol dashboard / shell
```

Verified together on the physical Arduino UNO / ATmega328P:

- resident kernel remains in Flash;
- application content remains on microSD;
- application launches without firmware rebuild or reflashing;
- application draws through the resident direct ILI9341 service;
- application waits for and resumes on resident Touch events;
- application exits back to KonSol;
- repeated RUN/EXIT cycles do not reset the board;
- shell, SD filesystem and browser remain available after exit;
- observed shell free RAM remains 750 B after the tested lifecycle.

### TEST-04 final status

**FULL PHYSICAL PASS.**

This is the first strong KonSol OS-boundary certification: the resident system
and the external application are separate artifacts, and the application can be
changed on SD without changing the resident firmware.

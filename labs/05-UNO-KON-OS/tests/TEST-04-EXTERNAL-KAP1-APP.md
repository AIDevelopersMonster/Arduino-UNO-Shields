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

Record Flash and SRAM before upload.

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

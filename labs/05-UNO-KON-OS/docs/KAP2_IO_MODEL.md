# KonSol 0.6 — KAP2 input/output model

Status: **current physically verified interface through TEST-08**.

This document separates resident KonSol services from operations that are
already exposed to external KAP1/KAP2 applications.

## 1. Application input

Currently available to external KAP2:

```text
WAIT_TOUCH
GET_TOUCH_X Rn
GET_TOUCH_Y Rn
```

`WAIT_TOUCH` is cooperative: the APP task yields while the resident TOUCH task
continues sampling the screen.

`GET_TOUCH_X` and `GET_TOUCH_Y` copy the most recent Touch coordinates into
16-bit VM registers.

Serial input belongs to the resident Shell and is **not** currently a general
KAP2 input syscall.

## 2. Application output

Currently available to KAP1/KAP2:

```text
CLS color
TEXT x y scale color "text"
DRAW_REG x y scale color Rn
SERIAL "text"
```

The TFT operations use the resident direct ILI9341 service. Applications do not
own the LCD bus directly.

`SERIAL` is an application output service. It does not imply that arbitrary
Serial input is available to the application.

## 3. Application control services

Current KAP services also include:

```text
WAIT milliseconds
EXIT
```

KAP2 state/control flow:

```text
MOVI / INC / DEC / CMPI
legacy MARK / JZ / JNZ
KonSol 0.6 LABEL / JMP / JZ / JNZ
```

KonSol 0.6 supports up to eight indexed labels while retaining old MARK-based
bytecode.

## 4. microSD role

microSD is a resident system service with two verified roles:

1. filesystem for Shell / Touch File Browser;
2. storage for external `.KAP` applications streamed by the VM.

Current KAP2 does **not** expose a general application-level filesystem API such
as OPEN / READ / WRITE / CLOSE for arbitrary user data.

## 5. Hardware ownership on MAR2406

Verified wiring:

```text
LCD D0..D7 = UNO D8,D9,D2,D3,D4,D5,D6,D7
RD=A0, WR=A1, RS=A2, CS=A3, RST=A4

Touch:
XP=D6, XM=A2, YP=A1, YM=D7

microSD SPI:
CS=D10, MOSI=D11, MISO=D12, SCK=D13

Serial:
D0 / D1
```

The LCD and Touch share pins. The resident driver restores pin direction after
Touch sampling before display writes.

Because the shield consumes or shares most UNO pins, a future generic I/O API
must include explicit ownership/conflict rules rather than simply exposing raw
Arduino pin functions to applications.

## 6. Not yet part of the KAP System API

The following are planned/research directions, not current KAP2 capabilities:

```text
GPIO read/write
ADC read
PWM write
I2C transactions
controlled SPI device access
additional UART where hardware allows
application filesystem API
```

These services should be added only with a defined resident API, memory budget,
pin-ownership rules and physical certification.

## 7. KON-Boot / Flash Writer is separate

A future native Flash Writer is not part of KAP2 or KonSol 0.6.

```text
KAP2:
microSD -> resident VM -> app -> EXIT -> KonSol

KON-Boot:
microSD native image -> Boot Loader Section -> program Application Flash
                       -> verify -> reset/jump
```

KON-Boot requires separate work on BOOTSZ, image format, target checks, CRC,
bootloader protection, interrupted-update recovery and Flash endurance. It
should remain a separate TEST/publication line.

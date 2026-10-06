# KonSol 0.7 — KAP2 and device input/output model

Status: **current physically verified interface through TEST-10**.

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

For the assembled KonSol appliance used in LAB-05, these pins are not treated
as externally available application I/O. The physical device boundary is:

```text
Input:       resistive Touch + USB-TTL Serial
Output:      TFT + USB-TTL Serial
Storage:     microSD
Host link:   HOST1 over USB-TTL Serial
```

The device is therefore developed as a small self-contained computer, not as a
general Arduino GPIO controller.

## 6. System-level directions

Raw GPIO / ADC / PWM / I2C exposure is **out of scope for this assembled
LAB-05 device** because those interfaces are not brought out as part of its
external contract.

The next system-level services are host/application-platform features:

```text
HOST1 machine protocol
verified file transfer
remote application lifecycle
Host Manager
application metadata/catalog
on-device application launcher
future application-level filesystem API
```

TEST-09 physically verifies HOST1 and TEST-10 physically verifies the Host
Manager layer.

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

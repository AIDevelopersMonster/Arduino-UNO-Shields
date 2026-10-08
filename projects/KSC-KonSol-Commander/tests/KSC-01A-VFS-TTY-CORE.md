# KSC-01A - Virtual Device Filesystem / TTY Core

Status: **FULL PHYSICAL PASS**

Date: 2026-10-08

Target:

- Arduino UNO / ATmega328P
- HY-M302 multi-purpose shield
- HY_M302 library 0.2.0
- USB Serial, COM4
- 115200 8N1
- no SD card
- no local display

Firmware:

`projects/KSC-KonSol-Commander/sketches/01_KSC_VFS_TTY/01_KSC_VFS_TTY.ino`

## Purpose

Prove the first KSC architectural claim:

> live hardware and kernel information can be exposed through a filesystem-like
> namespace even when no physical filesystem exists.

This test does not yet certify the ANSI Commander UI or IR/keyboard navigation.
Those remain later KSC-01/KSC-02 work.

## Build

Command:

```powershell
arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\libraries `
  .\projects\KSC-KonSol-Commander\sketches\01_KSC_VFS_TTY
```

Observed:

```text
Flash: 10932 / 32256 bytes (33%)
SRAM globals: 995 / 2048 bytes (48%)
Linker-reported SRAM remaining: 1053 bytes
```

## Boot

Observed physical terminal:

```text
KSC 0.1
KonSol Commander - Virtual Device Filesystem
Arduino UNO / ATmega328P + HY-M302
Storage: virtual namespace only
FREE RAM: 1015 B
Type HELP

KSC:/>
```

Steady shell measurement:

```text
CAT /proc/mem
1047
```

The lower value printed during setup is consistent with the deeper setup call
stack. The steady shell value is the more useful runtime reference.

## Namespace PASS

Root:

```text
LS /
D dev
D proc
D sys
```

Device namespace:

```text
LS /dev
F sw1          RO
F sw2          RO
F pot          RO
F light        RO
D dht
D led
D rgb
F buzzer       RW
```

Kernel namespace:

```text
LS /proc
F mem          RO
F uptime       RO
```

System namespace:

```text
LS /sys
F version      RO
F target       RO
F storage      RO
```

## System nodes PASS

Observed:

```text
CAT /sys/version
KSC 0.1

CAT /sys/target
Arduino UNO + HY-M302

CAT /sys/storage
VIRTUAL ONLY
```

This physically confirms that the first KSC namespace is virtual and requires
no SD card.

## Live sensor nodes PASS

Observed physical values included:

```text
CAT /dev/light
294

CAT /dev/pot
0

CAT /dev/dht/temp
28.8

CAT /dev/dht/humidity
36.0

CAT /dev/dht/status
OK
```

After navigating through the virtual directory tree:

```text
CD /dev
CD dht
CAT temp
29.6

CAT humidity
36.0
```

The DHT value changed between reads, demonstrating that the node is backed by a
live hardware service rather than static text.

## Relative navigation PASS

Observed:

```text
CD /dev
OK
PWD
/dev

CD dht
OK
PWD
/dev/dht

CD ..
OK
PWD
/dev
```

Both absolute and relative virtual paths therefore work in the first prototype.

## Writable hardware nodes PASS

The operator physically confirmed that all tested outputs worked.

Verified through virtual paths:

- red discrete LED ON/OFF;
- blue discrete LED ON/OFF;
- RGB output through red/green/blue channel nodes;
- active buzzer ON/OFF.

Representative commands:

```text
WRITE /dev/led/red 1
WRITE /dev/led/red 0

WRITE /dev/led/blue 1
WRITE /dev/led/blue 0

WRITE /dev/rgb/red 255
WRITE /dev/rgb/green 0
WRITE /dev/rgb/blue 0

WRITE /dev/buzzer 1
WRITE /dev/buzzer 0
```

Readable state was also confirmed for the discrete LED and buzzer nodes.

## Terminal paste observation

Several malformed lines in the captured terminal session were caused by
multi-line paste/input concatenation in the monitor session, for example two
commands arriving as one line.

Examples produced expected parser errors such as:

```text
ERR NOT_FOUND
ERR VALUE
```

Repeating the same operations as individual commands succeeded, and the operator
confirmed the corresponding physical outputs.

This is not treated as a VFS or hardware-driver failure. It is an input/terminal
transport behavior to consider when the later ANSI keyboard layer is designed.

## Result

**KSC-01A FULL PHYSICAL PASS.**

Physically established:

```text
virtual path
    ->
VFS dispatch
    ->
HY-M302 driver / kernel service
    ->
real value or real physical output
```

The first KSC result therefore exists without SD, TFT, Touch, or an external
filesystem.

KSC-01 remains open for the next input-layer work:

- asynchronous IR as logical keyboard input;
- terminal arrow-key decoding;
- common KEY_* event model.

After that, KSC-02 can build the one-panel KonSol Commander interface on the
same VFS.

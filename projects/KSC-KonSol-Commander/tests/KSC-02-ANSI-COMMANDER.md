# KSC-02 - One-Panel ANSI KonSol Commander

Status: FULL PHYSICAL PASS

Target:

- Arduino UNO / ATmega328P
- HY-M302
- USB Serial 115200
- KSC Raw TTY 0.3
- asynchronous NEC IR
- no SD card required

## Purpose

KSC-02 is the first real one-panel KonSol Commander UI.

It combines the previously certified layers:

```text
KSC-01A  virtual VFS
KSC-01C  CHAR / KEY input model
          |
          v
KSC-02   ANSI one-panel Commander
```

The shell and Commander use the same VFS implementation.

## Virtual namespace

```text
/
+-- dev/
|   +-- sw1         RO
|   +-- sw2         RO
|   +-- pot         RO
|   +-- light       RO
|   +-- dht/
|   |   +-- temp        RO
|   |   +-- humidity    RO
|   |   +-- status      RO
|   |   +-- age         RO
|   +-- led/
|   |   +-- red         RW
|   |   +-- blue        RW
|   +-- rgb/
|   |   +-- red         RW 0..255
|   |   +-- green       RW 0..255
|   |   +-- blue        RW 0..255
|   +-- buzzer      RW 0..1
+-- proc/
|   +-- mem         RO
|   +-- uptime      RO
+-- sys/
    +-- version     RO
    +-- target      RO
    +-- storage     RO
```

## PC controls

```text
UP/DOWN        move selection
ENTER/RIGHT    open selected directory/node
LEFT/BACK      parent / return
HOME           root
END            last item
F9             help
F10            exit Commander to shell
Q              exit Commander to shell
0..9           numeric entry in RW node
DELETE         cancel numeric edit
```

## IR controls

```text
UP/DOWN        move selection
OK/RIGHT       open
LEFT/RETURN    parent / return
HOME           root
MENU           help
POWER          exit Commander to shell
0..9           numeric entry in RW node
```

## RW node behavior

For boolean nodes:

```text
/dev/led/red
/dev/led/blue
/dev/buzzer
```

valid range is 0..1.

For RGB channels:

```text
/dev/rgb/red
/dev/rgb/green
/dev/rgb/blue
```

valid range is 0..255.

Inside an RW node:

```text
digits       enter an exact value
ENTER        apply entered value
LEFT/RIGHT   decrement/increment and apply immediately
BACK         cancel active edit; otherwise return
```

## Build measurements

Arduino CLI compile result:

```text
Sketch uses 18026 bytes (55%) of program storage space.
Maximum is 32256 bytes.

Global variables use 1172 bytes (57%) of dynamic memory,
leaving 876 bytes for local variables.
Maximum is 2048 bytes.
```

Build status: PASS.

Boot runtime free RAM is 818 B. Commander-view runtime is 782 B during physical navigation.

## Boot runtime measurement

Physical startup on Arduino UNO + HY-M302:

```text
KSC 0.2
KonSol Commander - ANSI VFS Navigator
Arduino UNO / ATmega328P + HY-M302
IR INIT: OK
FREE RAM: 818 B
Storage: virtual namespace only
Type KSC to open Commander, HELP for shell commands.
```

Boot status: PASS.

Measured runtime free RAM after initialization: 818 B.

## First ANSI Commander runtime

The first real one-panel ANSI Commander screen was opened successfully on the
physical Arduino UNO + HY-M302 target.

Observed root screen:

```text
Path: /
----------------------------------------
> [dev/]
  [proc/]
  [sys/]
----------------------------------------
UP/DOWN Select   ENTER/RIGHT Open
LEFT/BACK Parent HOME Root
F9/MENU Help     F10/POWER/Q Shell
RAM 782 B   IR drop 0/0
```

Observed runtime state:

```text
Commander free RAM: 782 B
IR dropped edges:   0
IR dropped frames:  0
```

Interactive ANSI navigation was exercised on the physical target. The complete
node-by-node physical PASS gate below was subsequently reported as passed.

## Physical PASS gate

KSC-02 is FULL PHYSICAL PASS only after all of the following are observed on
real hardware:

1. Firmware compiles and uploads.
2. Boot reports IR INIT OK and runtime free RAM.
3. Typing KSC in the shell opens the ANSI Commander.
4. Root lists dev, proc, and sys.
5. PC UP/DOWN changes the highlighted selection.
6. PC ENTER/RIGHT opens directories.
7. PC LEFT/BACK returns to parent.
8. HOME returns to root.
9. A real RO sensor node can be viewed and refreshes.
10. /proc/mem and /sys/version are viewable.
11. A real RW LED node can be changed and the physical LED follows.
12. A real RGB channel can be set numerically and the physical RGB output follows.
13. Buzzer 0/1 changes the physical buzzer.
14. F9 opens Help and returns cleanly.
15. F10 or Q exits to the shell.
16. Shell remains operational after Commander exit.
17. IR UP/DOWN controls the same selection state.
18. IR OK opens the same selected object.
19. IR RETURN navigates back.
20. IR HOME returns to root.
21. IR MENU opens Help.
22. IR digits can set an RW value.
23. IR POWER exits to shell.
24. No input-event or IR drop condition is observed during normal test operation.
25. Flash, global SRAM, and runtime free RAM are recorded.

## Certification result

**KSC-02 FULL PHYSICAL PASS**

The complete physical test route was executed on the real Arduino UNO + HY-M302
target and reported as passing.

Certified behavior includes:

- one-panel ANSI directory navigation through the same VFS used by the shell;
- PC keyboard navigation and node control;
- HY-M302 IR remote navigation and node control;
- live RO sensor viewing;
- /proc and /sys node viewing;
- physical RW control of discrete LED, RGB channel, and buzzer;
- numeric RW entry through the CHAR input path;
- Help and clean return from Commander to the shell;
- continued shell operation after Commander exit;
- no observed IR drops during normal test operation.

Final observed system screen included:

```text
Path: /sys
----------------------------------------
> version  [RO]
  target  [RO]
  storage  [RO]
----------------------------------------
UP/DOWN Select   ENTER/RIGHT Open
LEFT/BACK Parent HOME Root
F9/MENU Help     F10/POWER/Q Shell
RAM 782 B   IR drop 0/0
```

and opening the version node produced:

```text
Path: /sys/version
----------------------------------------
Type: RO
Value: KSC 0.2

ENTER Refresh      LEFT/BACK Return
HOME Root          F9/MENU Help
F10/POWER/Q Shell
----------------------------------------
RAM 782 B
```

Recorded resource envelope:

```text
Flash:              18026 / 32256 B = 55%
Global SRAM:         1172 / 2048 B  = 57%
Boot free RAM:       818 B
Commander free RAM:  782 B
IR drops:             0 / 0 observed
```

## Suggested first physical route

```text
shell
  -> KSC
  -> /dev
  -> light
  -> back
  -> dht
  -> temp
  -> back
  -> back
  -> led
  -> red
  -> set 1
  -> verify physical LED
  -> set 0
  -> verify physical LED
  -> back
  -> rgb
  -> red
  -> enter 128
  -> verify physical RGB
  -> back
  -> back
  -> buzzer
  -> set 1
  -> verify buzzer
  -> set 0
  -> HOME
  -> /proc/mem
  -> HOME
  -> /sys/version
  -> F9 help
  -> F10 shell
```

Repeat the navigation/control portion with the IR remote before certification.

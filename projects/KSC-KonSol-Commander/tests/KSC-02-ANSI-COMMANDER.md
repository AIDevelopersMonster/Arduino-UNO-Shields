# KSC-02 - One-Panel ANSI KonSol Commander

Status: TEST READY

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

Boot runtime free RAM is measured at 818 B; Commander-view runtime is still to be observed during physical navigation.

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

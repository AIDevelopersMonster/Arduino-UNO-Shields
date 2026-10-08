# KSC-01B - Unified Keyboard Layer

Status: FULL PHYSICAL PASS

Date: 2026-10-08

Target:

- Arduino UNO / ATmega328P
- HY-M302
- HY_M302 0.2.0
- USB Serial 115200
- asynchronous NEC IR
- Windows KSC Raw TTY

## Build

Flash: 6300 / 32256 bytes (19%)
SRAM globals: 636 / 2048 bytes (31%)
Boot free RAM: 1399 B

## IR input PASS

Physically verified:

UP
DOWN
LEFT
RIGHT
OK -> ENTER
RETURN -> BACK
HOME
MENU
POWER
0..9

IR diagnostic result:

dropped_edges=0
dropped_frames=0

The digit 8 was explicitly retested and produced:

KEY SRC=IR CODE=8

## TTY input PASS

KSC Raw TTY physically verified:

UP
DOWN
LEFT
RIGHT
ENTER
BACKSPACE -> BACK
ESC -> BACK
HOME
MENU
POWER
0..9

## Architectural result

Two independent physical input paths can produce the same logical KSC key:

PC keyboard -> TTY/ANSI -> KSC key
IR remote   -> NEC       -> KSC key

The source remains distinguishable as SRC=TTY or SRC=IR.

## Limitation discovered

KSC-01B intentionally mapped printable characters such as digits into KSC keys.

This is sufficient to prove source unification, but it is not the correct final
input model because Shell and Commander need ordinary printable characters.

The next stage therefore separates:

CHAR - printable input
KEY  - special/navigation input

Status: FULL PHYSICAL PASS.

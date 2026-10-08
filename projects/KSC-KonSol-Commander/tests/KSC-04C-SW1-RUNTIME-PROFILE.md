# KSC-04C - SW1 Runtime Profile

Status: **IMPLEMENTED - BUILD/PHYSICAL TEST PENDING**

## Goal

Demonstrate that a remote KSC program changes the later behavior of a physical
button without recompiling, uploading, or resetting the Arduino UNO.

The firmware exposes one resident writable VFS node:

```text
/sys/sw1
```

Values:

```text
0 = SW1 press -> discrete RED LED ON, BLUE OFF
1 = SW1 press -> discrete BLUE LED ON, RED OFF
```

Writing the profile clears both discrete LEDs. The selected behavior then
persists in RAM until another script changes `/sys/sw1` or the board resets.

SW1 is edge/debounce processed in the target service loop. A stable press is
accepted after 25 ms and triggers the currently loaded profile.

## Programs

### SW1_0.KSC

```text
PRINT SW1_0 LOAD RED PROFILE
WRITE /sys/sw1 0
PRINT PRESS SW1 FOR RED
STOP
```

### SW1_1.KSC

```text
PRINT SW1_1 LOAD BLUE PROFILE
WRITE /sys/sw1 1
PRINT PRESS SW1 FOR BLUE
STOP
```

## Required physical sequence

1. Run `/host/DEMOS/SW1_0.KSC`.
2. Confirm `RUN OK`.
3. Press SW1.
4. Confirm RED LED ON and BLUE LED OFF.
5. Run `/host/DEMOS/SW1_1.KSC` without reset or reflash.
6. Confirm `RUN OK`.
7. Press the same physical SW1.
8. Confirm BLUE LED ON and RED LED OFF.
9. Confirm final status has no input, IR, or host errors.

Expected final status target:

```text
IN drop 0
IR drop 0/0
HOST M/0
```

## Bounded claim if PASS

**KSC-04C demonstrates that a streamed remote KSC program can change resident
runtime input-to-output behavior on Arduino UNO, so the same physical button
performs a different action after loading a different host-side program,
without firmware recompilation, upload, or reset.**

Non-claims:

- this is one fixed SW1 profile slot, not a general event-binding language;
- the profile is RAM-resident and is lost on reset;
- only RED/BLUE discrete LED actions are certified in KSC-04C;
- persistent storage, arbitrary event rules, conditions, and multiple bindings
  are outside this UNO-stage test.

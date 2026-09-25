# LAB-02 test plan

## Preconditions

- Arduino UNO compatible board works by itself.
- Multi-Function Shield is mounted correctly.
- No optional module is installed in D0/D1, D2 or A4 positions.
- GUI is optional; all firmware commands remain available over serial.
- Serial speed is 115200 baud.

## Test 01 — firmware / serial

1. Upload `01_MFS_Lab.ino`.
2. Open a 115200-baud terminal.
3. Reset the UNO.
4. Confirm `@SYS,READY,MFSHIELD,LAB02,0.1`.
5. Send `PING`; expect `@PONG`.
6. Send `INFO`; record the reported configuration.

Pass condition: stable serial exchange without resets or corrupted lines.

## Test 02 — LEDs

From GUI or terminal:

- `LED,1,ON` ... `LED,4,ON`
- switch each one OFF again
- `LED,ALL,ON`
- `LED,ALL,OFF`

Pass condition: each physical D1-D4 follows only its own commanded state.

## Test 03 — buttons

Watch the BUTTONS tab or `@STATE`.

Press S1, S2 and S3 individually at least five times.

Pass condition:

- no button is permanently asserted;
- each button maps to its expected A1/A2/A3 position;
- press counter increments once per deliberate press;
- pressing one button does not increment another.

## Test 04 — potentiometer

Rotate VR-A0 from one stop to the other and back.

Record:

- observed minimum;
- observed maximum;
- stability near both ends.

Do not require exact 0 and 1023; the measured physical range is the evidence.

## Test 05 — display

Send:

- `DISP,0000`
- `DISP,1234`
- `DISP,8888`

Check:

- digit order;
- all four digit positions;
- all seven segments;
- visible flicker;
- brightness consistency.

Start with `CFG,DIGITSEL,STD`.
Use `CFG,DIGITSEL,INV` only if the physical clone proves the standard
digit-select polarity is wrong.

## Test 06 — buzzer

The physical sample is treated as an **active / self-oscillating buzzer**.

Bench result:

- D3 HIGH gives a strong sustained sound;
- D3 LOW is quiet;
- applying frequency/PWM control caused a large drop in loudness.

Therefore frequency control has been removed from both the firmware protocol and
the GUI for this shield.

Canonical test:

1. `BUZ,ON`
2. hold for about 1 second
3. `BUZ,OFF`
4. `BEEP,200`

Pass condition: strong sustained sound while ON, silence while OFF, and a clear
short beep from `BEEP,200`.

## Test 07 — automated stimulus sequence

Send `TEST,ALL`.

Expected order:

1. display 8888;
2. LED D1;
3. LED D2;
4. LED D3;
5. LED D4;
6. active-buzzer DC stimulus;
7. current potentiometer value on the display;
8. `@TEST,DONE`.

This sequence is for repeatable observation and video capture. It does not
self-certify physical PASS.

## Result recording

Write measured results in `../results/README.md`.
Only after the physical test should provisional hardware statements be promoted
to verified statements.

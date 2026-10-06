# HY_M302 Arduino Library

Low-overhead Arduino UNO library for the HY-M302 multi-purpose shield.

Status: **v0.2.0 / modular driver architecture / async NEC + symbolic remote-key layer / TEST-07 cooperative control PHYSICAL PASS**.

The library intentionally starts with its own small hardware routines instead of
pulling in large third-party dependencies. This keeps the Flash/SRAM cost visible
and gives us room to decide later which functions deserve dedicated low-level
drivers.

## Included now

- SW1 / SW2
- LED1 / LED2
- bench-certified RGB control: D9=R, D10=G, D11=B
- bench-certified active/self-oscillating buzzer on D5
- potentiometer
- LDR
- LM35 raw + Celsius conversion
- minimal DHT11 reader
- bench-verified non-blocking NEC IR receiver on D6
- configurable pin map

## Important status

The physical HY-M302 silkscreen confirms the board-level pin allocation.
TEST-01 on the physical shield has now bench-certified the LED mapping:

- D9 = RGB RED;
- D10 = RGB GREEN;
- D11 = RGB BLUE;
- RGB uses direct polarity: 0 = off, 255 = full channel;
- D12 = red discrete LED;
- D13 = blue discrete LED.

The library therefore exposes named `setRGB(red, green, blue)`, `ledRed()` and
`ledBlue()` helpers while retaining the raw channel API for diagnostics.

The DHT11 routine is deliberately small and dependency-free. It is now
**PHYSICAL PASS** on the tested HY-M302.

Reference comparison on the same board:

```text
Adafruit reference: 30.3 C, 39-41 % RH
HY_M302 low-level:  30.2 C, 39-40 % RH
```

The first in-house implementation failed because it measured DHT pulses with
`micros()` while interrupts were disabled. The corrected AVR implementation
reads the input port directly and measures pulse lengths using loop counts,
removing the Timer0 timing dependency.

A compact in-house NEC decoder is included. The physical D6 receiver has now
been verified independently with Arduino-IRremote using an Orange Pi remote:
protocol NEC, address 0x04. The reference run produced 12 unique commands:
0x13, 0x10, 0x11, 0x0F, 0x0C, 0x0D, 0x0B, 0x08, 0x09, 0x58, 0x47 and 0x53.

The first in-house NEC test exposed two blocking-decoder bugs, both now
corrected: the initial timeout was shorter than the approximately 9 ms NEC
leader LOW pulse, and a subsequent pulseIn(HIGH) skipped the already-active
leader HIGH pulse.

After those fixes, the in-house decoder produced the same 12 unique frames as
Arduino-IRremote on the Orange Pi remote:

```text
RAW       ADDR CMD
EC13FB04  04   13
EF10FB04  04   10
EE11FB04  04   11
F00FFB04  04   0F
F30CFB04  04   0C
F20DFB04  04   0D
F40BFB04  04   0B
F708FB04  04   08
F609FB04  04   09
A758FB04  04   58
B847FB04  04   47
AC53FB04  04   53
```

Semantic decode status is therefore PASS for these frames. The earlier blocking
implementation still produced one timeout in the comparison run and had to
discard trailing NEC repeat frames.

A new non-blocking AVR/UNO receiver is now implemented as the KonSol-oriented
path. On the standard HY-M302 wiring, D6/PD6/PCINT22 captures edge timing in a
minimal ISR. The main code calls `serviceIrNec()`, which runs the NEC state
machine outside the ISR and queues decoded frames. The driver exposes dropped
edge/frame counters so physical robustness can be measured explicitly.

The asynchronous path has now passed a physical 12-button comparison run on the
same Orange Pi remote. Every full frame matched Arduino-IRremote exactly:

```text
EC13FB04 / 04 / 13
EF10FB04 / 04 / 10
EE11FB04 / 04 / 11
F00FFB04 / 04 / 0F
F30CFB04 / 04 / 0C
F20DFB04 / 04 / 0D
F40BFB04 / 04 / 0B
F708FB04 / 04 / 08
F609FB04 / 04 / 09
A758FB04 / 04 / 58
B847FB04 / 04 / 47
AC53FB04 / 04 / 53
```

Status: **ASYNC NEC PHYSICAL PASS — functional correctness 12/12**.

The live stress run is also complete. Full NEC frames and repeat frames were
received continuously while the driver reported:

```text
IR STATS dropped_edges=0 dropped_frames=0
```

Status: **ASYNC NEC ROBUSTNESS PASS on the tested UNO + HY-M302 sample**.

The old blocking `readIrNec()` remains available only for comparison.


## Modular source layout

The library is no longer implemented as one monolithic `HY_M302.cpp`.
Drivers are split so the AVR linker can omit modules that an application does
not reference:

```text
src/
  HY_M302.h
  HY_M302_Core.cpp
  HY_M302_Service.cpp
  HY_M302_Analog.cpp
  HY_M302_DHT11.cpp
  HY_M302_Buzzer.cpp
  HY_M302_RGB.cpp
  HY_M302_IR_NEC.cpp
```

The asynchronous IR ring buffer and ISR state live inside the IR translation
unit rather than inside every `HY_M302` object. A sketch that does not use the
IR API therefore does not pay the IR SRAM cost.

`service()` is the common cooperative hook for asynchronous drivers:

```cpp
HY_M302 shield;

void setup() {
  shield.begin();
  shield.beginIrNecAsync();
}

void loop() {
  shield.service();
}
```

## Arduino IDE installation

Copy or symlink this directory into the Arduino libraries directory as
`HY_M302`, then restart Arduino IDE.

Examples appear under:

    File -> Examples -> HY_M302

## Memory policy

Target platform:

- ATmega328P
- 32 KB Flash
- 2 KB SRAM

The project prefers:

- static storage;
- no Arduino String in core library code;
- no dynamic allocation;
- small direct drivers when large generic libraries would consume too much
  Flash/SRAM.

This library will also become the hardware-abstraction layer for the later
KonSol-HY project.


## Bench-certified analog behavior

LDR direction is bench-certified on the physical HY-M302 sample:

- ordinary room light: about 368 ADC counts;
- LDR covered: about 56-61 ADC counts;
- therefore brighter -> higher ADC reading, darker -> lower ADC reading.

These are not universal thresholds; they document the verified direction of the
divider on our sample.


## LM35 status on tested shield

The physical LM35 fitted to our HY-M302 sample failed a cooling-response test.

Observed A2 raw readings remained about 93 -> 91 -> 90 -> 89 -> 89 despite
deliberate cooling, so this specific sensor is treated as defective.

The library retains `readLm35Raw()` and `readLm35C()` because the A2 mapping is
still valid and other HY-M302 samples or a replacement LM35 may work correctly.
Do not use the current tested board's LM35 readings as a certified temperature
measurement.


## DHT11 physical certification

Status: **PASS**.

Physical procedure:

1. verify the onboard DHT11 on D4 with the Adafruit reference library;
2. verify the same sensor with the HY_M302 dependency-free low-level driver;
3. compare repeated temperature/humidity results.

Observed:

```text
Adafruit:
30.3 C / 39.0 %
30.3 C / 41.0 %

HY_M302:
30.2 C / 39.0 %
30.2 C / 40.0 %
30.2 C / 40.0 %
30.2 C / 40.0 %
```

The two implementations agree within the normal resolution/variation expected
for this class of sensor.


## Buzzer physical certification

Status: **PASS as active/self-oscillating buzzer on the tested sample**.

Manual bench behavior:

- constant HIGH on D5 produces the strongest sustained sound;
- LOW / OFF silences the buzzer;
- frequency-driven `tone()` output is weaker and is retained only as an
  optional compatibility/experiment path.

The primary API is therefore:

```cpp
shield.buzzerOn();
shield.buzzerOff();
```

`buzzerTone()` remains available, but it is not the preferred control mode for
the tested HY-M302 sample.


## Non-blocking NEC API

For Arduino UNO + standard HY-M302 D6 wiring:

```cpp
shield.beginIrNecAsync();

void loop() {
  shield.serviceIrNec();

  HY_M302::IrNecFrame frame;
  while (shield.readIrNecAsync(frame)) {
    // handle full or repeat NEC frame
  }
}
```

Diagnostic counters:

```cpp
shield.irNecDroppedEdges();
shield.irNecDroppedFrames();
shield.resetIrNecStats();
```

The interrupt handler records only edge duration/level into a fixed ring buffer.
Protocol decoding and frame validation are performed outside the ISR.


## TEST-05 integrated-board certification

The new example `05_Integrated_Board` is the physical integration gate for the
modular library.

It runs the already certified functions through one `HY_M302` instance:

- SW1 -> red discrete LED;
- SW2 -> blue discrete LED;
- potentiometer -> red RGB brightness;
- LDR/POT/analog snapshot once per second;
- DHT11 read every 3 seconds;
- non-blocking NEC reception continuously through `service()`;
- active buzzer through explicit commands.

Commands:

```text
RUN
STOP
DHT
SNAP
BUZZ ON
BUZZ OFF
STATS
ZERO
?
```

The important integration question is DHT11 versus asynchronous IR. The current
DHT11 implementation briefly disables interrupts for its timing-critical read.
TEST-05 is intentionally designed to expose any resulting IR loss instead of
hiding it.

The original 0.1.0 promotion gate has been superseded by the later physical
remote-menu integration tests. The library is now promoted to **0.2.0**.


## Remote Mapper and TEST-07 physical pass

A host-side CLI/GUI mapper now learns a physical NEC remote through the onboard
D6 receiver and generates a reusable symbolic key profile.

The tested iDroid / Orange Pi remote profile contains 19 learned keys:

```text
0 1 2 3 4 5 6 7 8 9
OK HOME RETURN MENU
UP DOWN LEFT RIGHT
POWER
```

The independently checked OK key is:

```text
RAW=0xA35CFB04 ADDR=0x04 CMD=0x5C
```

The generated profile is stored under:

```text
profiles/HY-M302-Remotes/iDroid-OrangePi/
```

Example `07_Remote_Test_Menu` was compiled and physically exercised on the
UNO + HY-M302. The remote selection/command path works on hardware.

Observed build size before the fully cooperative follow-up refactor:

```text
Flash: 10786 / 32256 bytes (33%)
SRAM:    747 / 2048 bytes (36%)
free SRAM reported by the build: 1301 bytes
```

Status: **REMOTE TEST MENU PHYSICAL PASS**.

The fully cooperative follow-up implementation was then physically retested.
RETURN, HOME and POWER remain responsive while a test is active, including the
timed button, POT, LDR, RGB, buzzer and IR diagnostic paths.

Status: **TEST-07 COOPERATIVE CONTROL PHYSICAL PASS**.


## Built-in remote-key layer

Starting with library 0.2.0, application sketches no longer need to carry a
private generated remote map beside the sketch.

Generic logical keys are provided by:

```cpp
#include <HY_M302_Remote.h>
```

The physically learned iDroid / Orange Pi profile is provided by:

```cpp
#include <HY_M302_Remote_iDroid_OrangePi.h>
```

Typical decode path:

```cpp
using namespace HY_M302_Remote;

Key key = IDroidOrangePi::decode(frame.address, frame.command);

if (key == KEY_OK) {
  // logical action
}
```

The generic layer also exposes `isDigit()`, `digit()` and `keyName()`.
The learned command table is stored in AVR program memory (`PROGMEM`) rather
than ordinary SRAM.

The canonical reproducible mapper output remains under
`profiles/HY-M302-Remotes/iDroid-OrangePi/`. That directory is the learned
source profile; the library module is the runtime integration of that verified
profile.

TEST-07 now consumes the library profile directly. Its former private
`HY_M302_RemoteMap.h/.cpp` copies have been removed.

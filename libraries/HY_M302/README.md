# HY_M302 Arduino Library

Low-overhead Arduino UNO library for the HY-M302 multi-purpose shield.

Status: **v0.1 experimental**.

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
- IR pin exposure for the next driver stage
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

The first in-house NEC test timed out because its pulseIn timeout was only
5000 us while the NEC leader LOW pulse itself is about 9000 us. The decoder
timeout has been corrected to 15000 us and is pending physical re-test before
the in-house decoder is promoted to PASS.

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

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
- passive buzzer using AVR/Arduino tone support
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

The DHT11 routine is deliberately small and dependency-free. It must be tested
on the physical shield before being promoted to PASS.

IR decoding is not yet included in v0.1. We will first determine the actual
remote/protocol requirements and then decide whether a compact in-house decoder
is preferable to a full external library.

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

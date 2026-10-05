# Project 01 — HY-M302 KonSol Operating Environment

## Concept

A special-purpose KonSol operating environment for Arduino UNO / ATmega328P +
HY-M302.

The HY-M302 is treated as the base I/O board. Additional hardware is attached
through optional **drivers** and must explicitly declare its pin/resource use.

Core targets:

- USB Serial console;
- Bluetooth serial transport;
- IR input;
- onboard sensors and actuators;
- optional SD storage;
- optional display;
- compact external measurement/control programs;
- one resident cooperative kernel.

This is not a claim of a general-purpose OS. The target is a small, measurable,
driver-based operating environment inside the UNO resource envelope.

## Base HY-M302 hardware map

Current silkscreen evidence on our physical sample establishes:

| Service | UNO resource |
| --- | --- |
| SW1 | D2 |
| SW2 | D3 |
| DHT11 | D4 |
| passive buzzer | D5 |
| IR receiver | D6 |
| expansion GPIO | D7 |
| expansion GPIO | D8 |
| RGB LED | D9-D11 |
| LED2 | D12 |
| LED1 | D13 |
| potentiometer | A0 |
| LDR | A1 |
| LM35 | A2 |
| expansion analog/GPIO | A3 |
| I2C SDA/SCL | A4/A5 |
| UART / Bluetooth | D0/D1 |

Exact RGB channel ordering and electrical polarity remain subject to physical
certification.

## Driver model

KonSol-HY should not hard-code every possible peripheral into the kernel.

Target layering:

    applications / scripts
            |
        system API
            |
    +-------+--------+----------------+
    |                |                |
    sensor drivers   actuator drivers external drivers
    |                |                |
    DHT11            RGB              SD
    LM35             LED1/LED2        DISPLAY
    LDR              buzzer           BLUETOOTH
    POT              GPIO             I2C devices
    IR

Each driver declares:

- pins/resources used;
- initialization routine;
- service calls;
- periodic task, if required;
- RAM/Flash cost where practical;
- conflicts with other drivers.

This makes resource conflicts an explicit system property instead of an
accidental wiring failure.

## Resource ownership and conflicts

The shield already consumes most UNO pins, so optional drivers cannot simply
assume the standard Arduino pin map.

### Hardware SPI conflict

UNO hardware SPI is:

- D11 MOSI
- D12 MISO
- D13 SCK

HY-M302 uses D11-D13 for onboard LEDs/RGB functions.

Therefore an SD or SPI display driver has several possible modes:

1. **exclusive hardware-SPI mode**
   - reclaim D11-D13 from onboard LED use;
   - fastest option;
   - onboard LED functions on those pins become unavailable while active.

2. **software-SPI mode**
   - use a selected set of expansion/analog pins;
   - slower but avoids the fixed D11-D13 hardware-SPI assignment;
   - requires enough free pins.

3. **shared bus with explicit ownership**
   - possible only after the exact external circuit and onboard loading are
     physically verified;
   - never assumed safe merely because SPI electrically exists.

The OS must know which mode is active.

## SD driver

SD is feasible as an optional KonSol-HY driver.

Possible roles:

- filesystem;
- external program storage;
- logs;
- configuration;
- measurement history;
- application packages.

Target service API:

    SD MOUNT
    SD INFO
    DIR
    TYPE <file>
    WRITE <file> <data>
    APPEND <file> <data>
    RUN <file>

The first implementation should prefer a reproducible wiring profile instead of
assuming the usual UNO D10-D13 SD wiring.

### Candidate software-SPI profile

One candidate for bench evaluation is to use free/repurposable pins such as:

    SCK  = D7
    MOSI = D8
    MISO = A3
    CS   = A4 or A5

This is only a design candidate until physically tested. If A4/A5 are needed for
I2C, a different allocation is required.

A software-SPI-capable library or a minimal custom SPI/SD layer may be required;
the classic Arduino SD library is optimized around the hardware SPI path.

## Display driver

A display is also feasible and should be treated as another optional driver.

The preferred display type depends on the pin budget.

### I2C display

Most attractive first option:

- SDA = A4
- SCL = A5
- only two signal pins;
- compatible with a small OLED/LCD module;
- leaves D7/D8/A3 available for other functions.

This is the cleanest display path for KonSol-HY.

### UART display

Possible through D0/D1, but this competes directly with:

- USB Serial;
- Bluetooth serial.

It therefore requires transport arbitration or a software-UART alternative.

### SPI display

Possible, but must share the same resource analysis as SD.

An SPI display and SD may eventually share one SPI bus with separate CS lines,
but only after the selected hardware and HY-M302 loading are physically tested.

## Driver registration concept

A future kernel may expose a compact driver table:

    DRIVER SERIAL   READY
    DRIVER HYIO     READY
    DRIVER IR       READY
    DRIVER SD       OFF
    DRIVER DISPLAY  OFF
    DRIVER BT       OFF

Commands:

    DRIVERS
    DRIVER SD ON
    DRIVER DISPLAY ON
    DRIVER BT ON

If activation would cause a resource collision, the kernel should reject it:

    ERR PIN_CONFLICT D13
    ERR RESOURCE SPI
    ERR UART_BUSY

This is more valuable than silently allowing two libraries to fight for the same
pins or timer.

## One command language, multiple transports

USB Serial and Bluetooth should feed the same command parser:

    PC CLI -------- USB Serial ----+
                                   |
    PC GUI -------- USB Serial ----+--> KonSol-HY parser --> services
                                   |
    Android app --- Bluetooth -----+

The transport changes; the logical API does not.

## IR as a resident event driver

The IR receiver on D6 becomes a system input driver rather than a one-off
example.

Possible events:

    IR_CODE <value>
    BUTTON 1 DOWN
    BUTTON 2 DOWN
    TIMER <id>
    SENSOR <id> <value>

Applications can react to these events without directly owning the hardware.

## Measurement and actuator programs

The external-program model should cover both measurement and control.

Candidate operations:

    READ
    SET
    CMP
    JMP
    JZ
    JNZ
    WAIT
    EVENT
    PRINT
    LOG
    EXIT

Examples:

    READ LIGHT R0
    CMP R0 700
    JZ DARK
    SET RGB 0 255 0
    WAIT 500
    EXIT

or:

    READ DHT_TEMP R0
    PRINT R0
    LOG /TEMP.CSV R0
    WAIT 5000
    JMP LOOP

The exact text/bytecode format will be fixed only after memory measurements.

## Storage hierarchy

KonSol-HY can support progressively richer storage:

1. Flash-resident kernel;
2. EEPROM for small persistent settings/program fragments;
3. upload over USB Serial;
4. upload over Bluetooth Serial;
5. optional SD filesystem;
6. host-streamed programs if SD is absent.

So lack of onboard SD does not block the architecture.

## Proposed project stages

### P0 — HY-M302 hardware certification

Verify all onboard functions and exact RGB/polarity behavior.

### P1 — HY-M302 Arduino library

Create one verified device abstraction usable from ordinary Arduino IDE
sketches.

### P2 — KonSol-HY kernel

Add scheduler, shell, memory diagnostics, device table and event layer.

### P3 — Built-in HY-M302 drivers

Sensors, buttons, RGB, LEDs, buzzer, IR and GPIO.

### P4 — PC protocol, CLI and GUI

One serial protocol for engineering control and diagnostics.

### P5 — Bluetooth driver

Attach a TTL Bluetooth module and expose the same logical protocol.

### P6 — Android client

Phone front end for telemetry, control and application management.

### P7 — SD driver

Add optional external storage with explicit pin/resource profile.

### P8 — Display driver

Start with an I2C display because it has the lowest pin cost. Later evaluate
SPI displays and shared SD/display bus configurations.

### P9 — External application model

Run compact measurement/control programs through resident services.

## KonSol relationship

Reuse from existing KonSol:

- cooperative scheduling;
- resident service ownership;
- static memory discipline;
- shell;
- application lifecycle;
- compact VM/state-machine ideas;
- KASM-style host tooling where useful.

Do not blindly copy MAR2406 assumptions. KAP1/KAP2 are a conceptual and
implementation baseline, not yet a binary-compatibility promise.

## Resource philosophy

The ATmega328P still provides only:

- 32 KB Flash;
- 2 KB SRAM;
- 1 KB EEPROM;
- 16 MHz.

The narrow resource envelope is the point of the project.

A driver is accepted only when its measured Flash/SRAM cost and pin ownership
remain compatible with the selected system configuration.

## Non-claims

Until physically demonstrated, this project does not claim:

- simultaneous activation of every HY-M302 peripheral;
- universal SPI compatibility;
- KAP1/KAP2 binary compatibility;
- protected processes;
- preemptive multitasking;
- memory protection;
- hard real-time behavior.

Every capability moves from DESIGN to PASS only through a reproducible physical
test.

## Current status

**DESIGN OPENED.**

The first dependency is physical certification of the HY-M302 board and the
shared Arduino library.

Related:

- [HY-M302 shield documentation](../../shields/HY-M302-Multi-Purpose-Shield/)
- [LAB-05 KonSol / KON-OS](../../labs/05-UNO-KON-OS/)

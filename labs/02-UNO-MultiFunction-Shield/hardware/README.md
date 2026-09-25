# LAB-02 hardware notes

## Physical sample

The project photo shows the classic Arduino UNO Multi-Function Shield layout:

- four-digit 7-segment display;
- four red LEDs;
- S1, S2, S3 and RESET push buttons;
- `VR-A0` potentiometer;
- `LS1-3` buzzer position;
- `U4-IR-2` optional IR-receiver position;
- `U5-18B20-LM35-A4` temperature-sensor position;
- APC220 / Bluetooth / Voice Recognition Module serial header;
- two display-driver shift-register IC positions.

## APC220 / serial expansion header

Family-level documentation for the common Funduino-style board gives this
7-pin mapping:

| Pin | Function |
| ---: | --- |
| 1 | GND |
| 2 | +5 V |
| 3 | NC |
| 4 | D1 / TX |
| 5 | D0 / RX |
| 6 | NC |
| 7 | NC |

This is the UNO hardware UART, not an independent serial port. D0/D1 are shared
with the USB-serial interface, so an attached APC220/Bluetooth/voice module can
conflict with upload or diagnostics.

Before powering an unknown module, confirm header orientation and continuity on
the actual shield.

## J1 / J2 working reference

Common Funduino schematic convention:

- J1: enables a 10 kOhm pull-up on A4 for the U5 temperature-sensor signal.
- J2: enables 10 kOhm pull-ups on A1/A2/A3 for S1/S2/S3.

Operational consequence:

- DS18B20: J1 normally closed.
- LM35: J1 normally open.
- S1/S2/S3: J2 may be closed for the shield's external pull-ups.
- LAB-02 uses `INPUT_PULLUP`, therefore S1/S2/S3 also work with J2 open.

Some clone descriptions reverse the J1/J2 labels. For our sample, continuity
measurement has priority over Internet silkscreen conventions.

## Bench items to certify

Record these from the physical sample before calling the shield revision fully
identified:

1. top markings of both shift-register ICs;
2. top marking / type of the four-digit display if readable;
3. verify whether J1 is the A4 10 kOhm pull-up and record installed/open state;
4. verify whether J2 is the A1/A2/A3 button pull-up enable and record installed/open state;
5. whether the buzzer behaves as active, passive, or both;
6. standard or inverted display digit-select polarity;
7. continuity of the optional expansion headers to D5/D6/D9/A5;
8. continuity of D0/D1 to the serial expansion header.

## Safety / conflict note

Do not connect an external UART module to D0/D1 during the first GUI test. The
UNO USB serial path uses the hardware UART and an external module can interfere
with upload or diagnostics.

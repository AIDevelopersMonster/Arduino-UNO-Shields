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

## Bench items to certify

Record these from the physical sample before calling the shield revision fully
identified:

1. top markings of both shift-register ICs;
2. top marking / type of the four-digit display if readable;
3. J1 function and installed/open state;
4. J2 function and installed/open state;
5. whether the buzzer behaves as active, passive, or both;
6. standard or inverted display digit-select polarity;
7. continuity of the optional expansion headers to D5/D6/D9/A5;
8. continuity of D0/D1 to the serial expansion header.

## Safety / conflict note

Do not connect an external UART module to D0/D1 during the first GUI test. The
UNO USB serial path uses the hardware UART and an external module can interfere
with upload or diagnostics.

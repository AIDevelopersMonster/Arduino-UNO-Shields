# Stage-01 physical test matrix

All entries **NOT RUN**. A bench PASS must include specimen ID, firmware SHA, wiring sketch, tester, observed output and date. Test sketch output alone is never a physical PASS.

| ID | Test / stimulus | PASS criterion | Initial state |
| --- | --- | --- | --- |
| T01 | Both board surfaces, pin labels, solder and clearances | photos + signed specimen ID | NOT RUN |
| T02 | Unpowered 5V–GND and terminal isolation check | no shorts; measured readings logged | NOT RUN |
| T03 | Unpowered NO-COM / NC-COM each of 4 | expected continuity verified per silkscreen | NOT RUN |
| T04 | Trace digital inputs to R1–R4, logic driver | four unique actual pins recorded | NOT RUN |
| T05 | Input active level and polarity, no loads | each channel matches observation, others unchanged | NOT RUN |
| T06 | RESET and initial power-on with passive scope/LED | actual unexpected actuation documented; do not assume PASS if glitch | NOT RUN |
| T07 | UNO + shield supply current (0..4 relays) | measured I / V; no resets or abnormal heating | NOT RUN |
| T08 | ON/OFF individually, low-voltage pilot load | only selected NO/NC switches and returns | NOT RUN |
| T09 | USB terminal round-trip at 115200 | reported input stable, no unexpected GPIO | NOT RUN |
| T10 | Compile UNO from IDE 1.8.19 and arduino-cli | compiler pass with core/toolchain recorded | NOT RUN |
| T11 | RS-485 wiring inspection and direction idle | DE=0 at reset, no collisions before energized bus | NOT RUN |
| T12 | Stack check against existing W5100/HY-M302/TFT | measured resources, collisions documented, not blanket PASS | NOT RUN |

## Required rig for T01–T10

Arduino UNO 5V board, 4-relay shield, multimeter, USB cable with suitable 5V current budget, logic probe or oscilloscope if available, non-conductive spacing, low-voltage pilot LED/test load only for T08. Never connect mains for this lab.

## Operator protocol

1. Disconnect all supplies and loads. Photograph both faces.
2. Measure and record expected contact topology before installing shield onto UNO.
3. Test the passive PinProbe firmware with outputs left INPUT. Check that it does not produce relay clicks.
4. Determine logical pin mapping by continuity on the *unpowered* board, not a guessed map.
5. Independently confirm active-level (high/low) and no abnormal boot behavior before enabling any output test.
6. Record uncertainties separately; PASS is forbidden if a channel cannot be independently attributed.
7. Record UART upload contention and any unexpected resets.

## Stage-01 verdict

DOCUMENTATION STARTED / BENCH PENDING. No physical PASS and no relay board factory conformity claim.

# LAB-01 Results

Verified results from the physical stand go here.

## A1 — Arduino UNO smoke test

**Date:** 2026-09-25  
**Sketch:** `01_UNO_Smoke`  
**Upload path:** Arduino CLI  
**Board target:** `arduino:avr:uno`  
**Serial port:** COM3  
**Serial Monitor:** 115200 baud  
**Result:** PASS

Observed Serial Monitor output:

```text
LAB-01 / 01_UNO_Smoke
Arduino UNO execution: PASS
Built-in LED should toggle every second.
```

Notes:

- Arduino CLI upload is confirmed working.
- `arduino-cli monitor` keeps COM3 open; stop the monitor with `Ctrl+C` before the next upload.
- The same board also uploads successfully through Arduino IDE.

## A2 — W5100 Ethernet

**Date:** 2026-09-25  
**Sketches:** `02_W5100_DHCP`, `02A_W5100_Diagnostics v2`  
**Ethernet library:** 2.0.2  
**Result:** BLOCKED — controller not detected

Observed output:

```text
LAB-01 / 02A_W5100_Diagnostics v2
Step 1: initialize WIZnet controller without DHCP...
Hardware: NO HARDWARE
RESULT: Ethernet controller not detected after initialization.
```

Interpretation:

- DHCP is not yet the issue because the W5100 is not detected over SPI.
- The RobotDyn/Arduino W5100 architecture uses D10 as an active-low chip select.
- D10 HIGH disables W5100; D10 LOW selects it.
- The inverter present on classic W5100 shield designs is for W5100 SEN handling and does not reverse the Arduino-facing D10 chip-select semantics.

## A3 — SD 4 GB

**Date:** 2026-09-25  
**Sketch:** `03_SD_Card`  
**Result:** BLOCKED — SD initialization failed

Observed output:

```text
LAB-01 / 03_SD_Card
Initializing SD...
SD init: FAIL
Check card format, insertion, D4 CS and shield.
```

Interpretation:

- W5100 and SD fail independently while using different chip-select pins (D10 and D4).
- Both depend on the shield's common SPI path.
- Mechanical ICSP mating was visually confirmed.
- The old RobotDyn shield is therefore deferred as suspect hardware rather than spending more time on component-level fault isolation.

## Current LAB-01 status

- Arduino UNO clone: PASS
- Arduino CLI workflow: PASS
- W5100 shield: DEFERRED / SUSPECT HARDWARE
- SD interface on the same shield: DEFERRED
- LAB-01 will resume with replacement Ethernet shield hardware.

## Result-recording rule

For every test record:

- date;
- Arduino UNO board identification;
- W5100 shield identification/revision;
- SD card identification;
- Arduino IDE / Arduino CLI version;
- Ethernet library version if known;
- SD library version if known;
- sketch name;
- Serial Monitor output;
- PASS / FAIL;
- notes and photographs when useful.

No expected result should be recorded as a measured result before the hardware test.

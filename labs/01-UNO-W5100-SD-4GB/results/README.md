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

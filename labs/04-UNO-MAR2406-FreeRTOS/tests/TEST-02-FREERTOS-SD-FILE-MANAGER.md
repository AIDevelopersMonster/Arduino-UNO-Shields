# LAB-04 / TEST-02 — FreeRTOS SD File Manager

## Goal

Evaluate whether Arduino UNO R3 / ATmega328P can run a practical FreeRTOS-based
microSD file manager over USB Serial while using the MAR2406 shield.

## Result

**CLOSED — FAIL AS A PRACTICAL ARCHITECTURE ON THIS HARDWARE**

The experiment produced a useful boundary result.

### Attempt 1 — FreeRTOS + SD + full TFT stack

Combined:

- FreeRTOS
- SD/SPI
- MCUFRIEND_kbv
- Adafruit_GFX
- Serial file-transfer protocol

Result:

- build failed because the text section exceeded Arduino UNO Flash.

Conclusion:

The full LAB-03 graphics stack plus FreeRTOS plus SD/file-transfer logic does
not fit the ATmega328P program-memory budget in this architecture.

### Attempt 2 — staged FreeRTOS + SD + Serial only

TFT/GFX were removed.

Verified build:

```text
Sketch: 21166 / 32256 bytes Flash (65%)
Globals: 1137 / 2048 bytes SRAM (55%)
Linker-reported SRAM remaining: 911 bytes
```

Physical startup with a 256-byte FILE task stack:

```text
BOOT0 FRTOSFM/1
SD INIT BEGIN
SD INIT PASS
TASK CREATE PASS
RTOS FILE TASK RUNNING
OK FRTOSFM/1 SD=READY BLOCK=32 BAUD=115200
PING
OK PONG FRTOSFM/1
INFO
OK FRTOSFM/1 SD=READY BLOCK=32 BAUD=115200
```

The first real filesystem operation:

```text
LS /
```

did not complete.

### Attempt 3 — larger FILE task stack

The FILE task stack was increased from 256 to 384 bytes.

Observed:

```text
BOOT0 FRTOSFM/1
SD INIT BEGIN
SD INIT PASS
TASK CREATE PASS
```

but the FILE task did not reach:

```text
RTOS FILE TASK RUNNING
```

This is consistent with the runtime SRAM margin becoming insufficient once the
larger task stack and FreeRTOS runtime structures are included.

## Engineering conclusion

TEST-02 is closed as unsuccessful for practical use on the classic UNO.

What was proven:

- FreeRTOS itself works on the UNO + MAR2406 platform;
- FreeRTOS + TFT + Touch was physically demonstrated in TEST-01;
- SD initialization works;
- FreeRTOS + SD + Serial command handling can start;
- the limiting resource for the staged SD build is runtime SRAM;
- the full TFT + SD + FreeRTOS design also exceeds Flash.

What was not achieved reliably:

- directory listing under FreeRTOS;
- GET/PUT file transfer;
- a usable FreeRTOS file manager.

This is not a claim that every possible FreeRTOS/SD implementation on an
ATmega328P is impossible. It is a project decision that further optimization is
not justified for this hardware and objective.

## Recommended path

For a file manager on this exact Arduino UNO + MAR2406 shield:

- use the normal Arduino execution model without FreeRTOS;
- keep SD + Serial + optional TFT/Touch;
- use the already verified LAB-03 SD hardware path.

For RTOS + filesystem + GUI work, move to a device with substantially more
SRAM/Flash.

## Firmware retained

The experimental firmware is retained for reproducibility:

`sketches/02_FreeRTOS_SD_File_Manager/02_FreeRTOS_SD_File_Manager.ino`

Status: **ARCHIVED EXPERIMENT / FAIL**.

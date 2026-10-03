# TEST-06 — microSD read/write certification

## Purpose

TEST-06 certifies the microSD interface on the MAR2406 2.4-inch TFT shield before combining SD with touch/HMI code.

The verified shield SD signal map is:

```text
SD_SS / CS   -> D10
SD_DI / MOSI -> D11
SD_DO / MISO -> D12
SD_SCK       -> D13
```

The test is deliberately independent from the resistive touch layer.

## Test sequence

1. Initialize the SD card with `SD.begin(10)`.
2. List the root directory to Serial.
3. Create temporary file `LAB03.TST`.
4. Write exactly 512 deterministic bytes.
5. Close and reopen the file.
6. Verify file size and every byte.
7. Delete the temporary file.
8. Show PASS/FAIL on both TFT and Serial.

The deterministic byte pattern is:

```text
byte[i] = (i * 37 + 11) & 0xFF
```

This avoids treating a successful `SD.begin()` as sufficient proof. TEST-06 requires an actual write/read round trip.

## PASS criteria

PASS only if all of the following succeed:

- SD initialization;
- temporary file creation;
- 512-byte write;
- reopen for reading;
- exact 512-byte verification.

Temporary-file removal is also reported, but does not redefine the basic electrical read/write result.

## Firmware

`sketches/06_SD_Read_Write/06_SD_Read_Write.ino`

## Build

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\06_SD_Read_Write
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\03-UNO-MAR2406-TFT\sketches\06_SD_Read_Write
```

## Monitor

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Expected successful ending:

```text
SD INIT: PASS
WRITE 512 B: PASS
REOPEN: PASS
VERIFY 512 B: PASS
REMOVE TEMP: PASS

TEST-06 PASS
```

The root-directory listing may contain any user files already present on the card and is informational only.

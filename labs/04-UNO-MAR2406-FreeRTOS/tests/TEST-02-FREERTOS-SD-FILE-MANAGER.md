# LAB-04 / TEST-02 — FreeRTOS SD File Manager

## Goal

Turn the MAR2406 microSD slot into a real removable file store reachable from a
Windows PC over the Arduino UNO USB serial connection.

The built-in LED heartbeat from TEST-01 is removed because D13 is SPI SCK for
the SD card.

## Flash-limit result from the first build

The first TEST-02 revision combined:

- FreeRTOS
- SD/SPI
- MCUFRIEND_kbv
- Adafruit_GFX
- Serial file-transfer protocol

On the Arduino UNO / ATmega328P that build exceeded the available program Flash
and the linker reported that the text section did not fit.

That is a useful architectural result: the full LAB-03 graphics stack plus
FreeRTOS plus the SD filesystem is too large for this firmware layout.

TEST-02 therefore now uses a staged design.

## TEST-02A architecture

```text
Windows PC
   |
 USB / COM4
   |
 FreeRTOS FILE task
   |
 SPI / SD
   |
 microSD
```

The TFT and Touch are deliberately not linked into this build. The physical
shield may remain installed; only its microSD interface is used.

Once file transfer is certified, a later test can reintroduce status graphics
with a much smaller direct ILI9341 driver instead of MCUFRIEND_kbv +
Adafruit_GFX.

## Firmware

`sketches/02_FreeRTOS_SD_File_Manager/02_FreeRTOS_SD_File_Manager.ino`

## Protocol

Protocol identifier: `FRTOSFM/1`

Commands:

```text
PING
INFO
MOUNT
LS [path]
GET <path>
PUT <size> <path>
```

### LIST

```text
LS /
```

Response format:

```text
BEGIN LS
F<TAB>123<TAB>TEST.TXT
D<TAB>0<TAB>LEVELS/
END LS
```

### GET

Host:

```text
GET /TEST.TXT
```

UNO:

```text
DATA <size> 32
```

UNO then sends up to 32 raw bytes. After each block the PC sends:

```text
ACK
```

After the last block:

```text
END <CRC16>
```

### PUT

Host:

```text
PUT <size> /TEST.TXT
```

UNO:

```text
READY 32
```

The PC sends one block of at most 32 raw bytes and waits for:

```text
ACK <total_received>
```

After the final block:

```text
OK <size> <CRC16>
```

CRC is CRC-16/CCITT, polynomial 0x1021, initial value 0xFFFF.

The 32-byte block size is intentional: it keeps the transfer comfortably below
the AVR serial RX-buffer limit while the SD library performs card writes.

## Build

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull

arduino-cli compile --fqbn arduino:avr:uno .\labs\04-UNO-MAR2406-FreeRTOS\sketches\02_FreeRTOS_SD_File_Manager
```

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\04-UNO-MAR2406-FreeRTOS\sketches\02_FreeRTOS_SD_File_Manager
```

## First bench check

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Expected boot:

```text
BOOT FRTOSFM/1
OK FRTOSFM/1 SD=READY BLOCK=32 BAUD=115200
```

Then test:

```text
PING
INFO
LS /
```

## Verified build result

```text
Sketch: 21166 / 32256 bytes Flash (65%)
Globals: 1137 / 2048 bytes SRAM (55%)
Linker-reported SRAM remaining: 911 bytes
```

Build status: **PASS**.

This confirms that the staged architecture fits comfortably in Flash once
MCUFRIEND_kbv and Adafruit_GFX are removed.

The SRAM margin is now the critical resource. The 911-byte linker remainder is
not equal to the final runtime free RAM: FreeRTOS task/idle stacks and control
structures are allocated after startup. TEST-02A therefore must be judged on
physical stability during SD initialization, directory listing and file
transfer, not only on the compiler report.

## PASS criteria

TEST-02A passes when:

1. the firmware fits in UNO Flash;
2. SD initializes as READY;
3. PING returns OK PONG FRTOSFM/1;
4. LS / returns the real card directory;
5. repeated listings do not reset or hang the board.

TEST-02B then adds the Windows host utility and verifies PUT/GET round-trip
with CRC.


## Physical startup result

Observed on the real Arduino UNO + MAR2406 shield:

```text
BOOT0 FRTOSFM/1
SD INIT BEGIN
SD INIT PASS
TASK CREATE PASS
RTOS FILE TASK RUNNING
OK FRTOSFM/1 SD=READY BLOCK=32 BAUD=115200
```

This physically confirms:

- microSD initialization succeeds on D10-D13;
- the FreeRTOS FILE task is created successfully;
- the scheduler runs the FILE task;
- the serial protocol server reaches its command loop;
- the SD card is reported READY after the scheduler starts.

Status: **STARTUP PASS / DIRECTORY TEST NEXT**.

The white TFT screen in TEST-02A is expected because MCUFRIEND_kbv and
Adafruit_GFX are intentionally not linked in this staged build.

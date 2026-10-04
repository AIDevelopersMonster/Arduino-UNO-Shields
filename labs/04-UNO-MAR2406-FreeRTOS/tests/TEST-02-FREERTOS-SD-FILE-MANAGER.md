# LAB-04 / TEST-02 — FreeRTOS SD File Manager

## Goal

Turn the MAR2406 microSD slot into a real removable file store reachable from a
Windows PC over the Arduino UNO USB serial connection, while FreeRTOS keeps the
HMI responsive.

The built-in LED heartbeat from TEST-01 is removed. D13 is SPI SCK for the SD
card, so it must not be used as a status LED during SD traffic.

## Architecture

```text
Windows PC
   |
 USB / COM4
   |
 FILE task
   |
 SPI / SD
   |
 microSD

HMI task
   |
 TFT status screen
```

The FILE task owns Serial and the SD card. The HMI task owns the TFT. Touch is
not part of TEST-02 yet; this keeps the first transfer test focused and avoids
mixing two new subsystems at once.

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
RM <path>
MKDIR <path>
RMDIR <path>
```

### LIST

Example:

```text
LS /
```

Response:

```text
BEGIN LS
F<TAB>123<TAB>TEST.TXT
D<TAB>0<TAB>LEVELS/
END LS
```

### GET

Host sends:

```text
GET /TEST.TXT
```

UNO replies:

```text
DATA <size> 32
```

Then UNO sends up to 32 raw bytes. After each block the PC sends:

```text
ACK
```

After the last block UNO sends:

```text
END <CRC16>
```

CRC is CRC-16/CCITT with polynomial 0x1021 and initial value 0xFFFF.

### PUT

Host sends:

```text
PUT <size> /TEST.TXT
```

UNO replies:

```text
READY 32
```

The PC sends at most 32 raw bytes per block. After every block UNO replies:

```text
ACK <total_received>
```

After the last block UNO replies:

```text
OK <size> <CRC16>
```

The deliberately small 32-byte transport block prevents the AVR serial RX
buffer from overflowing while the SD library is performing a sector write.

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

## First serial check

```powershell
arduino-cli monitor -p COM4 -c baudrate=115200
```

Expected boot output:

```text
BOOT FRTOSFM/1
OK FRTOSFM/1 SD=READY BLOCK=32 BAUD=115200
```

Then type:

```text
PING
INFO
LS /
```

## PASS criteria

TEST-02A passes when:

1. SD initializes as READY;
2. `PING` returns `OK PONG FRTOSFM/1`;
3. `LS /` returns the actual directory contents;
4. the TFT remains alive and shows the current operation;
5. no reset or display corruption occurs during repeated directory listing.

After TEST-02A we add the Windows CLI utility for PUT/GET and verify binary file
round-trip with CRC.

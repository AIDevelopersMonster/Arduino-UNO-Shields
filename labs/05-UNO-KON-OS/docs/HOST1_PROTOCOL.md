# KonSol HOST1 protocol — physically verified TEST-09/TEST-10

Status: **FULL PHYSICAL PASS with KonSol 0.7**.

HOST1 is the machine-oriented protocol carried over the same 115200 8N1
USB-TTL Serial link as the human KonSol Shell.

Machine commands begin with `@`.

## Device boundary

```text
PC / host
   |
USB-TTL Serial
   |
HOST1
   |
KonSol 0.7
   |
microSD + KAP1/KAP2 VM + TFT/Touch
```

## Commands

```text
@PING
@INFO
@MEM
@PS
@LS [path]
@GET <path>
@PUTB <path>
@PUTD <path> <hex-bytes>
@PUTE <path> <size> <crc16>
@DEL <path>
@RUN <file.KAP>
@APP
@STOP
```

## Typical responses

```text
@OK PONG HOST1
@OK INFO V=0.7 HOST=1 SD=1 APP=0 RAM=652 TASKS=5
@OK MEM 652
@TASK ...
@END PS 5
@F <size> <name>
@D <name>
@END LS <count>
@OK PUTB
@OK PUTD <bytes>
@OK PUTE <size> <crc16>
@BEGIN GET <size>
@DATA <hex>
@END GET <size> <crc16>
@OK RUN
@OK APP IDLE FORMAT=0 EXIT=0
@OK STOP
@OK DEL
```

Errors use:

```text
@ERR <reason>
```

## Verified transfer model

Upload:

```text
PUTB -> zero/create destination
PUTD -> append decoded hex payload
PUTE -> re-read and verify size + CRC16/CCITT
```

Download:

```text
@BEGIN GET <size>
@DATA <hex>
...
@END GET <size> <crc16>
```

TEST-09 verified an exact PC -> KonSol -> microSD -> KonSol -> PC round-trip
with matching SHA-256.

## AVR Serial constraint

The classic ATmega328P HardwareSerial RX buffer is 64 bytes. TEST-09 found that
24-byte PUTD payloads could make a complete command line too long under real
scheduler load.

The certified host tools therefore use **20 data bytes per PUTD record** and LF
line termination so a complete PUTD command stays below the receive-buffer
limit.

## FAT filename constraint

The Arduino SD/FAT path used by this LAB is treated as FAT 8.3 for installed
application names. Host Manager validates destination names before transfer.

Example:

```text
/HOSTGUI.KAP
```

## Runtime states

Physical TEST-10 observations:

```text
resident shell: APP=0 RAM=652 TASKS=5
running KAP:    APP=1 RAM=621 TASKS=5
normal EXIT:    APP EXIT 0 -> APP=0 RAM=652
remote STOP:    APP EXIT 254 -> @OK STOP -> APP=0 RAM=652
```

HOST1 is therefore verified not only for file transfer but for remote
application lifecycle control while the resident scheduler continues to run.

# KonSol Host Manager

Desktop manager for KonSol 0.7 HOST1.

The manager is intentionally host-side so that TEST-10 adds practical usability
without consuming more ATmega328P Flash or SRAM.

## Requirements

- Windows 10/11
- Python 3
- Tkinter from the standard Python Windows distribution
- pyserial

Install pyserial:

```powershell
python -m pip install pyserial
```

## Start

From the repository root:

```powershell
python .\tools\konsol-host-manager\konsol_host_manager.py
```

## Current TEST-10 functions

- enumerate serial ports;
- connect/disconnect to KonSol HOST1;
- verify HOST1 with @PING;
- show INFO / RAM / task status;
- browse the KonSol microSD;
- install a local .KAP application;
- safe HOST1 PUTD chunking under the UNO RX-buffer limit;
- verify upload with size + CRC16;
- download files with CRC verification;
- show SHA-256 for uploaded/downloaded files;
- run selected .KAP applications;
- stop a running application;
- delete files from the KonSol microSD;
- invoke the existing KASM assembler to build .kasm -> .KAP.

## TEST-10 direction

TEST-09 proved the wire protocol. TEST-10 turns that protocol into a normal
operator interface.

The target workflow is:

```text
PC
 |
 +-- KonSol Host Manager
       |
       +-- CONNECT
       +-- STATUS
       +-- FILES
       +-- INSTALL KAP
       +-- DOWNLOAD
       +-- RUN / STOP
       +-- DELETE
       +-- BUILD KASM -> KAP
              |
              v
            HOST1
              |
              v
           KonSol 0.7
```

Status: **TEST-10 FULL PHYSICAL PASS**.

Verified on physical KonSol 0.7 hardware:

- COM4 connection and HOST1 PING;
- parsed INFO / RAM / TASKS state;
- microSD browsing;
- GUI Install / Download / Run / Stop / Delete;
- exact GUI round-trip with matching SHA-256;
- KASM -> KAP build from the GUI;
- execution of the GUI-built application on TFT/Touch;
- clean APP EXIT 0 and remote APP EXIT 254 via Stop;
- RAM recovery to 652 B after application termination;
- FAT 8.3 destination-name validation;
- safe short PUTD records below the UNO HardwareSerial RX-buffer limit.

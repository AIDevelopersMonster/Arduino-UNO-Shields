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

Status: **IMPLEMENTED / PHYSICAL TEST PENDING**.

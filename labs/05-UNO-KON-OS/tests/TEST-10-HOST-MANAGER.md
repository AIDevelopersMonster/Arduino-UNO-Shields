# LAB-05 / TEST-10 — KonSol Host Manager

Status: **IMPLEMENTED / PHYSICAL TEST PENDING**.

## Purpose

TEST-09 physically proved the HOST1 machine protocol. TEST-10 moves the next
layer to the PC: a normal desktop manager for the assembled KonSol device.

This is deliberately host-side work. KonSol 0.7 already occupies about 94% of
ATmega328P Flash, so TEST-10 should improve usability without spending the last
UNO firmware headroom.

## Program

`tools/konsol-host-manager/konsol_host_manager.py`

## Start

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull
python -m pip install pyserial
python .\tools\konsol-host-manager\konsol_host_manager.py
```

## What TEST-10 must verify

1. COM port discovery.
2. Connect to KonSol 0.7 and receive HOST1 PONG.
3. INFO line is parsed into version / HOST / SD / APP / RAM / TASKS.
4. SD root appears as a desktop file list.
5. `Install KAP` transfers a local application and verifies size + CRC.
6. `Download` retrieves the selected file and verifies CRC.
7. `Run` starts a selected .KAP application on KonSol.
8. TFT/Touch application still works while the manager remains connected.
9. `APP`, status and asynchronous Serial output remain visible in the log.
10. `Stop` can terminate a running application.
11. `Delete` removes a selected file from microSD.
12. `Build KASM -> KAP` invokes the existing repository KASM assembler.

## First physical test

Keep Serial Monitor closed because the manager needs exclusive access to COM4.

Start:

```powershell
python .\tools\konsol-host-manager\konsol_host_manager.py
```

Then in the GUI:

```text
Refresh ports
-> select COM4
-> Connect
-> Refresh status
-> Refresh files
```

Expected top status:

```text
KonSol: 0.7
HOST: 1
SD: READY
APP: IDLE
RAM: about 652 B
TASKS: 5
```

Expected file table: the same microSD root previously observed through
`@LS /`.

## Why this matters

TEST-09 proved that another computer *can* manage KonSol through HOST1.

TEST-10 checks the stronger practical result: an operator can manage the device
without manually typing protocol records.

The intended transition is:

```text
TEST-09
raw HOST1 protocol

        ->

TEST-10
desktop KonSol Host Manager
```


## Physical evidence — GUI install and run

Observed on the physical KonSol 0.7 device through the TEST-10 Host Manager:

- GUI connected to COM4 successfully;
- parsed status: KonSol 0.7 / HOST1 / SD READY / TASKS 5;
- SD root listing displayed in the desktop table;
- new application `HOSTMGR.KAP` appeared on microSD after GUI installation;
- observed Windows-side installed size: 454 B;
- GUI launched `/HOSTMGR.KAP` with `@RUN`;
- while the application was running, `@INFO` reported `APP=1` and `RAM=621`;
- application Serial output was visible in the manager log:
  - `TEST10 HOST MANAGER`
  - `HOST MANAGER APP PASS`
- TFT displayed `INSTALL PASS`;
- resident Touch service returned real coordinates:
  - X = 69
  - Y = 156
- TFT displayed `TOUCH TO EXIT`.

This closes the TEST-10 gates for GUI connection, SD browsing, GUI-side KAP
installation, remote RUN, live APP state, asynchronous application Serial output
and TFT/Touch execution.

Next gate: complete APP EXIT, verify APP returns IDLE, then test GUI Download and
byte-integrity verification.


## Physical evidence — clean APP exit and resident recovery

Observed on physical hardware after GUI launch of `/HOSTMGR.KAP`:

```text
@OK INFO V=0.7 HOST=1 SD=1 APP=1 RAM=621 TASKS=5
TEST10 HOST MANAGER
HOST MANAGER APP PASS
APP EXIT 0
@OK INFO V=0.7 HOST=1 SD=1 APP=0 RAM=652 TASKS=5
```

This verifies clean return from the external KAP2 application to resident KonSol:
APP returns to IDLE, RAM returns from 621 B to the normal 652 B shell value, and
all five cooperative tasks remain present.


## Physical evidence — GUI download

The TEST-10 Host Manager downloaded the GUI-installed application back from the
physical KonSol microSD.

Observed:

```text
DOWNLOAD PASS:
C:\GitHub\Arduino-UNO-Shields\labs\05-UNO-KON-OS\apps\KAP2\HOSTMGR.downloaded.KAP
size=454
CRC=6C3D
SHA256=D44507C76C80C80584DFB58593F9A6765467B4A8575860DFE5D2114EF52AFE5A
```

This closes the GUI-side GET + CRC verification gate. Final byte-for-byte
round-trip certification requires comparing this SHA-256 with the local source
`HOSTMGR.KAP`.


## Physical evidence — exact GUI round-trip

The file downloaded by the TEST-10 Host Manager was compared against the local
source file with SHA-256.

Observed:

```text
SRC SHA256 = D44507C76C80C80584DFB58593F9A6765467B4A8575860DFE5D2114EF52AFE5A
RX  SHA256 = D44507C76C80C80584DFB58593F9A6765467B4A8575860DFE5D2114EF52AFE5A
MATCH      = True
```

This certifies an exact byte-for-byte GUI round-trip:

```text
PC source
 -> KonSol Host Manager
 -> HOST1
 -> microSD
 -> HOST1
 -> KonSol Host Manager
 -> PC download
```

The GUI install/download path is therefore physically verified independently of
the TEST-09 PowerShell transfer script.

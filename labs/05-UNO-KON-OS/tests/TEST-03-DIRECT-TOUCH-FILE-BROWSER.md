# LAB-05 / TEST-03 — KonSol 0.3 Direct Touch + File Browser

## Purpose

Make KonSol usable from the Arduino UNO + MAR2406 itself, without requiring
the PC Serial terminal for ordinary file browsing.

TEST-03 adds a direct resistive-touch service and an on-screen microSD file
browser to the physically verified KonSol 0.2 architecture.

No MCUFRIEND_kbv, Adafruit_GFX or TouchScreen library is used by this firmware.

## Firmware

`sketches/03_KonSol_Touch_File_Browser/03_KonSol_Touch_File_Browser.ino`

## Architecture

```text
+-----------------------------------+
| TFT dashboard / file browser      |
| text viewer                       |
+-----------------------------------+
| Serial shell + filesystem service |
+-----------------------------------+
| cooperative kernel                |
| SERIAL CLOCK DISPLAY TOUCH        |
+-----------------------------------+
| direct ILI9341 | direct Touch | SD|
+-----------------------------------+
| ATmega328P / 2 KB SRAM            |
+-----------------------------------+
```

Tasks:

```text
ID  TASK      PERIOD
0   SERIAL    1 ms
1   CLOCK     100 ms
2   DISPLAY   1000 ms
3   TOUCH     30 ms
```

## Touch

Verified MAR2406 wiring:

```text
XP = D6
XM = A2
YP = A1
YM = D7
```

Certified calibration:

```text
LEFT   153
RIGHT  930
TOP    962
BOTTOM 168
```

The firmware implements touch measurement directly and restores every shared
pin to the LCD bus before any graphics operation.

## UI

Dashboard:

```text
KONSOL 0.3
TOUCH SYSTEM + FILES

KERNEL   RUN
SD       READY
RAM      ...
UPTIME   ...
TOUCH    READY

[FILES]
```

Touch FILES to enter the browser.

Browser controls:

- file/directory rows — open item;
- DASH — return to system dashboard;
- PREV / NEXT — page through the directory;
- UP — parent directory.

Directories open in the browser. Files open in a one-page ASCII text viewer.
Touch BACK to return to the browser.

## New shell commands

```text
FILES       open touch file browser
BROWSER     alias for FILES
HOME        return to dashboard
TOUCH       show last touch state / coordinates / pressure
```

All KonSol 0.2 filesystem and diagnostic commands remain available.

## Build

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git pull

arduino-cli compile --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\03_KonSol_Touch_File_Browser
```

Record Flash and SRAM before upload.

## Upload

```powershell
arduino-cli upload -p COM4 --fqbn arduino:avr:uno .\labs\05-UNO-KON-OS\sketches\03_KonSol_Touch_File_Browser
```

## Serial verification

```text
INFO
MEM
PS
DIR /
TOUCH
FILES
```

Expected PS includes the fourth TOUCH task.

## Physical verification

1. Dashboard is readable.
2. Touch FILES.
3. Browser lists the real SD root.
4. Touch a text file such as XOLOG.TXT.
5. Viewer displays file contents.
6. Touch BACK.
7. PREV/NEXT work when enough files exist.
8. Directory entry opens and UP returns to parent.
9. DASH returns to system dashboard.
10. Serial DIR / still works after repeated touch browsing.
11. MEM remains stable.
12. No reset, display corruption or filesystem corruption occurs.

## PASS criteria

TEST-03 passes only after the direct Touch service, browser, viewer, Serial
shell and microSD filesystem are all observed working together on the physical
Arduino UNO.

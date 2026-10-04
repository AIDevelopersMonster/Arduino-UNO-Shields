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

Verified build:

```text
Sketch: 23666 / 32256 bytes Flash (73%)
Globals: 1155 / 2048 bytes SRAM (56%)
Linker-reported SRAM remaining: 893 bytes
```

Build status: **PASS**.

Compared with KonSol 0.2:

```text
KonSol 0.2: 20264 B Flash / 1059 B globals
KonSol 0.3: 23666 B Flash / 1155 B globals

Delta: +3402 B Flash
       +96 B global SRAM
```

Compared with KonSol 0.1:

```text
Delta: +6736 B Flash
       +122 B global SRAM
```

The direct Touch service, browser state, path buffer and file-viewer UI still
fit with 893 bytes of linker-reported SRAM remaining. Runtime free RAM must be
measured on hardware before TEST-03 can be certified.

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


## Physical bench result — 2026-10-04

Serial verification:

```text
BOOT: TFT init
BOOT: kernel init
BOOT: SD mount
SD: READY
FREE RAM: 885 B

INFO
TASKS: 4
TFT: ILI9341 direct 8-bit
TOUCH: direct resistive
SD: READY
FREE RAM: 823 B

PS
0   SERIAL    1 ms
1   CLOCK     100 ms
2   DISPLAY   1000 ms
3   TOUCH     30 ms

DIR /
F 175 T07LOG.TXT
F 23 XOLOG.TXT
FILES: 2
```

The watchdog REBOOT command also returned the system to a clean KonSol 0.3 boot.

Physical TFT/Touch observations:

- dashboard renders correctly and reports KERNEL RUN, SD READY and TOUCH READY;
- FILES opens the touch file browser;
- the SD root is shown with T07LOG.TXT and XOLOG.TXT;
- touching XOLOG.TXT opens the VIEW screen for that file;
- returning to the dashboard works;
- the direct touch service and direct ILI9341 driver coexist with the SD filesystem on the physical UNO.

Observed dashboard free-RAM display was approximately 855 B during the photographed run.
This value is sampled from a different call depth than the Serial INFO/MEM value and is
therefore not treated as directly interchangeable with the 823 B shell measurement.

### Current certification state

**CORE PHYSICAL PASS** for dashboard, direct Touch, file browser, file opening/viewer,
Serial shell, SD filesystem and watchdog reboot.

The remaining extended navigation cases (multi-page PREV/NEXT and directory-enter/UP)
require a directory with more than five entries and at least one subdirectory. They are
kept as explicit final checks rather than inferred from the current two-file SD card.

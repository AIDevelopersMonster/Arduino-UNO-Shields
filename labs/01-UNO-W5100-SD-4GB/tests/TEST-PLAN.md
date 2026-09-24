# LAB-01 Bring-up Test Plan

## A1 — Arduino UNO smoke test

Pass criteria:

- sketch uploads without error;
- built-in LED changes state;
- Serial Monitor shows the LAB-01 banner.

## A2 — W5100 Ethernet

Preparation:

- Ethernet cable connected;
- SD disabled by driving D4 HIGH.

Pass criteria:

- Ethernet controller is detected by the library;
- physical link is reported when supported;
- DHCP succeeds;
- local IP, gateway, subnet mask and DNS are printed.

## A3 — SD 4 GB

Preparation:

- W5100 disabled by driving D10 HIGH.

Pass criteria:

- `SD.begin(4)` succeeds;
- root directory can be read;
- `LAB01.TXT` is created/appended;
- the file can be read back.

## A4 — Ethernet + SD together

Pass criteria:

- Ethernet DHCP succeeds;
- SD initialization succeeds;
- both results are printed in the same run;
- `LAB01.TXT` remains readable.

## Failure rule

If a stage fails, stop there and record the exact Serial Monitor output.
Do not compensate with unrelated code changes before the failed hardware path is isolated.

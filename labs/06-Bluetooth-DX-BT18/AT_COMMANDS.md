# DX-BT18 / BT18-LE: AT command registry and access levels

This is a **documentation-based reference**, not a certified command list for the unmarked purchased module. Default serial settings in the original manual: 9600 baud, 8N1, CRLF, uppercase commands. Original dual-mode requires module P26 (GPIO25) LOW to enter AT mode; BT18-LE differs.

## Dual-mode DX-BT18

| Function | Documented read/test | Documented write/action | First-pass probe |
|---|---|---|---|
| AT test | `AT+TEST` | — | YES |
| Firmware version | `AT+VERS?` | — | YES |
| Bluetooth address | `AT+ADSS?` | `AT+ADSS=<MAC>` | YES |
| Device name | `AT+NAME?` | `AT+NAME=<name>` | YES |
| UART speed | `AT+BAUD?` | `AT+BAUD=<1..5>` | YES |
| SPP PIN | `AT+PIN?` | `AT+PIN=<pin>` | YES |
| BLE UUID trio | `AT+UUID?` | Individual setters below | YES |
| Service UUID | via `AT+UUID?` | `AT+SERV=<uuid>` | NO write |
| Notify UUID | via `AT+UUID?` | `AT+NOTIFY=<uuid>` | NO write |
| Write UUID | via `AT+UUID?` | Manual prints `AT+WRITE?=...` (suspicious) | NO write |
| Flow control | `AT+FLOW?` | `AT+FLOW=0/1` | YES |
| Disconnect link | — | `AT+DIS?` (has effect!) | NO |

The manual maps `AT+BAUD` values: 1=9600, 2=19200, 3=38400, 4=57600, 5=115200. A power cycle is specified after changing baud rate. Factory advertised BLE UUIDs: service FFE0, notify FFE1, write FFE2. The manual shows swapped byte order in configuration examples (`e0ff` etc.). Do not assume a cloned firmware uses the same convention.

**Documentation caveat:** `AT+WRITE?=...` looks like a typo in the manual. Do not "fix" by guessing and issuing live writes.

## BT18-LE (BLE-only) alternative

| Function | Read/test | Write (not part of the first test) |
|---|---|---|
| AT test | `AT` | — |
| Version | `AT+VER` | — |
| Address | `AT+ADDR` | — |
| Name | `AT+NAME` | `AT+NAME=<name>` |
| Baud | `AT+BAUD` | `AT+BAUD=<1..5>` |
| Service UUID | `AT+SERV` | `AT+SERV=<uuid>` |
| Notify UUID | `AT+NOTI` | `AT+NOTI=<uuid>` |
| Write UUID | `AT+WRIT` | `AT+WRIT=<uuid>` |
| Delay/channel wait | `AT+WAIT` | `AT+WAIT=<value>` |

No SPP is specified for BT18-LE. A similarly named clone may have different parsing rules; automatic ID is **heuristic**.

## Physical "registers", logical settings, GATT

1. **AT fields** (name, PIN, MAC, baud, BLE UUID) are logical configuration, not arbitrary memory-mapped registers. Queries above can inspect them where firmware supports those commands.
2. **BLE GATT services, characteristics, descriptors** are protocol objects. Reading/writing a characteristic is only permitted according to its advertised properties; it is not raw SoC RAM/FLASH access.
3. **Internal SoC registers / firmware storage**: no arbitrary address READ/WRITE in the published DX-BT18 AT commands.
4. **ICE / VPP** exist on the *original* documented 33-contact radio module (P8/P7), but a reproducible, verified open firmware flashing route for the unidentified sample has not been established. Do not assert firmware rewrite is available based on pin names.
5. A YC1021-based third-party adapter EEPROM map, if found, is specific to that adapter; do not write those offsets to our module.

## Decision rule before any modification

Only after identity is supported by at least independent physical marking/pinout or a matching, documented firmware+hardware fingerprint should we consider configuration writes. Firmware rewriting additionally requires confirmed chip and programming method, an exact matching image, protection/OTP understanding, a recovery plan and a tested readback/backup route. Otherwise stop at read-only identification.

## Primary reference

- [Shenzhen DX-SMART DX-BT18 & BT18-LE technical manual](https://manuals.plus/m/b2042a61df3340ba1f528646b16efdf3d786b18f2edb66204799996c10bea264)
- [DX-BT18 FCC filing](https://fccid.io/2AKS8DX-BT18)

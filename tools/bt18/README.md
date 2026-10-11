# BT18 read-only host tools

The tools are deliberately generic: USB-UART + Windows PC and, separately, PC Bluetooth LE adapter. Arduino UNO is **not** needed for the first AT/GATT identification steps.

## Install

From repository root (`C:\\GitHub\\Arduino-UNO-Shields`):

```powershell
py -m venv .venv
.\\.venv\\Scripts\\python.exe -m pip install -r .\\tools\\bt18\\requirements.txt
```

## UART AT

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --list-ports
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --port COM4 --baud 9600 --profile dual
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --port COM4 --baud 9600 --profile le
```

Optional JSON local log:

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --port COM4 --profile both --json .\\labs\\06-Bluetooth-DX-BT18\\results\\uart.json
```

`--ending` accepts `crlf`, `cr`, `lf`. `--timeout` is seconds **per command**, default 0.65. `--profile dual` is default. Both profiles send only documented test/read commands and no `AT+DIS?` (which disconnects a session despite the question mark).

A silent module could simply need its verified AT-mode pin asserted. Do not guess P26/KEY on an unknown adapter; read the lab wiring cautions.

## BLE discovery

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_ble_scan.py --duration 12
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_ble_scan.py --duration 12 --name BT18
```

Optional GATT services and characteristics (actively connects, no characteristic writes):

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_ble_scan.py --address "BLE_ID_FOUND_ABOVE"
```

Windows may report different BLE identifiers from the Classic Bluetooth address. A BLE scan cannot identify a Classic-only device and cannot prove SPP.

## Test boundaries

- No firmware images, ICE/VPP programming, arbitrary memory access or GATT writes.
- AT/GATT fingerprint is **not** a certificate of DX-SMART authenticity.
- Logs can contain MACs or identifiers of nearby devices. The lab `results/` folder is ignored by Git.
- Hardware result files are created only after observing a real sample.

See [LAB-06](../../labs/06-Bluetooth-DX-BT18/README.md).

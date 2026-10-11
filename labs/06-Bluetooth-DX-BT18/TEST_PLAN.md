# LAB-06 — TEST plan and acceptance criteria

**Current evidence status: ALL PENDING.** No commands have been tested on the purchased unmarked module yet. Test commands do not imply a certified PASS.

| ID | Procedure | PASS condition | Key non-claim |
|---|---|---|---|
| TEST-01 UART / AT | Identify USB-UART port; 9600 8N1; try dual and LE **read-only** profiles if needed | Stable, intelligible repeated responses with raw console evidence | AT syntax similarity does not prove maker or chip |
| TEST-02 BLE | Scan unfiltered advertisements; note name/identifier; optional GATT enumeration | Reproducible observed advertisements and/or successful services enumeration | BLE does not prove Classic SPP |
| TEST-03 Classic/SPP | Pair where supported; inspect OS-created serial port; bidirectional bytes | Exact TX/RX on actual UART and SPP, repeatable | No evidence of SPP merely from device name |
| TEST-04 Identity | Compare both sides of PCB, radio shield, chip markings, AT response and GATT characteristics | Evidence-based family attribution, list alternatives and uncertainty | "DX-BT18" shop title is not part number evidence |
| TEST-05 Firmware feasibility | Check confirmed SoC datasheet, memory organization, programmer, image provenance, OTP protection | Documentation-based decision; **no live writes** | ICE marking alone is not firmware compatibility |

## TEST-01 command sequence (PowerShell)

```powershell
Set-Location C:\\GitHub\\Arduino-UNO-Shields
git pull --ff-only
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --list-ports
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --port COM4 --baud 9600 --profile dual --json .\\labs\\06-Bluetooth-DX-BT18\\results\\test-01-dual.json
```

If the original dual-mode AT switch pin is independently identified and accessible, P26/GPIO25 LOW is described for the original module; **do not** short unverified contacts on a clone. If no response and an LE-only variant is plausible:

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_probe.py --port COM4 --baud 9600 --profile le
```

If settings were previously changed, choose a different **known** baud. Avoid brute-force commands that change configuration.

## TEST-02 command sequence

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_ble_scan.py --duration 12
```

Copy the actual BLE address/OS identifier from the output only if the user wants GATT enumeration:

```powershell
.\\.venv\\Scripts\\python.exe .\\tools\\bt18\\bt18_ble_scan.py --address "ACTUAL_DEVICE_IDENTIFIER"
```

## Evidence standard

Use [RESULT_TEMPLATE](RESULT_TEMPLATE.md) for all new result files. Record the date, known hardware, connection diagram, command transcript, detected versions, pass criteria, uncertainty and exact non-claims. Local UART / BLE logs may expose MAC addresses; `results/` is intentionally ignored by Git.

After two or three non-destructive functional attempts fail, record a bounded result and move to another device/task rather than starting a repair investigation.

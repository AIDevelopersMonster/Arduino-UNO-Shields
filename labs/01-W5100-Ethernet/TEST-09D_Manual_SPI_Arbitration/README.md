# TEST-09D — Manual SPI Arbitration (final experiment)

**Status: source published; compile and bench PASS not yet verified.**

This is an operator-controlled, **sequential** SD-to-Ethernet experiment on Arduino UNO + blue W5100 shield. No automatic SD/Ethernet switching. All initial device selections are inactive (**CS D10 HIGH, CS D4 HIGH**). The human operator types individual commands in the serial monitor and receives a progress report after each step.

## Commands

| Command | Action |
| --- | --- |
| `H` | Show help and suggested command sequence |
| `?` | Print current software states and static CS-pin levels |
| `S1` | Initialize SD, delete/create `K09D.TXT`, write text, close, reopen and byte-verify |
| `S0` | SD phase ended: D4 HIGH, no open file or SD operations |
| `L1` | Initialize W5100 DHCP, bind UDP Echo port 5003, print obtained IP |
| `L0` | Stop UDP Echo and set D10 HIGH |
 
The firmware blocks `L1` if the SD phase has not been closed by `S0` and blocks `S1` while LAN is on. **OFF means deselected by CS, not electrically disconnected or depowered.** A passed test of sequential software access does **not** establish that MISO is high-impedance when a slave is deselected.

## How to perform

Power OFF UNO before insertion/removal of microSD. Use the microSD that passed TEST-09C. Connect RJ45; do not modify router/DHCP.

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git restore --source=origin/main --worktree -- labs/01-W5100-Ethernet/TEST-09D_Manual_SPI_Arbitration/
$sketch = '.\labs\01-W5100-Ethernet\TEST-09D_Manual_SPI_Arbitration'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Use Serial Monitor input to issue commands **one per line** (press Enter). Set line ending to newline or CRLF when using an IDE serial monitor. Some terminal/CLI setups do not forward console input: if command responses are absent, try Arduino IDE 1.8.19 Serial Monitor at 115200 or a serial terminal that supports two-way COM communication; do not run a second COM-opening application concurrently.

Expected operator transcript:

```text
KON TEST-09D MANUAL SPI / WAIT FOR COMMAND
STATE SD=OFF LAN=OFF D4=HIGH D10=HIGH
...
S1
S1 STEP 1: SD CONNECTED PASS
S1 STEP 2: WRITE PASS / FILE CLOSED
S1 STEP 3: READ VERIFY PASS / FILE CLOSED
S1 COMPLETE: SD PASS. NEXT COMMAND S0
S0
S0: SD DESELECTED / D4 HIGH / NO SD OPERATIONS
L1
L1 PASS: LAN CONNECTED / UDP ECHO READY
IP: 192.168.1.xxx
UDP PORT: 5003
L0
L0: UDP STOPPED / LAN DESELECTED / D10 HIGH
```

These are expected strings, **not observed bench results**. To prove actual network data transfer after `L1`, use PowerShell from a **second terminal** (leave serial monitor open) and send a UDP payload, e.g.:

```powershell
$ip='192.168.1.73' # REPLACE with actual IP printed after L1
$udp=[System.Net.Sockets.UdpClient]::new(0)
$udp.Client.ReceiveTimeout=3000
try {
  $payload=[System.Text.Encoding]::ASCII.GetBytes('KON-LAN-09D')
  [void]$udp.Send($payload,$payload.Length,$ip,5003)
  $from=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any,0)
  $echo=$udp.Receive([ref]$from)
  [System.Text.Encoding]::ASCII.GetString($echo)
} finally { $udp.Dispose() }
```

The same bytes should be echoed and Serial Monitor should show `UDP ECHO PASS #1`. Finally issue `L0`. Opening a COM monitor can reset an UNO via DTR: start the monitor **before** issuing commands and leave it open throughout.

## Important scope and data warning

`S1` **deletes** any pre-existing `K09D.TXT` and overwrites it. Back it up if valuable. No other files are intentionally modified. The board only has 2KB SRAM, so compile size/memory must be confirmed before upload.

If `S1` fails, use `S0`, report the exact last message, and do not assume electrical damage. If `L1` fails after `S0`, report DHCP errors. This is intentionally the last bounded experiment of this subprogram, not an invitation to indefinite hardware repair.

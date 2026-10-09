# W5100 Ethernet Shield — TEST-01 SPI Probe

Hardware: Arduino UNO + WIZnet W5100 shield (blue HanRun sample). No microSD card and no Ethernet cable required.

This test directly reads and writes W5100 registers via SPI, without the Ethernet library or DHCP. W5100 CS = D10; microSD CS = D4 (held HIGH); SPI = ICSP / D11-D13 on UNO. Serial = 115200.

The sketch reads MR and RTR, temporarily writes RTR=0x1234, checks readback, and restores original RTR. PASS confirms register read/write, **not** Ethernet PHY, link, or network connectivity.

From repository root (PowerShell):

```powershell
git pull --ff-only
$sketch = ".\labs\01-W5100-Ethernet\TEST-01_SPI_Probe"
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

If serial output is missing after opening the monitor, press RESET once. Change COM4 if necessary. Record full terminal output before interpreting hardware status.

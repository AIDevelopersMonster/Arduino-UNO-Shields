# TEST-07B — No SD Code: physical card-presence A/B test

**PENDING actual result.** This variant preserves the TEST-07 UDP Echo protocol (DHCP, UDP port 5002, identical MAC and payload validation) but removes all application-level handling of the microSD chip-select pin.

- No `SD.h`, `SD.begin()`, `SD.open()`, or card-detect routine.
- No `SD_CS` variable, no `pinMode(4,...)`, no `digitalWrite(4,...)`.
- **SPI.h and Ethernet.h remain necessary to operate the W5100.**
- The original [TEST-07](../TEST-07_Cable_Recovery/TEST-07_Cable_Recovery.ino) is unchanged.
- This variant **does not electrically disconnect the SD slot**: D4 may be left in its reset/default input state, and hardware pull-ups or card circuitry may still interact with SPI. The absence of SD software is not the same as an electrically deselected card.

## Protocol

Power OFF the UNO before inserting/removing the microSD. Keep RJ45 plugged in. Perform just a basic start-up/DHCP test with the card installed. Do not run cable removal phases if DHCP fails.

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git restore --source=origin/main --worktree -- labs/01-W5100-Ethernet/TEST-07_Network_Robustness/TEST-07B_No_SD_Code/
$sketch = '.\labs\01-W5100-Ethernet\TEST-07_Network_Robustness\TEST-07B_No_SD_Code'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

If DHCP and UDP start, the prior TEST-07 PowerShell script can run unchanged against the current IP and UDP 5002. If DHCP fails, stop and record the serial output. Do not modify router or its DHCP settings.

**Interpretation:** If this variant changes behavior, the D4 configuration is implicated, but it does not alone prove a mechanical Card Detect signal or establish root cause. If it still fails, an electrical presence effect remains plausible; no conclusion of defective card/board without additional independent evidence.

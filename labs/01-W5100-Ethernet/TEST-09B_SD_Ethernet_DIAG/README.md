# TEST-09B Boot Diagnostic — Ethernet + microSD

**Diagnostic firmware, awaiting bench validation.** Based on TEST-09B. Preserves W5100 D10, SD D4, UDP 5003, MAC `02:4B:4F:4E:59:0B`, the 24-byte Echo and SD append/verify protocol.

It prints one startup line containing `sd=PASS/FAIL`, `eth=PASS/FAIL`, W5100 hardware-status enumeration, actual DHCP `ip=`, and UDP port. It then stops communicating over UART throughout the packet-processing period. On a host `K9END` request it emits the final line again.

**IMPORTANT:** Opening COM4 on Arduino UNO generally resets the AVR (DTR); the boot line should be captured at startup. Close Serial Monitor before host traffic. No COM transmission occurs during the test itself. The original TEST-09B remains untouched.

## Commands
```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git restore --source=origin/main --worktree -- labs/01-W5100-Ethernet/TEST-09B_SD_Ethernet_DIAG/
$sketch = '.\labs\01-W5100-Ethernet\TEST-09B_SD_Ethernet_DIAG'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Use the actual `ip=` from startup. If `sd=FAIL` or `eth=FAIL`, do not start the client yet: submit the startup line. If both PASS, close monitor and run the existing `Test-IntegratedSilent.ps1 -IP <real ip> -Count 1`.

For single-packet PASS, require `K9RESULT eth=1 sd=1 rx=1 echo=1 wr=1 rd=1 err=0 bytes=24` and exact Echo. The test appends to `K9BTEST.BIN` and does not clear earlier content.

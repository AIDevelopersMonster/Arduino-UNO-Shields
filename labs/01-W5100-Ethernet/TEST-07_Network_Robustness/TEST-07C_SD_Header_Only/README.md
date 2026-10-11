# TEST-07C — SD.h Included, microSD Inactive

**Status: PENDING hardware validation.** This is a controlled comparison with TEST-07 and TEST-07B.

* Includes `SPI.h`, `Ethernet.h`, `EthernetUdp.h`, and **`SD.h`**.
* Keeps Ethernet CS **D10** and sets SD CS **D4 = OUTPUT HIGH** to deselect microSD on the shared SPI bus.
* Deliberately does **not** call `SD.begin()`, `SD.open()`, `SD.read()`, `SD.write()`, or card detect.
* Otherwise retains the original TEST-07 DHCP + UDP Echo implementation (UDP port **5002**).
* A header include alone does not guarantee that the SD library's implementation is linked into the firmware; code that is unused may be omitted by the toolchain. This test isolates **header inclusion**, not active SD operation.

## Procedure

Power down the board (USB and any other power) before inserting a microSD. Test one card at a time with the same firmware and cabling. There is no need to unplug the RJ45 or run the four TEST-07 phases; record whether DHCP prints a valid IP and `READY`. If DHCP fails, stop and record the UART output.

The user's current checkout may be on `feature/w5100-test09-ethernet-sd` with unrelated local changes. Import only this new directory from `origin/main`:

```powershell
cd C:\GitHub\Arduino-UNO-Shields
git fetch origin
git restore --source=origin/main --worktree -- labs/01-W5100-Ethernet/TEST-07_Network_Robustness/TEST-07C_SD_Header_Only/
$sketch = '.\labs\01-W5100-Ethernet\TEST-07_Network_Robustness\TEST-07C_SD_Header_Only'
arduino-cli compile --fqbn arduino:avr:uno $sketch
arduino-cli upload -p COM4 --fqbn arduino:avr:uno $sketch
arduino-cli monitor -p COM4 -c baudrate=115200
```

Log both card identities and whether Windows mounts them separately. A card mounting in a PC reader does not certify compatibility with this 5V Arduino shield's SD-level shifting and shared SPI wiring.

Do not alter the router or DHCP configuration. Keep TEST-07/TEST-07B unchanged as baselines.

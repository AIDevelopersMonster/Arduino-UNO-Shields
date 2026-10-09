# Shield 01 — W5100 Ethernet + SD

First shield in the **Arduino UNO & Shields** project.

## Hardware

- WIZnet W5100 Ethernet controller
- RJ45 Ethernet connector
- SD card socket/interface
- Arduino UNO shield form factor

## Test sequence

1. Visual identification and board revision notes
2. Arduino UNO + shield assembly
3. SPI and chip-select pin verification
4. W5100 Ethernet initialization
5. DHCP / static IP test
6. TCP/UDP communication tests
7. Web server example
8. SD card initialization and read/write test
9. Ethernet + SD simultaneous operation
10. Practical network projects

## Initial hardware set

- Arduino UNO compatible board
- W5100 Ethernet Shield
- 4 GB SD card

Pin mapping and verified examples will be added after bench tests.

## Verified blue W5100 sample (separate no-SD test track)

The [blue W5100 Ethernet laboratory](../../labs/01-W5100-Ethernet/README.md)
currently certifies SPI (TEST-01), DHCP/Ping (TEST-02) and HTTP serving
(TEST-03). TEST-04 browser-controlled D6/D7 is published but **not yet bench
certified**. No result of these tests is evidence that the microSD slot works.
The older [LAB-01 W5100 + SD 4GB](../../labs/01-UNO-W5100-SD-4GB/README.md)
remains a separate hardware track. [Full test roadmap and CLI](../../labs/01-W5100-Ethernet/README.md).

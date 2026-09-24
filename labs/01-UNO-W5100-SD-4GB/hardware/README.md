# LAB-01 Hardware

## Assembly

1. Disconnect USB power.
2. Seat the W5100 Ethernet Shield fully on the Arduino UNO headers.
3. Insert the 4 GB SD card.
4. Connect RJ45 Ethernet to a router/switch or other known-good Ethernet network.
5. Connect the Arduino UNO to the PC by USB.
6. Select the correct Arduino UNO board and COM port in Arduino IDE.

## Important SPI resources

- D4 — SD chip select
- D10 — W5100 chip select
- D11 — SPI MOSI
- D12 — SPI MISO
- D13 — SPI SCK

D4 and D10 are therefore reserved by LAB-01.

## SD card preparation

For the first test, use a simple FAT-formatted card with no important data.
The laboratory writes a small file named `LAB01.TXT`.

Record the exact card brand/model and formatting details later in the results.

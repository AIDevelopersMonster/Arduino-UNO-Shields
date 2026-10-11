# LAB-01 — Arduino UNO Clone + W5100 + SD 4 GB

> **Status: CLOSED / HISTORICAL HARDWARE TRACK.** The original shield sample was deferred as suspect hardware. The later blue W5100 track established useful Ethernet-only results, while reliable combined onboard microSD + W5100 operation on tested clone hardware was ultimately rejected. No further repair or shared-SPI development is planned.

## Purpose

This is the first complete laboratory stand in the **Arduino UNO & Shields** project.

Hardware:

- Arduino UNO compatible board
- W5100 Ethernet Shield
- 4 GB SD card
- USB cable
- RJ45 Ethernet cable

The laboratory is intentionally a **series of examples**, not one sketch.

This README preserves the original staged plan. The final project decision supersedes that plan where the tested hardware failed or was later scoped to Ethernet-only use.

## Pin map used by this laboratory

| Function | Arduino UNO pin |
| --- | ---: |
| SD chip select | D4 |
| Ethernet W5100 chip select | D10 |
| SPI MOSI | D11 |
| SPI MISO | D12 |
| SPI SCK | D13 |

Ethernet and SD share the SPI bus. Only one chip-select must be active at a time.

## Stage A — bring-up

Run the sketches in this order:

1. `01_UNO_Smoke` — prove that the UNO uploads, runs and prints to Serial.
2. `02_W5100_DHCP` — disable SD and prove that W5100 gets a network address.
3. `03_SD_Card` — disable W5100 and prove that the 4 GB card can be written and read.
4. `04_Ethernet_SD_Combined` — initialize and use Ethernet and SD in one sketch.

Historical gate: the original plan required all four stages to pass before practical projects. That condition was not reached on the original sample, and the track is now closed.

## Expected result

At the end of Stage A we should have:

- stable USB programming;
- Serial Monitor output at 115200 baud;
- Ethernet link;
- DHCP address from the router/network;
- successful SD initialization;
- a test file written and read from the card;
- Ethernet and SD both initialized in the same program.

## Stage B — demonstrations (historical plan)

After bring-up, this laboratory will contain multiple demonstrations such as:

- static-IP Ethernet test;
- ping / network visibility test;
- TCP client;
- UDP sender/receiver;
- minimal web server;
- web page stored on SD;
- file browser over HTTP;
- SD logging;
- network configuration stored on SD.

## Stage C — practical projects (historical plan)

The same stand can then be used for real applications:

- network sensor node;
- SD data logger with web access;
- simple local status server;
- relay / actuator control;
- network event recorder;
- configuration server;
- small local telemetry endpoint.

## Rule

A practical project is added only after the underlying hardware path has been
verified by a smaller laboratory example.

## Parallel blue W5100 Ethernet laboratory (without SD)

There is now a separate [LAB-01B blue W5100 track](../01-W5100-Ethernet/README.md).
Its TEST-01 SPI, TEST-02 DHCP/Ping and TEST-03 HTTP are certified on the
blue W5100 sample; TEST-04 Web Control is pending. **Do not transfer** those
PASS verdicts to the original RobotDyn or to the microSD portion of this
laboratory without direct hardware measurements. Refer to the LAB-01B README
for the consolidated roadmap, pin map, current Arduino CLI and result files.

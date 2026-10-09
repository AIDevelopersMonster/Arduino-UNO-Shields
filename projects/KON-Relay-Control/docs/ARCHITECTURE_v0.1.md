# Architecture v0.1 — KON Relay & Control

Status: DESIGN / 2026-10-08

## Software layering

* KON_RelayShield — four physical GPIO outputs; no protocol knowledge.
* KON_ControlCore — deterministic state transitions, pulse timing, arbitration, safe-state policy, bounded event counters.
* Serial transport / Modbus RTU adapter — mapping Modbus coils to *commanded* states, explicit error responses.
* KON-PLC Lite — local scan cycle and a limited fixed-size rule table, later stage.
* Bluetooth Classic / BLE and KSC adapter — OPTIONAL later stages, not prerequisites.

## Important UART decision

ATmega328P on UNO has one hardware USART (D0/D1). Use HardwareSerial/Serial in 8E1 mode for primary Modbus RTU experiments. RS-485 transceiver DE and /RE may share one GPIO, proposed D2, with a hardware pull-down to ensure receive-disabled-transmit after reset. Verify actual module DE-/RE circuit.

USB bridge also shares UNO USART, so production RS-485 attachment and USB monitor must be explicitly switched, isolated, or routed to separate firmware profiles; connecting both and assuming independent operation is NOT supported. Require a resistor-controlled transceiver interface and documented wiring. Do not directly expose RS-485 A/B to MCU TTL.

An optional SoftwareSerial 8N1 demo is not equivalent to mandatory RTU 8E1 operation. On UNO it also has receive-timing and ISR limitations. Simultaneous full-reliability USB debug + Modbus + Bluetooth must use another MCU with more UARTs or an external bridge.

## Modbus RTU candidate

- Slave/server ID configurable, default 1 only after host test.
- Start at 9600 baud / 8E1; select 19200 only after timing tests. Do not assume any device defaults.
- CRC-16/Modbus little-endian transmission; RTU idle framing per serial line spec, broadcast no reply.
- Functions staged: 01 Read Coils, 05 Write Single Coil, 0F Write Multiple Coils; 03/06 holding registers later; 02/04 only if semantically valid sensor data exists.
- Coils PDU addresses 0..3 reflect **commanded output states** only. They are NOT measured relay contact feedback.
- Planned holding addresses: 0..3 pulse duration; 4 communication timeout; 5 operating mode. Freeze map and data units before public compatibility claim.
- On loss of Modbus communication define and test fail-policy OFF / HOLD, with default lab policy OFF after configurable timeout; watchdog handles MCU stalls separately.
- Conflict-resolution precedence candidate: hardware stop > fault > local PLC policy > authorized remote request. Requires versioned explicit acceptance.

## Hardware risk boundaries

- Relay contact labels (3 A) are not board-level or inductive switching certification.
- No bench 230 VAC work, first tests without load and then only suitable SELV low-voltage loads.
- Reset state and brownout behavior require actual measurements. Boot-time pin safety cannot be guaranteed by firmware.
- Do not claim IEC 61131-3 / EMC / SIL / functional-safety compliance.

## Resource inventory

UNO Flash 32 KB (bootloader consumes part), SRAM 2 KB, EEPROM 1 KB, 16 MHz. Measure binary and static RAM separately for each profile. Dynamic allocation disallowed in core. No SD/Ethernet/GUI in firmware stage 01.

## Upcoming profiles

* RELAY_01_USB_PROBE: pins INPUT; no output unless manually enabled after certification.
* RELAY_02_BASIC: dedicated low-voltage bench demo.
* RELAY_04_RS485_UART: half-duplex echo and framing verification.
* RELAY_05_MODBUS: registered coil control and diagnostics.
* RELAY_09_PLC: deterministic local scan + bounded rules.

## Acceptance gates

Stage 01: design and passive test plan complete, hardware physical certification PENDING.
Stage 02: independent library released and physically verified.
Stage 05: independent Modbus client and bus faults certified.
Stage 09: cycle/priority/watchdog demonstrated and resource margins documented.

# TEST-09 — development validation / 2026-10-10

**BUILD VERIFIED / actual first hardware CARD_INIT FAIL.** No physical UNO, card or network was
exercised by the assistant for TEST-09. The operator
confirmed AVR 1.8.8 build/upload at 25,236 B Flash / 1,440 B SRAM, then supplied
the [actual failed run](RESULT_2026-10-10.md); no UDP/SD load began. TEST-08's actual evidence belongs to
its separate result record and does not certify this new firmware.

- Actual Arduino CLI 1.3.1 build: arduino:avr:uno, AVR 1.8.6, Ethernet 2.0.2,
  SD 1.3.0. **25,236/32,256 B Flash; 1,440/2,048 B static SRAM**. Both gates pass.
  Final helper compiler_err is empty. The first uncached build emitted only
  existing AVR core new.cpp unused tag parameter warnings.
- The real helper records used libraries, executable sections and source/HEX
  SHA256; no AVR 1.8.8 size is inferred. Library source files are not vendored.
- All four PowerShell files parse. **29 host verdict cases pass**, including
  complete declared synthetic evidence and failures for reset, missing results,
  early completion, metadata, token, CRC, RAM, cleanup, byte/counter mismatches,
  DHCP failures and failed/duplicate-sequence UDP probes.
- g++ -std=c++11 -Wall -Wextra -Werror compiles the sketch's actual crc16,
  sdRoundtrip and handleCommand functions against declared mocks. The model
  checks matching/wrong-session STOP, single RUN, 30..600 s duration limits,
  command overflow, cache discard before read, all four packet sizes, and
  rejection of corrupt readback/short write/sync/close failure. It does not
  model real card power, electrical signaling or the whole Arduino scheduler.
- Python binascii.crc_hqx independently matches the PowerShell payload CRCs
  for token 0123ABCD / sequence 0x01020304: 16 B=5A6D, 32 B=C43B,
  64 B=50ED, 128 B=230D. Sequence encoding is little-endian 04 03 02 01.

Hardware acceptance requires the actual bounded host run, all its exact UDP
echoes and terminal UART evidence, sampled SRAM >=512 B, preserved RTR and
file cleanup. No synthetic console is saved as a hardware run.

Measured local firmware SHA256: `105CADB8A25DCBD82F3C9B1051959CC43176C2695D2371AA7E9D3A577E52CCA5`.
Measured local non-bootloader HEX SHA256: `B8FD4E0759D35097B0C9980E4A39BD0FA64B9027716C67E56BF212744EBD27B7`.
These identify this AVR 1.8.6 build; the operator must retain actual local measurements.

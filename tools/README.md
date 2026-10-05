# Tools

Utilities used for hardware checks, network tests, serial diagnostics,
SD-card verification and reproducible project workflows.


## KASM - KonSol KAP assembler

The [kasm](kasm/) directory contains the dependency-free Python assembler for
human-readable KAP1/KAP2 source.

It converts `.kasm` files into the same ASCII-hex `.KAP` format consumed by
KonSol, so application-development convenience is added on the PC rather than
inside the Arduino UNO firmware.


## KonSol SD Writer — CLI + GUI

The [konsol-transfer](konsol-transfer/) utility automates the already verified
KonSol Serial Shell transfer path. It reads a local KAP file and sends it to the
UNO as sequential `WRITE` / `APPEND` commands, waiting for the resident shell
acknowledgement after every chunk. A Tk GUI and CLI are provided.

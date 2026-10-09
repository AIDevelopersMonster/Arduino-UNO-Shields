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


## HY-M302 Remote Mapper — CLI + GUI

The [HY-M302-Remote-Mapper](HY-M302-Remote-Mapper/) utility automates learning
the NEC key map of a physical remote through the HY-M302 onboard IR receiver.

It can compile/upload the dedicated mapper firmware, guide the user through
named buttons, capture verified full NEC frames, reject duplicate assignments,
and generate JSON plus reusable C++ `HY_M302_RemoteMap.h/.cpp` files for the
test framework and KonSol-HY.


## KSC Host - terminal + remote /host service

The KSC / KonSol Commander project has its own host tool at:

```text
projects/KSC-KonSol-Commander/tools/ksc_host.py
```

It owns the Arduino COM port while KSC remote mounting is active and multiplexes:

- ordinary terminal input/output;
- framed HOSTFS requests;
- `/host` directory and file service;
- bounded OPEN / explicit-offset READ / CLOSE streaming;
- KSC-03D fault injection for lost READ responses and invalidated handles.

Quick start:

```powershell
python .\projects\KSC-KonSol-Commander\tools\ksc_host.py -p COM4
```

See:
[projects/KSC-KonSol-Commander/QUICKSTART.md](../projects/KSC-KonSol-Commander/QUICKSTART.md)

Stage preprint:
https://doi.org/10.5281/zenodo.23251546

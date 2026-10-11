# Arduino UNO & Shields — Repository Freeze Audit

**Date:** 2026-10-11  
**Repository state after audit:** **FROZEN / MAINTENANCE-ONLY**

## Decision

The Arduino UNO / ATmega328P line is retained as a reproducible legacy laboratory and auxiliary-hardware repository rather than an actively expanding development programme.

Project assessment:

- Arduino UNO remains simple, well understood and useful for small auxiliary tasks;
- for new primary work, its 8-bit ATmega328P, 32 KB Flash / 2 KB SRAM envelope and typical board price offer weak price/performance compared with modern MCU platforms;
- the repository therefore preserves successful experiments, tools, libraries, publications and hardware mappings without maintaining a forward feature roadmap;
- a UNO/shield may still be reused when existing hardware or already-tested code makes it the convenient choice.

## Audit scope

The consolidation branch contains:

- **63 README files**
- **74 Arduino .ino files**
- **17 PowerShell .ps1 files**
- **5 Windows .cmd launchers**
- **12 C/C++ headers**
- **17 C++ source files**
- **309 tracked blob files total**

All README files were reviewed for project-status language such as current, active, future, planned, roadmap, pending and proposed. Canonical top-level/project README files were updated where those words incorrectly implied an active programme.

Arduino sketch headers and host-script starts were also reviewed. Historical and publication-linked source files were not rewritten merely to add cosmetic header comments: preserving reproducibility and source hashes is more important than stylistic uniformity. New relay/W5100 profiles were given explicit frozen/maintenance status where needed.

## Consolidated final states

- **LAB-01 original W5100 + SD:** closed historical track; original shield suspect.
- **LAB-01B blue W5100:** Ethernet results retained; onboard shared microSD path rejected on tested clone samples after TEST-12; W5100 treated as legacy Ethernet-only hardware.
- **LAB-02 Multi-Function Shield:** Stage A PASS; frozen auxiliary lab.
- **LAB-03 MAR2406 TFT/Touch:** complete and closed.
- **LAB-04 FreeRTOS:** closed bounded experiment; TEST-01 PASS, filesystem-heavy TEST-02 stopped at practical memory limit.
- **LAB-05 KonSol 0.8:** published, reproducible and frozen.
- **KSC / KonSol Commander:** Arduino UNO + HY-M302 phase complete through KSC-04C; no additional UNO target scheduled here.
- **HY_M302:** retained as a tested low-overhead shield library.
- **KON Relay & Control:** D4-D7 / ACTIVE HIGH USB/Serial profile retained as bounded BENCH PASS; Ethernet profile retained as prototype; Modbus/KON-PLC roadmap deferred.
- **KON_SPI_Shared:** archived diagnostic helper, not a recommended production abstraction.

## Branch policy

`main` is the canonical preserved state. Historical feature branches and the old W5100 robustness pull request are superseded by the consolidated freeze state and should not be treated as active work.

Future Arduino UNO changes should normally be small, self-contained maintenance or auxiliary tasks and should start from `main`.

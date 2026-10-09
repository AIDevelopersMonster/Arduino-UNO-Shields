# KSC HY-M302 Stage - Zenodo Record

Status: **PUBLISHED**

DOI:

https://doi.org/10.5281/zenodo.23251546

Title:

**KonSol Commander on Arduino UNO with HY-M302: A Resident Cooperative
Environment with a Unified Virtual Namespace, Streamed Host Files, and Loadable
Runtime Behavior**

Resource type:

Publication / Preprint

Creator:

A. A. Malachevsky  
ORCID: 0009-0008-6009-3196

Publication date:

2026-10-09

## Scope

This is an **intermediate KSC stage publication**, not the final KSC programme
paper.

It records the completed Arduino UNO / ATmega328P + HY-M302 target phase through
KSC-04C:

- unified `/dev /proc /sys /host` namespace;
- ANSI Commander + shell over the same VFS;
- PC keyboard + HY-M302 IR semantic input;
- one Serial/COM link carrying TTY and framed HOSTFS;
- host-file streaming in <=32-byte reads;
- explicit-offset lost-response retry and BAD_HANDLE reopen/resume;
- 192-byte logical Viewer windows without a 192-byte page buffer;
- streamed KSC Script v0.1 execution;
- host-loaded RAM-resident SW1 behavior profile.

Final certified build:

```text
Flash       31024 / 32256 B = 96%
Global SRAM  1556 / 2048 B = 75%
Observed Commander free RAM ~380 B
```

Representative final status:

```text
RUN OK
RAM 380 B   IN drop 0   IR drop 0/0   HOST M/0 R0
```

## Evidence snapshot

The stage manuscript freezes the experimental evidence at:

```text
cd7669ed905d03c50d5559665e619441b522bf9e
```

Later commits may improve repository documentation without changing the
certified physical result.

## Related repository material

- [KSC project README](../../README.md)
- [Quick start](../../QUICKSTART.md)
- [Stage article spine](../KSC_HYM302_STAGE_ARTICLE_SPINE_v0.1.md)
- [KSC-04C physical certification](../../tests/KSC-04C-SW1-RUNTIME-PROFILE.md)
- [KSC Host protocol](../../protocol/KSC_HOST_PROTOCOL_v0.1.md)
- [Demo pack](../../host-share/DEMOS/)
- [Publication index](../README.md)

## Related publication

Earlier KSC_Core two-target result:

https://doi.org/10.5281/zenodo.23232216

## Video

KSC-04 final demonstration:

https://youtu.be/pqV5DG1o-WA

## Version policy

Do not silently expand this DOI into future LCD Keypad, relay/control,
W5100/SD, or other MCU/SBC results. Substantially new target phases should be
published as separate stage records or as later programme-level synthesis.

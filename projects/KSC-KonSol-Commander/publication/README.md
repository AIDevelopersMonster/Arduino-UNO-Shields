# KSC Publications

This directory separates published results, stage manuscripts, article spines,
and publication audits for KSC / KonSol Commander.

## 1. HY-M302 stage preprint - current stage publication

**KonSol Commander on Arduino UNO with HY-M302: A Resident Cooperative
Environment with a Unified Virtual Namespace, Streamed Host Files, and Loadable
Runtime Behavior**

DOI:

https://doi.org/10.5281/zenodo.23251546

Status:

**PUBLISHED STAGE PREPRINT**

Scope:

- Arduino UNO / ATmega328P + HY-M302;
- KSC-01 through KSC-04C;
- unified `/dev /proc /sys /host` namespace;
- TTY/HOSTFS multiplexing;
- <=32-byte remote READ streaming;
- explicit-offset recovery;
- 192-byte logical Commander viewer windows;
- streamed KSC Script v0.1 execution;
- runtime-selected SW1 behavior;
- final build 31024 / 32256 B Flash, 1556 / 2048 B global SRAM;
- observed Commander free RAM about 380 B.

The preprint closes the HY-M302 target phase, **not** the wider KSC programme.

Supporting design spine:

- [KSC_HYM302_STAGE_ARTICLE_SPINE_v0.1.md](KSC_HYM302_STAGE_ARTICLE_SPINE_v0.1.md)

Final video:

https://youtu.be/pqV5DG1o-WA

## 2. KSC_Core two-target publication - earlier frozen result

**KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for
Resource-Constrained 8-bit Systems**

DOI:

https://doi.org/10.5281/zenodo.23232216

Status:

**PUBLISHED / FROZEN TWO-TARGET RESULT**

Files:

- [KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md](KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md)
- [final publication audit](audit/KSC_CORE_v0.2_FINAL_PUBLICATION_AUDIT.md)
- [KSC_Core Zenodo preservation package](zenodo/README.md)

This earlier publication deliberately excludes the later KSC-03/KSC-04 results.

## Publication policy

KSC publications are cumulative but their claims are not silently broadened.

- the KSC_Core DOI remains the frozen two-target core result;
- the HY-M302 stage DOI records the later remote-host/viewer/launcher/runtime
  profile phase;
- future LCD Keypad, relay/control, W5100/SD or MCU/SBC results should receive
  their own stage records when scientifically substantial;
- a final cross-target KSC article should wait until several substantially
  different target phases exist.

## Reproducibility

Start with:

- [../QUICKSTART.md](../QUICKSTART.md)
- [../tests/](../tests/)
- [../protocol/KSC_HOST_PROTOCOL_v0.1.md](../protocol/KSC_HOST_PROTOCOL_v0.1.md)

Repository:

https://github.com/AIDevelopersMonster/Arduino-UNO-Shields

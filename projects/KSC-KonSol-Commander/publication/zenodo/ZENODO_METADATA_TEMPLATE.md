# Zenodo Metadata Template - KSC_Core v0.2

Do not publish until fields marked **CONFIRM** are resolved.

## DOI

    Existing DOI for this exact object: NO
    Action: reserve Zenodo DOI before final PDF if DOI is to appear in manuscript
    Reserved DOI: <CONFIRM AFTER RESERVATION>

## Resource type

    Publication
    Subtype: Preprint

## Main title

    KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for Resource-Constrained 8-bit Systems

## Subtitle

    Two-target physical validation on Arduino UNO using a synthetic Reference Target and the HY-M302 multifunction shield

## Publication date

    2026-10-08

Change this only if the actual Zenodo publication occurs on a later date and the
record should use that publication date.

## Creators

**CONFIRM**

Do not infer publication authorship from Git commits or account display names.

For each creator enter:

    Family name:
    Given name(s):
    ORCID:
    Affiliation:

## Description / abstract

KSC_Core is a target-decoupled resident shell, ANSI-style Commander, virtual
namespace, and semantic input core for Arduino UNO / ATmega328P. The same
unchanged KSC_Core source was physically validated against two target backends:
a synthetic bare-UNO Reference Target and a physical HY-M302 multifunction
shield adapter. The Reference Target exercised dynamic and writable synthetic
nodes without external hardware. The HY-M302 target exercised live sensor
reads, physical actuator writes, and asynchronous NEC IR input mapped into the
same CHAR/KEY model. The two configurations used identical KSC_Core source
blobs. The result supports a bounded claim of target-backend independence for
the two tested adapters on Arduino UNO; it does not establish universal
cross-MCU, cross-framework, transport, peripheral, or filesystem portability.

## Keywords

    Arduino UNO
    ATmega328P
    AVR
    KSC_Core
    KonSol Commander
    virtual namespace
    virtual filesystem
    VFS
    embedded systems
    terminal UI
    ANSI terminal
    hardware abstraction
    target adapter
    resource-constrained systems
    HY-M302
    IR remote
    reproducibility

## License

**CONFIRM**

Zenodo requires a license for the record and currently defaults to CC BY 4.0 in
the deposit interface.

Do not accept the default silently.

Candidate for the article/preprint if the creator chooses it:

    Creative Commons Attribution 4.0 International (CC BY 4.0)

Software licensing must be decided separately if source code itself is uploaded
as a licensed software artifact. A documentation license must not be silently
treated as a software license.

## Publisher

    Zenodo

unless the same publication was already formally published elsewhere before
this deposit.

## Language

    English

## Related identifiers

Recommended repository identifier:

    https://github.com/AIDevelopersMonster/Arduino-UNO-Shields

Select the final relation in the Zenodo UI according to the exact object being
deposited. Do not claim that the mutable repository URL is identical to the
frozen Zenodo publication object.

Experimental snapshot to state in description/notes:

    96487e66b159df75a1b3162098590cb23ab4af49

## Notes

Experimental validation is limited to two target backends on Arduino UNO /
ATmega328P. Both configurations used the same KSC_Core source blobs:

    KSC_Core.h
    9190ef5d231acc82498b46a897e32b6e4327ed92

    KSC_Core.cpp
    211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0

KSC-03 /host is future work and is not part of the validated result.

## Communities

**OPTIONAL / CONFIRM**

Submit only to communities whose scope clearly includes embedded systems,
Arduino/AVR, reproducible engineering, or closely related topics.

## Funding

    None declared unless creator supplies funding information.

## Contributors

Optional. Use only for people/organisations who contributed but should not
appear in the academic citation.

## Files

Recommended:

    KSC_CORE_TWO_TARGET_VALIDATION_v0.2.pdf
    KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md
    KSC_CORE_v0.1_PREPUBLICATION_AUDIT.md
    REPRODUCIBILITY.md
    SOURCE_MANIFEST.txt
    toolchain-final.txt

# KSC_Core Zenodo Package

Status: PRE-DEPOSIT PACKAGE

This directory prepares the KSC_Core v0.2 publication candidate for Zenodo.

The experimental evidence is frozen at commit:

    96487e66b159df75a1b3162098590cb23ab4af49

The article candidate is:

    ../KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md

The adversarial audit is:

    ../audit/KSC_CORE_v0.1_PREPUBLICATION_AUDIT.md

## Deposit object

Recommended Zenodo resource type:

    Publication / Preprint

Recommended primary title:

    KSC_Core: A Target-Decoupled Virtual Namespace and ANSI Commander Core for Resource-Constrained 8-bit Systems

Recommended subtitle:

    Two-target physical validation on Arduino UNO using a synthetic Reference Target and the HY-M302 multifunction shield

## Files to include

Minimum preservation package:

    KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md
    KSC_CORE_v0.1_PREPUBLICATION_AUDIT.md
    REPRODUCIBILITY.md
    ZENODO_METADATA_TEMPLATE.md
    SOURCE_MANIFEST.txt

Recommended final package additionally contains:

    KSC_CORE_TWO_TARGET_VALIDATION_v0.2.pdf
    toolchain-final.txt

The PDF should be generated only after a DOI is reserved if the DOI is to appear
inside the document.

## Pre-deposit gates

Do not publish the Zenodo record until all items below are resolved.

- [ ] creator name(s) confirmed;
- [ ] creator order confirmed;
- [ ] ORCID(s) confirmed;
- [ ] publication/document license explicitly selected;
- [ ] code-license status explicitly decided or code kept as external repository reference;
- [ ] final toolchain capture recorded;
- [ ] final manuscript reviewed after DOI insertion;
- [ ] DOI reserved if it will be printed in the manuscript;
- [ ] PDF/Markdown source pair visually and textually checked;
- [ ] source snapshot and core blob identifiers unchanged;
- [ ] final Zenodo description matches the bounded claim;
- [ ] no KSC-03 result is represented as completed.

## DOI workflow

If the manuscript should contain its own Zenodo DOI, create the Zenodo draft and
reserve a DOI before producing the final PDF.

Keep the draft. Deleting the draft loses the reserved DOI.

After inserting the DOI into the manuscript:

1. generate the final publication PDF;
2. verify the PDF;
3. upload the final package;
4. preview the Zenodo record;
5. publish only after metadata and files match.

## Version policy

This deposit should represent the KSC_Core two-target result only.

KSC-03 /host is a future result and should not be added to this record as if it
were part of the validated experiment.

A later substantive revision should be a new Zenodo version rather than silent
replacement of the experimental claim.

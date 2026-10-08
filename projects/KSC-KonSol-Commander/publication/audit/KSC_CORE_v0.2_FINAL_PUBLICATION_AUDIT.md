# KSC_Core v0.2 - Final Publication Audit

**Audit type:** final pre-Zenodo scientific-engineering and visual review  
**Manuscript:** `KSC_CORE_TWO_TARGET_VALIDATION_v0.2.md`  
**DOI:** 10.5281/zenodo.23232216  
**Author:** A. A. Malachevsky  
**ORCID:** 0009-0008-6009-3196  
**License:** CC BY 4.0 for the article/preprint  
**Audit date:** 2026-10-08  
**Verdict:** PASS - READY FOR ZENODO FILE UPLOAD/PREVIEW

---

## 1. Content review

The final v0.2 manuscript was reviewed after DOI insertion.

The central claim remains bounded to the evidence:

> On the tested Arduino UNO platform, KSC_Core is independent of the concrete
> implementation of the two physically validated target backends: the synthetic
> Reference Target and the HY-M302 Target.

The final manuscript does not claim:

- universal MCU portability;
- non-AVR portability;
- Arduino-framework independence;
- POSIX filesystem compatibility;
- formal worst-case stack safety;
- lossless input at arbitrary rates;
- DHT metrological accuracy;
- completion of KSC-03 /host.

Prior-art boundaries for procfs/sysfs remain explicit.

The runtime SRAM methodology remains explicitly empirical and instantaneous.

The exact core blob identities remain:

```text
KSC_Core.h
9190ef5d231acc82498b46a897e32b6e4327ed92

KSC_Core.cpp
211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0
```

The experimental evidence snapshot remains:

```text
96487e66b159df75a1b3162098590cb23ab4af49
```

---

## 2. Reproducibility review

The privacy-scrubbed publication record contains:

```text
OS: Windows 10 Home, version 2009, build 19045
Git: 2.45.2.windows.1
Arduino CLI: 1.5.2-rc.1
Arduino CLI commit: fef6e48df
Arduino AVR core: 1.8.8
Board FQBN: arduino:avr:uno
Python: 3.14.4
pyserial: 3.5
Core-separation check: PASS
```

Local user-profile paths and unrelated working-tree filenames are excluded from
the public record.

---

## 3. PDF visual review

Final PDF filename:

```text
KSC_Core_v0.2_Zenodo_FINAL.pdf
```

Observed PDF properties:

```text
Pages: 12
Encrypted: no
Openable: yes
Scanned/image-only: no
```

Visual inspection was performed after rendering all 12 PDF pages.

PASS criteria:

- no clipped text observed;
- no text overlap observed;
- no broken tables observed;
- code blocks remain readable;
- the comparative results table fits cleanly;
- title, DOI, ORCID, license, and author metadata are readable;
- footer page numbers render correctly;
- Cyrillic text in Appendix B renders correctly;
- no broken-glyph boxes observed;
- no private local user/profile path appears in the PDF;
- final manuscript wording consistently describes v0.2 as a final preprint.

The large whitespace before Appendix A is acceptable and results from keeping
the compact certified statement together as one code block.

---

## 4. Final PDF integrity

Generated PDF SHA-256:

```text
4b9d4b1ba58e49098f9224cddd57d9ab84834fe68a543a047b94feb755c6442e
```

Generated PDF size:

```text
291802 bytes
```

If the PDF is regenerated, the checksum must be recomputed and this audit
updated before using the checksum as a deposit integrity marker.

---

## 5. Privacy review

Search of extracted PDF text found no occurrence of the previously identified
local user-profile identifier or private local paths.

Public scholarly identifiers intentionally remain:

- A. A. Malachevsky;
- ORCID 0009-0008-6009-3196;
- DOI 10.5281/zenodo.23232216.

These are publication metadata, not local-machine identifiers.

---

## 6. Final decision

```text
Scientific claim discipline:        PASS
Two-target evidence consistency:    PASS
Core source identity:               PASS
Resource table consistency:         PASS
Runtime-memory qualification:       PASS
Prior-art boundary:                 PASS
Reproducibility metadata:           PASS
Privacy scrub:                      PASS
DOI / ORCID / creator metadata:     PASS
Article license:                    PASS
KSC-03 separation:                  PASS
PDF visual inspection:              PASS
PDF text/privacy inspection:        PASS

FINAL VERDICT:
READY FOR ZENODO FILE UPLOAD AND RECORD PREVIEW
```

Publication itself should occur only after the uploaded files and Zenodo record
preview are checked against the final manuscript and metadata template.

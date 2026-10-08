# KSC_Core v0.2 - Reproducibility Record

## 1. Frozen experimental evidence

Repository:

    https://github.com/AIDevelopersMonster/Arduino-UNO-Shields

Branch used during experiment:

    feature/ksc-konsol-commander

Experimental evidence snapshot:

    96487e66b159df75a1b3162098590cb23ab4af49

Core blob identity:

    KSC_Core.h
    9190ef5d231acc82498b46a897e32b6e4327ed92

    KSC_Core.cpp
    211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0

## 2. Required final environment capture

Run from PowerShell before deposit:

~~~powershell
"=== DATE ==="
Get-Date -Format o

"=== OS ==="
Get-ComputerInfo |
  Select-Object WindowsProductName, WindowsVersion, OsBuildNumber

"=== GIT ==="
git --version
git rev-parse HEAD
git status --short

"=== ARDUINO CLI ==="
arduino-cli version

"=== INSTALLED CORES ==="
arduino-cli core list

"=== PYTHON ==="
python --version

"=== PYSERIAL ==="
python -m pip show pyserial
~~~

Save the complete output:

~~~powershell
& {
  "=== DATE ==="
  Get-Date -Format o

  "=== OS ==="
  Get-ComputerInfo |
    Select-Object WindowsProductName, WindowsVersion, OsBuildNumber

  "=== GIT ==="
  git --version
  git rev-parse HEAD
  git status --short

  "=== ARDUINO CLI ==="
  arduino-cli version

  "=== INSTALLED CORES ==="
  arduino-cli core list

  "=== PYTHON ==="
  python --version

  "=== PYSERIAL ==="
  python -m pip show pyserial
} *> .\projects\KSC-KonSol-Commander\publication\zenodo\toolchain-final.txt
~~~

Important: working-tree status is recorded because exact rebuild claims should
distinguish the committed snapshot from unrelated local modifications.

## 3. Reference Target rebuild

~~~powershell
New-Item -ItemType Directory -Force .\build\KSC\05_KSC_Core_Reference | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --output-dir .\build\KSC\05_KSC_Core_Reference `
  .\projects\KSC-KonSol-Commander\sketches\05_KSC_Core_Reference
~~~

Certified build observation:

    Sketch uses 13658 bytes (42%) of program storage space.
    Global variables use 669 bytes (32%) of dynamic memory.

Upload:

~~~powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\05_KSC_Core_Reference
~~~

Terminal:

~~~powershell
python `
  .\projects\KSC-KonSol-Commander\tools\ksc_raw_tty.py `
  -p COM4
~~~

Certified runtime observations:

    Boot free RAM:             1320 B
    Commander free RAM:        1279 B typical
    Minimum observed:          1262 B
    Input drops observed:         0

Full route:

    projects/KSC-KonSol-Commander/tests/KSC-02C-CORE-REFERENCE-TARGET.md

## 4. HY-M302 Target rebuild

~~~powershell
New-Item -ItemType Directory -Force .\build\KSC\06_KSC_Core_HY_M302 | Out-Null

arduino-cli compile `
  --fqbn arduino:avr:uno `
  --libraries .\projects\KSC-KonSol-Commander\libraries `
  --libraries .\libraries `
  --output-dir .\build\KSC\06_KSC_Core_HY_M302 `
  .\projects\KSC-KonSol-Commander\sketches\06_KSC_Core_HY_M302
~~~

Certified build observation:

    Sketch uses 20806 bytes (64%) of program storage space.
    Global variables use 1255 bytes (61%) of dynamic memory.

Upload:

~~~powershell
arduino-cli upload `
  -p COM4 `
  --fqbn arduino:avr:uno `
  --input-dir .\build\KSC\06_KSC_Core_HY_M302
~~~

Terminal:

~~~powershell
python `
  .\projects\KSC-KonSol-Commander\tools\ksc_raw_tty.py `
  -p COM4
~~~

Certified runtime observations:

    Boot free RAM:              734 B
    Commander free RAM:         693 B typical
    Minimum observed:           676 B
    Input drops observed:         0
    IR drops observed:           0 / 0

Full route:

    projects/KSC-KonSol-Commander/tests/KSC-02D-HY-M302-TARGET-ADAPTER.md

## 5. Runtime-memory interpretation

kscFreeRam() is an instantaneous AVR runtime estimate. The reported minimum is
the smallest value observed in the preserved route, not a formal stack
high-water proof.

## 6. Terminal test-harness scope

The physically certified helper:

    projects/KSC-KonSol-Commander/tools/ksc_raw_tty.py

uses Python msvcrt and therefore the tested helper route is Windows-specific.

No cross-platform terminal-helper claim is made in this publication.

## 7. Core-separation inspection

For the publication snapshot, verify the KSC_Core source contains no target
implementation:

~~~powershell
Select-String -Path `
  .\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.h,`
  .\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.cpp `
  -Pattern 'HY_M302|DHT|pinMode|digitalWrite|digitalRead|analogRead|analogWrite'
~~~

Expected result:

    no matches

## 8. Reproduction claim

A rebuild that differs in compiler/core versions may legitimately produce
different flash/SRAM totals.

Therefore the strongest exact-size reproduction claim should be made only when
the final captured toolchain matches the environment used for the certified
build or when the size difference is explicitly documented.

$ErrorActionPreference = 'Continue'

$OutFile = Join-Path $PSScriptRoot 'toolchain-final.txt'

& {
  "KSC_Core v0.2 final toolchain capture"
  "====================================="
  ""

  "=== DATE ==="
  Get-Date -Format o
  ""

  "=== OS ==="
  try {
    Get-ComputerInfo |
      Select-Object WindowsProductName, WindowsVersion, OsBuildNumber
  }
  catch {
    "Get-ComputerInfo failed: $($_.Exception.Message)"
  }
  ""

  "=== GIT ==="
  git --version
  "HEAD:"
  git rev-parse HEAD
  "STATUS:"
  git status --short
  ""

  "=== ARDUINO CLI ==="
  arduino-cli version
  ""

  "=== INSTALLED CORES ==="
  arduino-cli core list
  ""

  "=== PYTHON ==="
  python --version
  ""

  "=== PYSERIAL ==="
  python -m pip show pyserial
  ""

  "=== CORE SEPARATION CHECK ==="
  $CoreFiles = @(
    '.\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.h',
    '.\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.cpp'
  )

  $Matches = Select-String -Path $CoreFiles -Pattern 'HY_M302|DHT|pinMode|digitalWrite|digitalRead|analogRead|analogWrite'

  if ($Matches) {
    "FAIL: target-specific references found:"
    $Matches
  }
  else {
    "PASS: no target-specific references found."
  }
  ""

  "=== EXPECTED CORE BLOBS ==="
  "KSC_Core.h   9190ef5d231acc82498b46a897e32b6e4327ed92"
  "KSC_Core.cpp 211803fcc045b8a2dbba029fa0f7f62b2f6ac8e0"
  ""

  "=== PUBLICATION DOI ==="
  "10.5281/zenodo.23232216"
  "https://doi.org/10.5281/zenodo.23232216"
} *> $OutFile

Write-Host "WROTE $OutFile"
Get-Content $OutFile

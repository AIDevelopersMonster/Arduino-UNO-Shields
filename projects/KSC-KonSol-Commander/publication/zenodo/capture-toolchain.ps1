$ErrorActionPreference = 'Continue'

$OutFile = Join-Path $PSScriptRoot 'toolchain-public.txt'

$OutputEncoding = [System.Text.UTF8Encoding]::new()
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()

$DirtyEntries = @(git status --porcelain)
$WorkingTreeState = if ($DirtyEntries.Count -eq 0) { 'CLEAN' } else { 'NOT CLEAN' }

$Os = Get-ComputerInfo |
  Select-Object WindowsProductName, WindowsVersion, OsBuildNumber

$PySerialVersion = python -c "import serial; print(serial.__version__)"

& {
  "KSC_Core v0.2 public toolchain record"
  "====================================="
  ""

  "Capture time:"
  Get-Date -Format o
  ""

  "Privacy:"
  "Local filesystem paths, user-profile identifiers, and unrelated working-tree"
  "filenames are intentionally omitted from this public record."
  ""

  "OS:"
  $Os.WindowsProductName
  "Windows version: $($Os.WindowsVersion)"
  "OS build: $($Os.OsBuildNumber)"
  ""

  "Git:"
  git --version
  "Capture HEAD:"
  git rev-parse HEAD
  ""

  "Working tree:"
  $WorkingTreeState
  if ($DirtyEntries.Count -gt 0) {
    "Uncommitted entries: $($DirtyEntries.Count)"
    "Paths intentionally omitted."
  }
  ""

  "Arduino CLI:"
  arduino-cli version
  ""

  "Relevant Arduino core:"
  arduino-cli core list | Select-String 'arduino:avr'
  "Board FQBN used:"
  "arduino:avr:uno"
  ""

  "Python:"
  python --version
  ""

  "pyserial:"
  $PySerialVersion
  ""

  "Core separation check:"
  $CoreFiles = @(
    '.\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.h',
    '.\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.cpp'
  )

  $Matches = Select-String -Path $CoreFiles -Pattern 'HY_M302|DHT|pinMode|digitalWrite|digitalRead|analogRead|analogWrite'

  if ($Matches) {
    "FAIL - target-specific references found."
  }
  else {
    "PASS - no target-specific references found."
  }
  ""

  "Validated KSC_Core blobs:"
  "KSC_Core.h"
  git hash-object .\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.h
  ""
  "KSC_Core.cpp"
  git hash-object .\projects\KSC-KonSol-Commander\libraries\KSC_Core\src\KSC_Core.cpp
  ""

  "Experimental evidence snapshot:"
  "96487e66b159df75a1b3162098590cb23ab4af49"
  ""

  "Publication DOI:"
  "10.5281/zenodo.23232216"
  "https://doi.org/10.5281/zenodo.23232216"
} | Set-Content -Path $OutFile -Encoding UTF8

Write-Host "WROTE $OutFile"
Get-Content $OutFile

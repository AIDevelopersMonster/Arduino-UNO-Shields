#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildSummary,
    [Parameter(Mandatory)][string[]]$ScenarioSummary,
    [Parameter(Mandatory)][string]$Sample,
    [string]$OutputPath=(Join-Path $PSScriptRoot 'runs/certification.json')
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'NetworkRobustness.psm1') -Force
$build=Get-Content -Raw -LiteralPath $BuildSummary | ConvertFrom-Json -AsHashtable
$errors=[Collections.Generic.List[string]]::new()
if($build.test -ne 'TEST-07' -or $build.type -ne 'BUILD_ONLY' -or $build.status -ne 'PASS' -or $build.flash_bytes -gt 29000 -or $build.sram_static_bytes -gt 1200 -or $build.core -notin @('1.8.6','1.8.8') -or $build.ethernet -ne '2.0.2') {
    $errors.Add('build/memory gate failed')
}
$seen=[Collections.Generic.HashSet[string]]::new()
$reports=@(); $com=''
foreach($path in $ScenarioSummary) {
    $r=Get-Content -Raw -LiteralPath $path | ConvertFrom-Json -AsHashtable
    $reports+=@{path=[IO.Path]::GetFullPath($path);sha256=(Get-FileHash $path).Hash;scenario=$r.scenario;status=$r.status}
    $v=Get-NetworkVerdict $r.metrics
    if($r.scenario -ne $r.metrics.Scenario -or $r.actual_duration_s -lt $r.metrics.DurationSeconds) { $errors.Add("inconsistent run metadata: $($r.scenario)") }
    if($r.test -ne 'TEST-07' -or $r.status -ne 'PASS' -or $v.Status -ne 'PASS') { $errors.Add("scenario failed: $($r.scenario)") }
    if(-not $seen.Add($r.scenario)){ $errors.Add("duplicate scenario: $($r.scenario)") }
    if(-not $com){$com=$r.serial_port};if($com -ne $r.serial_port){$errors.Add('different serial ports; verify sample identity')}
    foreach($key in $build.source_hashes.Keys) {
        if($r.source_hashes[$key] -ne $build.source_hashes[$key]){$errors.Add("source mismatch: $($r.scenario) / $key")}
    }
}
foreach($name in @('Baseline','Cable','StartupDhcp','DhcpRenew','DhcpOutage','Soak','TcpAbort')) {
    if(-not $seen.Contains($name)){$errors.Add("missing scenario: $name")}
}
$dir=Split-Path -Parent ([IO.Path]::GetFullPath($OutputPath));$null=New-Item -ItemType Directory -Path $dir -Force
$cert=[ordered]@{test='TEST-07';sample=$Sample;status=$(if($errors.Count){'PARTIAL_OR_FAIL'}else{'FULL PASS'})
    utc=[DateTime]::UtcNow.ToString('o');reasons=@($errors);build=$build;reports=$reports
    scope='Operator identifies this sample and physical faults; measured bounded run only; no certification of all W5100 clones.'}
$cert | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Host "$($cert.status) / $OutputPath"
foreach($e in $errors){Write-Host $e}
if($errors.Count){exit 1}

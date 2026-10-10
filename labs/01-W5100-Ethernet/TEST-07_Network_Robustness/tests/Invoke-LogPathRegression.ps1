#requires -Version 7.0
<# Host-only regression: execute the actual runner's log initialization with
   different PowerShell and process working directories. No COM/network access. #>
[CmdletBinding()]
param([string]$RunnerPath=(Join-Path $PSScriptRoot '../Test-NetworkRobustness.ps1'))
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$runner=(Resolve-Path -LiteralPath $RunnerPath).Path
$source=Get-Content -LiteralPath $runner -Raw
$cut=$source.IndexOf('$clock = [Diagnostics.Stopwatch]::StartNew()',[StringComparison]::Ordinal)
if($cut -lt 0){throw 'Runner log-initialization boundary not found'}
$fixtureRoot=Join-Path ([IO.Path]::GetTempPath()) ('test07-logpaths-'+[guid]::NewGuid().ToString('N'))
$originalLocation=Get-Location
$originalProcessDirectory=[Environment]::CurrentDirectory
try {
    $fixtureDir=(New-Item -ItemType Directory -Path (Join-Path $fixtureRoot 'fixture') -Force).FullName
    $project=(New-Item -ItemType Directory -Path (Join-Path $fixtureRoot 'project с пробелом') -Force).FullName
    $processDir=(New-Item -ItemType Directory -Path (Join-Path $fixtureRoot 'process') -Force).FullName
    Copy-Item -LiteralPath (Join-Path (Split-Path $runner) 'NetworkRobustness.psm1') -Destination $fixtureDir
    $fixture=Join-Path $fixtureDir 'LogFixture.ps1'
    $tail=@'

try {
    $events.WriteLine('fixture-event')
    $uart.WriteLine('fixture-uart')
    $probes.WriteLine('fixture-probe')
} finally {
    $events.Dispose(); $uart.Dispose(); $probes.Dispose()
}
[pscustomobject]@{Run=$run}
'@
    Set-Content -LiteralPath $fixture -Value ($source.Substring(0,$cut)+$tail) -Encoding utf8
    $cases=@(
        @{Name='relative nested';Argument='./runs/controls/card-present-sd-idle';Parent=(Join-Path $project 'runs/controls/card-present-sd-idle')},
        @{Name='relative parent with spaces';Argument='../shared logs/nested';Parent=(Join-Path $fixtureRoot 'shared logs/nested')},
        @{Name='current directory';Argument='.';Parent=$project},
        @{Name='absolute directory';Argument=(Join-Path $fixtureRoot 'absolute logs');Parent=(Join-Path $fixtureRoot 'absolute logs')},
        @{Name='default directory';Argument=$null;Parent=(Join-Path $fixtureDir 'runs')}
    )
    foreach($case in $cases){
        Set-Location -LiteralPath $project
        [Environment]::CurrentDirectory=$processDir
        if($case.Argument){$result=& $fixture -SerialPort 'HOST_FIXTURE_UNUSED' -OutputDirectory $case.Argument}
        else{$result=& $fixture -SerialPort 'HOST_FIXTURE_UNUSED'}
        if(-not [IO.Path]::IsPathRooted($result.Run)){throw "Relative run path: $($case.Name)"}
        $expectedParent=[IO.Path]::GetFullPath($case.Parent)
        if([IO.Path]::GetDirectoryName($result.Run) -ne $expectedParent){throw "Wrong log directory: $($case.Name)"}
        foreach($item in @(@('events.jsonl','fixture-event'),@('serial.log','fixture-uart'),@('probes.csv','fixture-probe'))){
            $path=Join-Path $result.Run $item[0]
            if(-not [IO.File]::Exists($path) -or -not [IO.File]::ReadAllText($path).Contains($item[1])){
                throw "Missing/unflushed log: $($case.Name) / $($item[0])"
            }
        }
        if(@(Get-ChildItem -LiteralPath $processDir -Force).Count){throw 'Logs leaked into the process working directory'}
        Write-Host "PASS / $($case.Name)"
    }
    Write-Host 'LOG PATH REGRESSION PASS / 5 cases / no COM or network'
} finally {
    Set-Location -LiteralPath $originalLocation.Path
    [Environment]::CurrentDirectory=$originalProcessDirectory
    Remove-Item -LiteralPath $fixtureRoot -Recurse -Force -ErrorAction SilentlyContinue
}

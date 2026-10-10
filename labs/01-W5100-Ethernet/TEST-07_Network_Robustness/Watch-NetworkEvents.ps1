#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SerialPort,
    [ValidateRange(1,86400)][int]$DurationSeconds=120,
    [string]$OutputDirectory=(Join-Path $PSScriptRoot 'runs')
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'NetworkRobustness.psm1') -Force
$null=New-Item -ItemType Directory -Path $OutputDirectory -Force
$path=Join-Path $OutputDirectory ('watch-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.jsonl')
$writer=[IO.StreamWriter]::new($path,$false,[Text.UTF8Encoding]::new($false))
$writer.AutoFlush=$true
$serial=[IO.Ports.SerialPort]::new($SerialPort,115200)
$serial.DtrEnable=$true; $serial.RtsEnable=$false
$pending=''; $timer=[Diagnostics.Stopwatch]::StartNew()
try {
    Write-Host 'Standalone UART logger; do not run with Test-NetworkRobustness on the same COM. Opening may reset UNO.'
    $serial.Open()
    while($timer.Elapsed.TotalSeconds -lt $DurationSeconds) {
        $pending+=$serial.ReadExisting()
        if($pending.Length -gt 65536){throw 'UART line overflow'}
        while(($lf=$pending.IndexOf("`n")) -ge 0) {
            $line=$pending.Substring(0,$lf).TrimEnd("`r"); $pending=$pending.Substring($lf+1)
            $obj=ConvertFrom-NetworkLine $line
            $writer.WriteLine((@{utc=[DateTime]::UtcNow.ToString('o');line=$line;fields=$obj}|ConvertTo-Json -Depth 6 -Compress))
            Write-Host $line
        }
        Start-Sleep -Milliseconds 20
    }
} finally {if($serial.IsOpen){$serial.Close()};$serial.Dispose();$writer.Dispose()}
Write-Host "Saved $path / no PASS assessment performed"

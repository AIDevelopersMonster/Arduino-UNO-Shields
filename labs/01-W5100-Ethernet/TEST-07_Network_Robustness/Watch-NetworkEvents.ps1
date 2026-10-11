#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SerialPort,
    [ValidateRange(1,86400)][int]$DurationSeconds=120,
    [string]$OutputDirectory=(Join-Path $PSScriptRoot 'runs'),
    [ValidateSet('SD_THEN_ETH','ETH_ONLY')][string]$LibraryStartupMode
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
$modeSent=$false
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
            # Colour reported decisions only; keep the saved UART line unchanged.
            if ($line -match '(?:^|\s)status=FAIL(?:\s|$)|^RESULT FAIL(?:\s|$)') {
                Write-Host $line -ForegroundColor Red
            } elseif ($line -match '(?:^|\s)status=PASS(?:\s|$)|^RESULT PASS(?:\s|$)') {
                Write-Host $line -ForegroundColor Green
            } else {
                Write-Host $line
            }
            if ($LibraryStartupMode -and $line -eq 'READY test=LIBRARY_STARTUP fw=0.2') {
                if ($modeSent) { throw 'Repeated LibraryStartup READY; control run interrupted' }
                $serial.Write($LibraryStartupMode+"`n")
                $modeSent=$true
                $writer.WriteLine((@{utc=[DateTime]::UtcNow.ToString('o');line="HOST mode=$LibraryStartupMode";direction='tx';fields=$null}|ConvertTo-Json -Depth 6 -Compress))
                Write-Host "РЕЖИМ: $LibraryStartupMode отправлен; сохраните подготовленное положение карты и подключение Ethernet-кабеля." -ForegroundColor Cyan
            }
        }
        Start-Sleep -Milliseconds 20
    }
    if ($LibraryStartupMode -and -not $modeSent) { throw 'LibraryStartup fw=0.2 READY not captured; mode not sent' }
} finally {if($serial.IsOpen){$serial.Close()};$serial.Dispose();$writer.Dispose()}
Write-Host "Saved $path / no PASS assessment performed"

#requires -Version 7.0
<# Runs the actual Drain-Serial and Write-Event bodies with in-memory UART.
   No COM, physical boot, Ethernet or hardware PASS is involved.
#>
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'NetworkRobustness.psm1') -Force
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $root 'Test-NetworkRobustness.ps1'), [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw $errors[0] }
foreach ($name in @('Write-Event','Drain-Serial')) {
    $fn = $ast.Find({param($node)
        $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name
    }, $true)
    if (-not $fn) { throw "Missing actual function: $name" }
    Invoke-Expression $fn.Extent.Text
}
$clock = [Diagnostics.Stopwatch]::StartNew()
function Test-UartCase([string]$Name, [string[]]$Chunks, [int]$Boots, [bool]$Firmware) {
    $state = @{Pending='';LastBoardMs=-1}
    $m = @{Boots=0;Firmware=$false;Fatal=''}
    $events = [IO.StringWriter]::new()
    $uart = [IO.StringWriter]::new()
    $serial = [pscustomobject]@{Remaining=''}
    $serial | Add-Member ScriptMethod ReadExisting {
        $value = $this.Remaining; $this.Remaining = ''; return $value
    }
    try {
        foreach ($chunk in $Chunks) { $serial.Remaining=$chunk; Drain-Serial }
        if ($m.Boots -ne $Boots -or $m.Firmware -ne $Firmware -or $m.Fatal) {
            throw "${Name}: Boots=$($m.Boots), Firmware=$($m.Firmware), Fatal=$($m.Fatal)"
        }
        # Raw line content stays in the log; parser does not strip noise prefixes.
        if ($uart.ToString() -notmatch 'reset-noise') { throw "${Name}: raw noise lost" }
        Write-Host "HOST PASS / $Name"
    } finally { $events.Dispose(); $uart.Dispose() }
}
$boot = 'EVT ms=349 name=BOOT'
$info = 'INFO test=07 version=0.2'
Test-UartCase 'merged BOOT remains unrecognized' @("reset-noise$boot`n$info`n") 0 $true
Test-UartCase 'framed BOOT counted once' @("reset-noise`r`n$boot`r`n$info`r`n") 1 $true
Test-UartCase 'missing BOOT stays missing' @("reset-noise`n$info`n") 0 $true
Test-UartCase 'duplicate BOOT is not deduplicated' @("reset-noise`n$boot`n$boot`n$info`n") 2 $true
Test-UartCase 'old firmware banner is rejected' @("reset-noise`n$boot`nINFO test=07 version=0.1`n") 1 $false
Test-UartCase 'fragmented line assembled across reads' @("reset-noise`nEVT ms=349 na", "me=BOOT`n$info`n") 1 $true
Write-Host 'HOST PASS / 6 UART regression cases; hardware PENDING.'

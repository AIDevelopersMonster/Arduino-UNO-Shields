#requires -Version 7.0
param([string]$Python='python')
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../NetworkRobustness.psm1') -Force
function Assert([bool]$Condition,[string]$Name){if(-not $Condition){throw "FAILED: $Name"};Write-Host "OK $Name"}
$m=@{Scenario='Baseline';Completed=$true;DurationSeconds=120;HealthySeconds=90;Fatal='';Stats=3;Boots=1;Firmware=$true
    W5100=$true;MinFree=900;RamDrift=0;Corrupt=0;HealthyFail=0;HealthyOK=24;TcpOK=3;Boundaries=4
    SendFail=0;Drop=0;ManualDhcp=0;Restored=$true;Recovered=$true;RecoveryMs=1200;RecoveryLimitMs=45000
    PostOK=24;PostTcpOK=3;OutageMs=15000;OutageFailures=3;DhcpFailuresBeforeRestore=1
    MaintainFailuresBeforeRestore=1;RenewOK=1;Aborts=5;TcpReject=5}
Assert ((Get-NetworkVerdict $m).Status -eq 'PASS') 'complete synthetic baseline accepted'
foreach($scenario in @('Cable','StartupDhcp','DhcpOutage','DhcpRenew','TcpAbort')){
    $x=$m.Clone();$x.Scenario=$scenario
    Assert ((Get-NetworkVerdict $x).Status -eq 'PASS') "complete synthetic $scenario accepted"
}
$x=$m.Clone();$x.Scenario='Soak';$x.DurationSeconds=600;$x.HealthySeconds=570
Assert ((Get-NetworkVerdict $x).Status -eq 'PASS') 'complete synthetic soak accepted'
foreach($change in @(
    @{Completed=$false},@{Boots=2},@{Boots=0},@{Stats=0},@{Firmware=$false},@{W5100=$false},@{MinFree=511},@{RamDrift=65},
    @{HealthyFail=1},@{Corrupt=1},@{Boundaries=3},@{SendFail=1},@{ManualDhcp=1},
    @{Scenario='Cable';OutageFailures=0},@{Scenario='Cable';Restored=$false},
    @{Scenario='Cable';RecoveryMs=45001},@{Scenario='StartupDhcp';DhcpFailuresBeforeRestore=0},
    @{Scenario='DhcpRenew';RenewOK=0},@{Scenario='DhcpOutage';MaintainFailuresBeforeRestore=0},
    @{Scenario='TcpAbort';TcpReject=4},@{Scenario='Soak';DurationSeconds=120},
    @{Scenario='Cable';PostOK=23},@{Scenario='Cable';PostTcpOK=2}
)) {
    $x=$m.Clone();foreach($key in $change.Keys){$x[$key]=$change[$key]}
    Assert ((Get-NetworkVerdict $x).Status -eq 'FAIL') ('reject '+($change|ConvertTo-Json -Compress))
}
$obj=ConvertFrom-NetworkLine 'EVT ms=123 name=DHCP_MAINTAIN rc=2 elapsed_ms=51'
Assert ($obj.name -eq 'DHCP_MAINTAIN' -and $obj.rc -eq '2') 'UART event parsing'
$data=New-ProbePayload 128 42 ([guid]::NewGuid().ToByteArray())
Assert ($data.Length -eq 128 -and [BitConverter]::ToUInt32($data,8) -eq 42) 'binary sequence encoding'
$g=$m.Clone();$g.Scenario='Cable'
$s=@{Phase='HEALTHY';HealthyStart=1000;FaultRequestedMs=0;FaultConfirmed=$false;EarlyRestore=$false;FailureStreak=0;FirstFailureMs=-1;CutMs=-1}
Assert ((Get-NetworkGuidance $g $s 20999) -eq '') 'no disconnect instruction before automatic baseline interval'
Assert ((Get-NetworkGuidance $g $s 21000) -eq 'REQUEST_DISCONNECT') 'script requests disconnect after complete baseline'
$g.TcpOK=2
Assert ((Get-NetworkGuidance $g $s 25000) -eq '') 'no disconnect instruction before three TCP exchanges'
$g.TcpOK=3;$g.HealthyFail=1
Assert ((Get-NetworkGuidance $g $s 25000) -eq 'STOP_BASELINE') 'bad baseline stops before physical fault instruction'
$g.HealthyFail=0;$s.Phase='OUTAGE'
Update-NetworkOutage $s 'TIMEOUT' 1000
Update-NetworkOutage $s 'PASS' 2000
Assert (-not $s.FaultConfirmed -and $s.FailureStreak -eq 0) 'isolated loss does not confirm cable outage'
Update-NetworkOutage $s 'TIMEOUT' 3000
Update-NetworkOutage $s 'CORRUPT' 4000
Assert ($s.FailureStreak -eq 0) 'corrupt reply is not proof of no network response'
foreach($time in @(5000,6000,7000)){Update-NetworkOutage $s 'TIMEOUT' $time}
Assert ($s.FaultConfirmed -and $s.CutMs -eq 5000) 'three consecutive failures establish observation origin'
Assert ((Get-NetworkGuidance $g $s 19999) -eq '') 'script keeps connection down through hold interval'
Assert ((Get-NetworkGuidance $g $s 20000) -eq 'REQUEST_RESTORE') 'script gives restore instruction after measured hold'
$g.Scenario='StartupDhcp';$g.DhcpFailuresBeforeRestore=0
Assert ((Get-NetworkGuidance $g $s 20000) -eq '') 'startup does not restore without actual DHCP failure'
$g.DhcpFailuresBeforeRestore=1
Assert ((Get-NetworkGuidance $g $s 20000) -eq 'REQUEST_RESTORE') 'startup restores after DHCP failure and hold'
$g.Scenario='DhcpOutage';$g.MaintainFailuresBeforeRestore=0
Assert ((Get-NetworkGuidance $g $s 20000) -eq '') 'DHCP outage waits for natural maintenance failure'
$g.MaintainFailuresBeforeRestore=1
Assert ((Get-NetworkGuidance $g $s 20000) -eq 'REQUEST_RESTORE') 'DHCP outage instructs restart only after observed failure'
Update-NetworkOutage $s 'PASS' 21000
Assert ((Get-NetworkGuidance $g $s 21000) -eq 'STOP_EARLY_RESTORE') 'premature reconnection rejected'
$g.Scenario='Cable';$s.FaultConfirmed=$false;$s.EarlyRestore=$false
Assert ((Get-NetworkGuidance $g $s 60001) -eq 'STOP_NO_DISCONNECT') 'ignored disconnect instruction has bounded wait'
$x=$m.Clone();$x.Scenario='Cable';$x.GuidanceVersion=2;$x.FaultConfirmed=$true;$x.RestoreRequested=$true;$x.PrematureRestore=$false
Assert ((Get-NetworkVerdict $x).Status -eq 'PASS') 'complete guided cable accepted'
$x.RestoreRequested=$false
Assert ((Get-NetworkVerdict $x).Status -eq 'FAIL') 'missing automatic restore instruction rejected'
$x.RestoreRequested=$true;$x.PrematureRestore=$true
Assert ((Get-NetworkVerdict $x).Status -eq 'FAIL') 'early physical restoration invalidates guided cable'
$file=Join-Path ([IO.Path]::GetTempPath()) ('test07-'+[guid]::NewGuid().ToString('N')+'.json')
$proc=$null
try {
    $fixture=Join-Path $PSScriptRoot 'loopback_fixture.py'
    $proc=Start-Process -FilePath $Python -ArgumentList @(('"'+$fixture+'"'),('"'+$file+'"')) -PassThru -NoNewWindow
    $deadline=[DateTime]::UtcNow.AddSeconds(10)
    while(-not (Test-Path $file)) {if([DateTime]::UtcNow -gt $deadline){throw 'Fixture startup timeout'};Start-Sleep -Milliseconds 50}
    $ports=Get-Content -Raw $file | ConvertFrom-Json
    foreach($pair in @(@(0,'PASS'),@(1,'CORRUPT'),@(2,'LENGTH'),@(3,'SOURCE'),@(4,'TIMEOUT'))) {
        [byte[]]$packet=0..127;$packet[0]=[byte]$pair[0]
        $r=Test-UdpProbe '127.0.0.1' $ports.udp $packet 500
        Assert ($r.Code -eq $pair[1]) "real loopback UDP $($pair[1])"
    }
    $r=Test-TcpProbe '127.0.0.1' $ports.tcp ('X'*64) 1000
    Assert ($r.Code -eq 'PASS') 'real loopback TCP 64 B and EOF'
    $r=Test-TcpProbe '127.0.0.1' $ports.tcp 'BAD' 1000
    Assert ($r.Code -eq 'CORRUPT') 'real loopback TCP wrong reply rejected'
    $r=Test-TcpProbe '127.0.0.1' $ports.tcp 'INCOMPLETE' 1000 -Abort
    Assert ($r.Code -eq 'ABORT_SENT') 'real loopback TCP abort'
} finally {if($proc -and -not $proc.HasExited){$proc.Kill();$proc.WaitForExit()};Remove-Item $file -ErrorAction SilentlyContinue}
Write-Host 'HOST SELF-TEST PASS — synthetic verdicts + loopback only; UNO hardware PENDING.'

#requires -Version 7.0
<# Bounded hardware runner; one COM owner; logs all probes, UART lines and markers.
   D = physically disconnected / DHCP server disabled; R = physically restored.
   Start StartupDhcp with the UNO Ethernet cable already disconnected.
   No automatic packet retry inside a probe. Fresh UDP socket per probe.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SerialPort,
    [ValidateSet('Baseline','Cable','StartupDhcp','DhcpRenew','DhcpOutage','Soak','TcpAbort')]
    [string]$Scenario = 'Baseline',
    [ValidateRange(30,86400)][int]$DurationSeconds = 120,
    [ValidateRange(50,5000)][int]$IntervalMs = 100,
    [ValidateRange(100,5000)][int]$TimeoutMs = 1000,
    [ValidateRange(5,300)][int]$RecoveryLimitSeconds = 45,
    [ValidateRange(5,120)][int]$WarmupSeconds = 30,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'runs'),
    # Optional markers for consoles without KeyAvailable. Append D or R as lines.
    [string]$MarkerFile
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'NetworkRobustness.psm1') -Force
$run = Join-Path $OutputDirectory ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $Scenario + '-' + [guid]::NewGuid().ToString('N').Substring(0,6))
$null = New-Item -ItemType Directory -Path $run -Force
$utf8 = [Text.UTF8Encoding]::new($false)
$events = [IO.StreamWriter]::new((Join-Path $run 'events.jsonl'), $false, $utf8)
$uart = [IO.StreamWriter]::new((Join-Path $run 'serial.log'), $false, $utf8)
$probes = [IO.StreamWriter]::new((Join-Path $run 'probes.csv'), $false, $utf8)
$events.AutoFlush = $true; $uart.AutoFlush = $true; $probes.AutoFlush = $true
$probes.WriteLine('utc,elapsed_ms,phase,protocol,sequence,ip,bytes,result,rtt_ms')
$clock = [Diagnostics.Stopwatch]::StartNew()
$m = [ordered]@{
    Scenario=$Scenario; Completed=$false; DurationSeconds=$DurationSeconds; HealthySeconds=0; Fatal=''; Stats=0
    Boots=0; Firmware=$false; W5100=$false; MinFree=32767; RamDrift=0; Corrupt=0; HealthyFail=0
    HealthyOK=0; TcpOK=0; Boundaries=0; SendFail=0; Drop=0; ManualDhcp=0
    Restored=$false; Recovered=$false; RecoveryMs=-1; RecoveryLimitMs=($RecoveryLimitSeconds*1000)
    PostOK=0; PostTcpOK=0; OutageMs=0; OutageFailures=0; DhcpFailuresBeforeRestore=0
    MaintainFailuresBeforeRestore=0; RenewOK=0; Aborts=0; TcpReject=0
}
$state = @{ Phase=$(if($Scenario -eq 'StartupDhcp'){'OUTAGE'}else{'WARMUP'}); IP='0.0.0.0'; Ready=$false
    Pending=''; MarkersRead=0; CutMs=$(if($Scenario -eq 'StartupDhcp'){0}else{-1}); RestoreMs=-1
    HealthyStart=-1; Streak=0; FirstGoodMs=-1; StableFree=-1; LastBoardMs=-1; Quit=$false }
$serial = $null; $seq = [uint32]0; $tcpSeq=0; $lastTcp=-5000; $lastQuery=-5000
$session = [guid]::NewGuid().ToByteArray()
$boundaries = [Collections.Generic.HashSet[int]]::new()
$healthyLengths = @(1,16,64,128)
$healthyUdpCount=0

function Write-Event([string]$Name, $Data) {
    $events.WriteLine((@{ utc=[DateTime]::UtcNow.ToString('o'); elapsed_ms=$clock.ElapsedMilliseconds; name=$Name; data=$Data } | ConvertTo-Json -Depth 8 -Compress))
}
function Drain-Serial {
    $state.Pending += $serial.ReadExisting()
    if ($state.Pending.Length -gt 65536) { throw 'UART line overflow' }
    while (($lf = $state.Pending.IndexOf("`n")) -ge 0) {
        $line = $state.Pending.Substring(0,$lf).TrimEnd("`r")
        $state.Pending = $state.Pending.Substring($lf+1)
        $uart.WriteLine("$([DateTime]::UtcNow.ToString('o')) $line")
        $obj = ConvertFrom-NetworkLine $line
        if ($null -eq $obj) { continue }
        Write-Event 'UART' $obj
        if ($obj.type -eq 'INFO' -and $obj.test -eq '07' -and $obj.version -eq '0.1') { $m.Firmware=$true }
        if ($obj.PSObject.Properties['ms']) {
            $boardMs = [long]$obj.ms
            if ($state.LastBoardMs -gt $boardMs) { $m.Fatal = 'device uptime went backwards (reset/wrap)' }
            $state.LastBoardMs = $boardMs
        }
        if ($obj.type -eq 'EVT') {
            switch ($obj.name) {
                'BOOT' { $m.Boots++ }
                'HARDWARE' { if ($obj.chip -eq 'W5100') { $m.W5100=$true } }
                'SERVICES_READY' { $state.IP=$obj.ip; $state.Ready=$true }
                'NET' { $state.IP=$obj.ip }
                'DHCP_BEGIN' { $state.Ready=$false }
                'RETRY_SCHEDULED' { $state.Ready=$false }
                'DHCP_FAIL' { if (-not $m.Restored) { $m.DhcpFailuresBeforeRestore++ } }
                'MANUAL_DHCP' { $m.ManualDhcp++ }
                'UDP_SEND_FAIL' { if ($state.Phase -in @('HEALTHY','POST')) { $m.SendFail++ } }
                'TCP_SEND_FAIL' { if ($state.Phase -in @('HEALTHY','POST')) { $m.SendFail++ } }
                'DHCP_MAINTAIN' {
                    if ([int]$obj.rc -eq 2) { $m.RenewOK++ }
                    if ([int]$obj.rc -in @(1,3) -and $state.Phase -eq 'OUTAGE') { $m.MaintainFailuresBeforeRestore++ }
                }
            }
        }
        if ($obj.type -eq 'STAT') {
            Write-Host "UART ms=$($obj.ms) ready=$($obj.ready) IP=$($obj.ip) free=$($obj.free) phase=$($state.Phase)"
            $m.Stats++; $state.Ready=([int]$obj.ready -eq 1); $state.IP=$obj.ip
            $m.MinFree=[Math]::Min($m.MinFree,[int]$obj.min_free)
            if ($state.Phase -in @('HEALTHY','POST')) {
                if ($state.StableFree -lt 0) { $state.StableFree=[int]$obj.free }
                $m.RamDrift=[Math]::Max($m.RamDrift,$state.StableFree-[int]$obj.free)
            }
            $m.Drop=[long]$obj.drop; $m.TcpReject=[long]$obj.tcp_reject
        }
    }
}
function Mark-Event([char]$Key) {
    if ($Key -eq 'Q') { $state.Quit=$true; $m.Fatal='operator aborted run'; return }
    if ($Key -eq 'D' -and $Scenario -in @('Cable','DhcpOutage') -and $state.Phase -eq 'HEALTHY' -and $m.HealthyOK -ge 24 -and $m.TcpOK -ge 3) {
        $state.CutMs=$clock.ElapsedMilliseconds; $state.Phase='OUTAGE'; $state.Streak=0
        $m.HealthySeconds += ($state.CutMs-$state.HealthyStart)/1000
        Write-Event 'OPERATOR_DISCONNECT' @{meaning=$(if($Scenario -eq 'Cable'){'UNO cable removed'}else{'DHCP disabled on isolated test LAN'})}
        Write-Host 'OUTAGE marked. Restore physically, then press R (after >=10 s).'
    } elseif ($Key -eq 'R' -and $state.Phase -eq 'OUTAGE') {
        $state.RestoreMs=$clock.ElapsedMilliseconds; $m.Restored=$true
        $m.OutageMs=$state.RestoreMs-$state.CutMs
        $state.Phase='RECOVERY'; $state.Streak=0; $state.FirstGoodMs=-1
        Write-Event 'OPERATOR_RESTORE' @{outage_ms=$m.OutageMs}
        Write-Host 'Restoration marked; measuring recovery and exact echoes.'
    } elseif ($Key -in @('D','R')) { Write-Host 'Marker ignored: phase/baseline prerequisites not met.' }
}
function Read-Markers {
    if ($MarkerFile) {
        if (Test-Path $MarkerFile) {
            $lines = @(Get-Content -LiteralPath $MarkerFile)
            while ($state.MarkersRead -lt $lines.Count) {
                $key=$lines[$state.MarkersRead].Trim().ToUpperInvariant(); $state.MarkersRead++
                if ($key -in @('D','R','Q')) { Mark-Event $key[0] }
            }
        }
    } elseif (-not [Console]::IsInputRedirected) {
        while ([Console]::KeyAvailable) { Mark-Event ([char]::ToUpperInvariant([Console]::ReadKey($true).KeyChar)) }
    }
}
function Record-Probe([string]$Protocol, [int]$Bytes, $Result, [string]$Phase) {
    $probes.WriteLine(('{0},{1},{2},{3},{4},{5},{6},{7},{8}' -f [DateTime]::UtcNow.ToString('o'),$clock.ElapsedMilliseconds,$Phase,$Protocol,$seq,$state.IP,$Bytes,$Result.Code,$Result.Ms.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)))
    if ($Result.Code -in @('SOURCE','LENGTH','CORRUPT','EXTRA_BYTES','ERROR')) { $m.Corrupt++; Write-Event 'INTEGRITY_ERROR' $Result }
    if ($Phase -in @('HEALTHY','POST')) {
        if ($Result.Code -eq 'PASS') {
            if ($Protocol -eq 'UDP') { $m.HealthyOK++; if($Phase -eq 'POST'){$m.PostOK++} }
            else { $m.TcpOK++; if($Phase -eq 'POST'){$m.PostTcpOK++} }
        } elseif ($Result.Code -ne 'ABORT_SENT') { $m.HealthyFail++; Write-Event 'HEALTHY_FAILURE' $Result }
    }
    if ($Phase -eq 'OUTAGE' -and $Result.Code -ne 'PASS') { $m.OutageFailures++ }
}

try {
    Write-Host "TEST-07 $Scenario / bounded ${DurationSeconds}s / logs: $run"
    Write-Host 'Opening COM with DTR=true may reset UNO; capture one BOOT. Close Arduino monitor.'
    if ($Scenario -eq 'StartupDhcp') { Write-Host 'Cable must already be removed. After DHCP_FAIL and >=10 s, reconnect then press R.' }
    if ($Scenario -in @('Cable','DhcpOutage')) { Write-Host 'After >=24 UDP + 3 TCP baseline checks, perform fault then press D; physically restore then press R.' }
    if ($MarkerFile -and (Test-Path $MarkerFile) -and (Get-Item $MarkerFile).Length -gt 0) { throw 'MarkerFile must be empty at run start' }
    $serial = [IO.Ports.SerialPort]::new($SerialPort,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
    $serial.DtrEnable=$true; $serial.RtsEnable=$false; $serial.WriteTimeout=500; $serial.Open()
    Write-Event 'RUN_BEGIN' @{scenario=$Scenario; duration_s=$DurationSeconds; interval_ms=$IntervalMs; timeout_ms=$TimeoutMs; serial_port=$SerialPort; host_ps=$PSVersionTable.PSVersion.ToString()}
    $nextProbe=0L
    while ($clock.Elapsed.TotalSeconds -lt $DurationSeconds -and -not $state.Quit) {
        Drain-Serial
        Read-Markers
        if ($state.Phase -eq 'WARMUP' -and $clock.Elapsed.TotalSeconds -gt $WarmupSeconds) { throw 'Warmup deadline: no stable service' }
        if ($state.Phase -eq 'RECOVERY' -and $clock.ElapsedMilliseconds-$state.RestoreMs -gt $m.RecoveryLimitMs) { throw 'Recovery deadline exceeded' }
        if ($clock.ElapsedMilliseconds-$lastQuery -ge 5000) { $serial.Write('?'); $lastQuery=$clock.ElapsedMilliseconds }
        if ($clock.ElapsedMilliseconds -lt $nextProbe) { Start-Sleep -Milliseconds 10; continue }
        $seq++
        $phase=$state.Phase
        $length = if($phase -in @('HEALTHY','POST')) { $healthyLengths[$healthyUdpCount % 4] } else { 32 }
        $data=New-ProbePayload $length $seq $session
        $budget=[Math]::Min($TimeoutMs, [Math]::Max(1,[int]($DurationSeconds*1000-$clock.ElapsedMilliseconds)))
        $result = if ($state.IP -eq '0.0.0.0') {
            [pscustomobject]@{Code='NO_IP';Detail='DHCP not ready';Ms=0}
        } else { Test-UdpProbe $state.IP 5001 $data $budget }
        Record-Probe 'UDP' $length $result $phase
        if ($phase -eq 'RECOVERY' -and $clock.ElapsedMilliseconds-$state.RestoreMs -gt $m.RecoveryLimitMs) { throw 'Recovery confirmation deadline exceeded' }
        if ($phase -eq 'WARMUP' -and $clock.Elapsed.TotalSeconds -gt $WarmupSeconds) { throw 'Warmup confirmation deadline exceeded' }
        if ($phase -in @('HEALTHY','POST')) {
            $healthyUdpCount++
            if ($result.Code -eq 'PASS') { $null=$boundaries.Add($length); $m.Boundaries=$boundaries.Count }
        }
        if ($phase -in @('WARMUP','RECOVERY')) {
            if ($result.Code -eq 'PASS' -and $state.Ready) {
                if ($state.Streak -eq 0) { $state.FirstGoodMs=$clock.ElapsedMilliseconds }
                $state.Streak++
                if ($state.Streak -ge 3) {
                    if ($phase -eq 'RECOVERY') {
                        $m.Recovered=$true; $m.RecoveryMs=$state.FirstGoodMs-$state.RestoreMs; $state.Phase='POST'
                        Write-Event 'RECOVERED' @{first_exact_echo_ms=$m.RecoveryMs; confirmation_echoes=3}
                    } else { $state.Phase='HEALTHY' }
                    $state.HealthyStart=$clock.ElapsedMilliseconds
                    Write-Host "Service verified / IP=$($state.IP) / phase=$($state.Phase)"
                }
            } else { $state.Streak=0 }
        }
        if ($state.Phase -in @('HEALTHY','POST') -and $clock.ElapsedMilliseconds-$lastTcp -ge 5000 -and $clock.Elapsed.TotalSeconds -lt ($DurationSeconds-2)) {
            $tcpSeq++; $lastTcp=$clock.ElapsedMilliseconds
            $payload = if($tcpSeq % 3 -eq 0){'X'*64}else{"T07-$($session[0])-$tcpSeq"}
            $tcpResult=Test-TcpProbe $state.IP 5000 $payload $TimeoutMs
            Record-Probe 'TCP' $payload.Length $tcpResult $state.Phase
            if ($Scenario -eq 'TcpAbort' -and $m.Aborts -lt 5) {
                $abort=Test-TcpProbe $state.IP 5000 'INCOMPLETE' $TimeoutMs -Abort
                Record-Probe 'TCP_ABORT' 10 $abort $state.Phase
                if ($abort.Code -eq 'ABORT_SENT') { $m.Aborts++ }
            }
        }
        $nextProbe=$clock.ElapsedMilliseconds+$IntervalMs
    }
    Drain-Serial
    $m.Completed=($clock.Elapsed.TotalSeconds -ge $DurationSeconds -and -not $state.Quit)
} catch { $m.Fatal=$_.Exception.Message; Write-Event 'RUNNER_ERROR' $m.Fatal; Write-Warning $m.Fatal }
finally {
    if ($state.Phase -in @('HEALTHY','POST')) { $m.HealthySeconds += ($clock.ElapsedMilliseconds-$state.HealthyStart)/1000 }
    if ($serial) { if($serial.IsOpen){$serial.Close()}; $serial.Dispose() }
    $verdict=Get-NetworkVerdict $m
    $hashes=[ordered]@{}
    foreach($name in @('TEST-07_Network_Robustness.ino','NetworkRobustness.psm1','Test-NetworkRobustness.ps1')) {
        $hashes[$name]=(Get-FileHash (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
    }
    $summary=[ordered]@{ test='TEST-07'; scenario=$Scenario; status=$verdict.Status; reasons=$verdict.Reasons
        serial_port=$SerialPort; source_hashes=$hashes
        actual_duration_s=$clock.Elapsed.TotalSeconds; metrics=$m
        scope='Single scenario on supplied hardware. FULL PASS requires all seven scenarios plus build memory gate.' }
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'summary.json') -Encoding utf8
    Write-Event 'RUN_END' $summary
    $events.Dispose(); $uart.Dispose(); $probes.Dispose()
}
Write-Host "RESULT $($verdict.Status) / $Scenario / $run"
foreach ($reason in $verdict.Reasons) { Write-Host "  $reason" }
if ($verdict.Status -ne 'PASS') { exit 1 }

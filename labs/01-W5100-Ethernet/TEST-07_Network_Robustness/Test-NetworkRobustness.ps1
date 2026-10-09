#requires -Version 7.0
<# Bounded hardware runner; one COM owner; guided physical faults without D/R.
   Follow explicit console instructions; loss/return detected with exact UDP probes.
   StartupDhcp confirms cable removal before opening COM and starting the run clock.
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
    # Optional preparation/cancel file: READY for StartupDhcp, Q to cancel.
    [string]$MarkerFile
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'NetworkRobustness.psm1') -Force
if ($MarkerFile -and (Test-Path $MarkerFile) -and (Get-Item $MarkerFile).Length -gt 0) { throw 'MarkerFile must be empty at run start' }
$preparationSeconds=0
if ($Scenario -eq 'StartupDhcp') { $preparationSeconds=Wait-NetworkPreparation -MarkerFile $MarkerFile }
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
    GuidanceVersion=2; FaultConfirmed=$false; RestoreRequested=$false; PrematureRestore=$false
}
$state = @{ Phase=$(if($Scenario -eq 'StartupDhcp'){'OUTAGE'}else{'WARMUP'}); IP='0.0.0.0'; Ready=$false
    Pending=''; MarkersRead=0; CutMs=$(if($Scenario -eq 'StartupDhcp'){0}else{-1}); RestoreMs=-1
    HealthyStart=-1; Streak=0; FirstGoodMs=-1; StableFree=-1; LastBoardMs=-1; Quit=$false
    FaultRequestedMs=-1; FirstFailureMs=-1; FailureStreak=0; FaultConfirmed=$false; EarlyRestore=$false }
$serial = $null; $seq = [uint32]0; $tcpSeq=0; $lastTcp=-5000; $lastQuery=-5000
$session = [guid]::NewGuid().ToByteArray()
$boundaries = [Collections.Generic.HashSet[int]]::new()
$healthyLengths = @(1,16,64,128)
$healthyUdpCount=0
$lastProgress=-5000

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
                'DHCP_FAIL' { if (-not $m.RestoreRequested) { $m.DhcpFailuresBeforeRestore++ } }
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
    if ($Key -in @('D','R')) { Write-Host 'D/R не нужны. Выполняйте только текущую инструкцию скрипта.' }
}

function Apply-Guidance {
    $action=Get-NetworkGuidance $m $state $clock.ElapsedMilliseconds
    switch ($action) {
        'STOP_BASELINE' { throw 'Исходное соединение содержит ошибки; отключать кабель/DHCP не нужно. Смотрите probes.csv.' }
        'STOP_INTEGRITY' { throw 'Ошибка целостности во время отказа; сценарий не может получить PASS.' }
        'STOP_NO_DISCONNECT' { throw 'За 60 s после приглашения устойчивый разрыв не обнаружен. Действие не подтверждено.' }
        'STOP_EARLY_RESTORE' { $m.PrematureRestore=$true; throw 'Обмен вернулся до приглашения восстановить соединение; повторите, следуя подсказкам.' }
        { $_ -in @('REQUEST_DISCONNECT','REQUEST_DHCP_STOP') } {
            $state.FaultRequestedMs=$clock.ElapsedMilliseconds; $state.Phase='OUTAGE'; $state.Streak=0
            $m.HealthySeconds += ($state.FaultRequestedMs-$state.HealthyStart)/1000
            Write-Event $action @{baseline_udp=$m.HealthyOK;baseline_tcp=$m.TcpOK}
            if ($action -eq 'REQUEST_DISCONNECT') { Write-Host 'ШАГ 2: СЕЙЧАС ВЫНЬТЕ Ethernet-кабель из UNO. USB оставьте. Клавиши не нужны.' -ForegroundColor Yellow }
            else { Write-Host 'ШАГ 2: СЕЙЧАС ОТКЛЮЧИТЕ только DHCP-сервер в тестовой сети. LAN оставьте включённой.' -ForegroundColor Yellow }
            Write-Host 'Скрипт сам подтвердит отказ и отсчитает выдержку. Подключайте только по следующему приглашению.'
        }
        'REQUEST_RESTORE' {
            $state.RestoreMs=$clock.ElapsedMilliseconds; $m.RestoreRequested=$true
            $m.OutageMs=$state.RestoreMs-$state.CutMs; $state.Phase='RECOVERY'; $state.Streak=0; $state.FirstGoodMs=-1
            Write-Event 'RESTORE_REQUESTED' @{outage_ms=$m.OutageMs;timing_origin='script_prompt';limit_ms=$m.RecoveryLimitMs}
            if ($Scenario -eq 'DhcpOutage') { Write-Host 'ШАГ 3: СЕЙЧАС ВКЛЮЧИТЕ DHCP-сервер. Клавиши не нужны.' -ForegroundColor Yellow }
            else { Write-Host 'ШАГ 3: СЕЙЧАС ПОДКЛЮЧИТЕ Ethernet-кабель к UNO. Клавиши не нужны.' -ForegroundColor Yellow }
            Write-Host "Окно ${RecoveryLimitSeconds} s включает время вашей реакции. Ожидаю три точных UDP-ответа."
        }
    }
}
function Read-Markers {
    if ($MarkerFile) {
        if (Test-Path $MarkerFile) {
            $lines = @(Get-Content -LiteralPath $MarkerFile)
            while ($state.MarkersRead -lt $lines.Count) {
                $key=$lines[$state.MarkersRead].Trim().ToUpperInvariant(); $state.MarkersRead++
                if ($key -eq 'Q') { Mark-Event $key[0] }
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
        } elseif ($Result.Code -ne 'ABORT_SENT') {
            $m.HealthyFail++; Write-Event 'HEALTHY_FAILURE' @{phase=$Phase;protocol=$Protocol;bytes=$Bytes;result=$Result}
            Write-Host "ОШИБКА: phase=$Phase / $Protocol / $Bytes B / $($Result.Code) / $($Result.Detail)" -ForegroundColor Red
        }
    }
    if ($Phase -eq 'OUTAGE' -and $Result.Code -ne 'PASS') { $m.OutageFailures++ }
}

try {
    Write-Host "TEST-07 $Scenario / bounded ${DurationSeconds}s / logs: $run"
    Write-Host 'Opening COM with DTR=true may reset UNO; capture one BOOT. Close Arduino monitor.'
    if ($Scenario -eq 'StartupDhcp') { Write-Host 'Кабель отключён по подтверждению оператора. Ожидаю DHCP_FAIL; подключать только по приглашению.' }
    elseif ($Scenario -in @('Cable','DhcpOutage')) { Write-Host 'ШАГ 1: ОСТАВЬТЕ соединение исправным. Проверяю UDP/TCP; отключать только по приглашению.' -ForegroundColor Cyan }
    else { Write-Host 'Автоматический сценарий: кабель оставьте подключённым, действий не требуется.' }
    $serial = [IO.Ports.SerialPort]::new($SerialPort,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
    $serial.DtrEnable=$true; $serial.RtsEnable=$false; $serial.WriteTimeout=500; $serial.Open()
    Write-Event 'RUN_BEGIN' @{scenario=$Scenario; duration_s=$DurationSeconds; interval_ms=$IntervalMs; timeout_ms=$TimeoutMs; serial_port=$SerialPort; host_ps=$PSVersionTable.PSVersion.ToString()}
    $nextProbe=0L
    while ($clock.Elapsed.TotalSeconds -lt $DurationSeconds -and -not $state.Quit) {
        Drain-Serial
        Read-Markers
        Apply-Guidance
        if ($clock.ElapsedMilliseconds-$lastProgress -ge 5000) {
            $lastProgress=$clock.ElapsedMilliseconds
            $hold=if($state.FaultConfirmed -and $state.Phase -eq 'OUTAGE'){[Math]::Max(0,[Math]::Ceiling((15000-($clock.ElapsedMilliseconds-$state.CutMs))/1000))}else{0}
            Write-Host "ПРОГРЕСС: phase=$($state.Phase) / UDP=$($m.HealthyOK) / TCP=$($m.TcpOK) / errors=$($m.HealthyFail) / outage_fail=$($m.OutageFailures) / выдержка=${hold}s / осталось=$([Math]::Ceiling($DurationSeconds-$clock.Elapsed.TotalSeconds))s"
        }
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
        if ($phase -eq 'OUTAGE') {
            $wasConfirmed=$state.FaultConfirmed
            Update-NetworkOutage $state $result.Code $clock.ElapsedMilliseconds
            if (-not $wasConfirmed -and $state.FaultConfirmed) {
                $m.FaultConfirmed=$true
                Write-Event 'OUTAGE_CONFIRMED' @{first_failed_echo_ms=$state.CutMs;consecutive_failures=$state.FailureStreak}
                Write-Host 'Отказ подтверждён тремя последовательными неудачными пробами. Идёт автоматическая выдержка; пока не подключайте.' -ForegroundColor Cyan
            }
        }
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
                        $m.Restored=$true; $m.Recovered=$true; $m.RecoveryMs=$state.FirstGoodMs-$state.RestoreMs; $state.Phase='POST'
                        Write-Event 'RECOVERED' @{first_exact_echo_ms=$m.RecoveryMs; confirmation_echoes=3;timing_origin='script_prompt'}
                        Write-Host "ШАГ 4: ОБМЕН ВОССТАНОВЛЕН / от приглашения: $($m.RecoveryMs) ms. Оставьте кабель подключённым; идёт итоговая проверка UDP/TCP." -ForegroundColor Green
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
        runner_version='0.2';preparation_s=$preparationSeconds
        recovery_timing_origin=$(if($Scenario -in @('Cable','StartupDhcp','DhcpOutage')){'script_restore_prompt_including_operator_reaction'}else{'not_applicable'})
        scope='Single scenario on supplied hardware. Guided recovery includes operator reaction; no PHY timing claim. FULL PASS requires all seven scenarios plus build memory gate.' }
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'summary.json') -Encoding utf8
    Write-Event 'RUN_END' $summary
    $events.Dispose(); $uart.Dispose(); $probes.Dispose()
}
Write-Host "RESULT $($verdict.Status) / $Scenario / $run"
foreach ($reason in $verdict.Reasons) { Write-Host "  $reason" }
if ($verdict.Status -ne 'PASS') { exit 1 }

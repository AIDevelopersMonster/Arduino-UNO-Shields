#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$SerialPort,
      [ValidateRange(30,600)][int]$DurationSeconds=60,
      [ValidateRange(500,3000)][int]$TimeoutMs=2000,
      [string]$BuildSummary=(Join-Path $PSScriptRoot '../../../build/TEST09/build-summary.json'),
      [string]$LogDirectory=(Join-Path $PSScriptRoot 'runs'))
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'EthernetSD.psm1') -Force
$build=Get-Content -Raw -LiteralPath $BuildSummary | ConvertFrom-Json
if($build.test -ne 'TEST-09' -or $build.type -ne 'BUILD_ONLY' -or $build.status -ne 'PASS' -or
   $build.flash_bytes -le 0 -or $build.flash_bytes -gt 29000 -or $build.sram_static_bytes -le 0 -or
   $build.sram_static_bytes -gt 1536 -or $build.core -notin @('1.8.6','1.8.8') -or $build.sd -ne '1.3.0' -or $build.ethernet -ne '2.0.2'){
    throw 'A measured PASS build with AVR 1.8.6/1.8.8, SD 1.3.0 and Ethernet 2.0.2 is required'
}
$hashes=[ordered]@{}
foreach($name in @('TEST-09_Ethernet_SD_Integration.ino','EthernetSD.psm1','Test-EthernetSD.ps1')){
    $hashes[$name]=(Get-FileHash (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
    if($hashes[$name] -ne $build.source_hashes.$name){throw "Source differs from measured build: $name; rebuild TEST-09"}
}
$token=[Guid]::NewGuid().ToString('N').Substring(0,8).ToUpperInvariant()
$run=Join-Path ([IO.Path]::GetFullPath($LogDirectory)) ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-TEST09-'+$token)
$null=New-Item -ItemType Directory -Path $run -Force
$lines=[Collections.Generic.List[string]]::new()
$probes=[Collections.Generic.List[object]]::new()
$uart=[IO.StreamWriter]::new((Join-Path $run 'serial.log'),$false,[Text.UTF8Encoding]::new($false))
$csv=[IO.StreamWriter]::new((Join-Path $run 'probes.csv'),$false,[Text.UTF8Encoding]::new($false))
$csv.WriteLine('utc,sequence,bytes,crc16,result,rtt_ms')
$clock=[Diagnostics.Stopwatch]::StartNew();$load=[Diagnostics.Stopwatch]::new()
$state=@{pending='';sent=$false;finished=$false;ip='';fatal=''}
$port=[IO.Ports.SerialPort]::new($SerialPort,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
$port.DtrEnable=$true;$port.RtsEnable=$false;$port.WriteTimeout=1000;$port.NewLine="`n"
function Read-Uart {
    $state.pending+=$port.ReadExisting()
    if($state.pending.Length -gt 65536){throw 'UART line overflow'}
    while(($lf=$state.pending.IndexOf("`n")) -ge 0){
        $line=$state.pending.Substring(0,$lf).TrimEnd("`r");$state.pending=$state.pending.Substring($lf+1)
        if(-not $line){continue}
        $lines.Add($line);$uart.WriteLine([DateTime]::UtcNow.ToString('o')+' '+$line);$uart.Flush()
        if($line -match '(?:^| )status=PASS(?: |$)'){Write-Host $line -ForegroundColor Green}
            elseif($line -match '(?:^| )status=FAIL(?: |$)' -or $line -match '^FAIL '){Write-Host $line -ForegroundColor Red}
            else{Write-Host $line}
        if($line -ceq 'READY' -and -not $state.sent){
            if(@($lines | Where-Object {$_ -ceq 'BOOT test=TEST09 fw=0.1 eth_cs=10 sd_cs=4 uart=115200'}).Count -ne 1){throw 'Expected one TEST09 fw=0.1 BOOT before READY; rebuild/upload'}
            $port.WriteLine("RUN $token $DurationSeconds");$state.sent=$true
        }
        $row=ConvertFrom-Test09Line $line
        if($null -eq $row){continue}
        if($row.kind -eq 'NET' -and $row.token -ceq $token){$state.ip=$row.ip}
        if($row.kind -eq 'FAIL'){$state.fatal='Device failed; see serial.log'}
        if($row.kind -eq 'RESULT'){$state.finished=$true}
    }
}
Write-Host "TEST-09 Ethernet + SD / load ${DurationSeconds}s / logs: $run"
Write-Host 'Оставь карту и Ethernet-кабель подключёнными. Закрой монитор Arduino. Клавиши не нужны.'
try{
    $port.Open()
    while(-not $state.ip -and -not $state.finished -and $clock.Elapsed.TotalSeconds -lt 30){Read-Uart;Start-Sleep -Milliseconds 20}
    if(-not $state.ip -or $state.finished){throw 'SD/DHCP preparation failed or exceeded 30 s; see serial.log'}
    $parsedIP=$null
    if(-not [Net.IPAddress]::TryParse($state.ip,[ref]$parsedIP) -or $parsedIP.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or $parsedIP.Equals([Net.IPAddress]::Any)){throw 'Invalid DHCP IPv4 address'}
    Write-Host "ШАГ: IP=$($state.ip). Каждый UDP-пакет записывается на SD и читается после сброса кэша. Нагрузка завершится автоматически."
    $load.Start();$sizes=@(16,32,64,128);$sequence=[uint32]0;$nextProgress=5
    while($load.Elapsed.TotalSeconds -lt $DurationSeconds -and -not $state.finished){
        Read-Uart;if($state.finished){break}
        $sequence++;$length=$sizes[($sequence-1)%4]
        [byte[]]$data=New-Test09Payload -Length $length -Sequence $sequence -Token $token
        $crc=Get-Test09Crc $data
        $probe=Invoke-Test09Udp -IP $state.ip -Port 5001 -Data $data -TimeoutMs $TimeoutMs
        $row=[pscustomobject]@{sequence=$sequence;bytes=$length;crc16=$crc;code=$probe.Code;rtt_ms=$probe.Ms;detail=$probe.Detail}
        $probes.Add($row)
        $csv.WriteLine(('{0},{1},{2},{3},{4},{5}' -f [DateTime]::UtcNow.ToString('o'),$sequence,$length,$crc,$probe.Code,$probe.Ms.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)));$csv.Flush()
        if($probe.Code -ne 'PASS'){$state.fatal="UDP/SD echo failed: $($probe.Code) / $($probe.Detail)";break}
        Read-Uart
        if($load.Elapsed.TotalSeconds -ge $nextProgress){Write-Host ('Проверено {0} пакетов / осталось {1:N0}s' -f $sequence,($DurationSeconds-$load.Elapsed.TotalSeconds));$nextProgress+=5}
        # At most about 10 probes/s; no retries, floods or unbounded file growth.
        Start-Sleep -Milliseconds 100
    }
    $load.Stop()
    if(-not $state.finished){
        $port.WriteLine("STOP $token");$end=[Diagnostics.Stopwatch]::StartNew()
        while(-not $state.finished -and $end.Elapsed.TotalSeconds -lt 10){Read-Uart;Start-Sleep -Milliseconds 20}
        if(-not $state.finished){throw 'Terminal RESULT deadline exceeded'}
    }
}catch{$state.fatal=$_.Exception.Message}
finally{
    $load.Stop();$clock.Stop()
    if($port.IsOpen){$port.Close()};$port.Dispose();$uart.Dispose();$csv.Dispose()
}
try{$verdict=Get-Test09Verdict -Lines $lines.ToArray() -Token $token -DurationSeconds $DurationSeconds -Probes $probes.ToArray() -LoadSeconds $load.Elapsed.TotalSeconds -Fatal $state.fatal}
catch{$verdict=[pscustomobject]@{status='FAIL';reasons=@('incomplete/malformed evidence: '+$_.Exception.Message);metrics=$null}}
$summary=[ordered]@{test='TEST-09';type='HARDWARE';runner_version='0.1';status=$verdict.status;utc=[DateTime]::UtcNow.ToString('o');
    port=$SerialPort;token=$token;requested_load_s=$DurationSeconds;actual_load_s=$load.Elapsed.TotalSeconds;total_ms=$clock.ElapsedMilliseconds;
    reasons=$verdict.reasons;metrics=$verdict.metrics;source_hashes=$hashes;build=$build;test_file='T09CHECK.BIN';run_directory=$run}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $run 'summary.json') -Encoding utf8
Write-Host "RESULT $($summary.status) / TEST-09 / $run" -ForegroundColor $(if($summary.status -eq "PASS"){"Green"}else{"Red"})
foreach($reason in $summary.reasons){Write-Host "  $reason" -ForegroundColor Red}
if($summary.status -ne 'PASS'){Write-Host 'T09CHECK.BIN may remain for diagnosis. An existing name is never overwritten.';exit 1}
exit 0

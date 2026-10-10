#requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$SerialPort,
      [ValidateRange(15,300)][int]$DurationSeconds=90,
      [string]$BuildSummary=(Join-Path $PSScriptRoot '../../../build/TEST08/build-summary.json'),
      [string]$LogDirectory=(Join-Path $PSScriptRoot 'runs'))
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'MicroSDHardware.psm1') -Force
$build=Get-Content -Raw -LiteralPath $BuildSummary | ConvertFrom-Json
if($build.test -ne 'TEST-08' -or $build.type -ne 'BUILD_ONLY' -or $build.status -ne 'PASS' -or
   $build.flash_bytes -le 0 -or $build.flash_bytes -gt 29000 -or $build.sram_static_bytes -le 0 -or
   $build.sram_static_bytes -gt 1200 -or $build.core -notin @('1.8.6','1.8.8') -or $build.sd -ne '1.3.0'){
    throw 'A valid TEST-08 build summary is required before opening COM'
}
$hashes=[ordered]@{}
foreach($name in @('TEST-08_microSD_Hardware.ino','MicroSDHardware.psm1','Test-MicroSDHardware.ps1')){
    $hashes[$name]=(Get-FileHash (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
    if($hashes[$name] -ne $build.source_hashes.$name){throw "Source differs from measured build: $name; rebuild TEST-08"}
}
$token=[Guid]::NewGuid().ToString('N').Substring(0,8).ToUpperInvariant()
$run=Join-Path ([IO.Path]::GetFullPath($LogDirectory)) ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-TEST08-'+$token)
$null=New-Item -ItemType Directory -Path $run -Force
$lines=[Collections.Generic.List[string]]::new()
$errors=[Collections.Generic.List[string]]::new()
$watch=[Diagnostics.Stopwatch]::StartNew()
$port=[IO.Ports.SerialPort]::new($SerialPort,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
$port.DtrEnable=$true; $port.RtsEnable=$false; $port.WriteTimeout=1000; $port.NewLine="`n"
$log=[IO.StreamWriter]::new((Join-Path $run 'serial.log'),$false,[Text.UTF8Encoding]::new($false))
$pending='';$sent=$false;$finished=$false;$nextProgress=5
Write-Host "TEST-08 microSD / bounded ${DurationSeconds}s / logs: $run"
Write-Host 'Close Arduino monitor. COM opening may reset UNO. Keep the card inserted; no key presses are needed.'
try{
    $port.Open()
    while($watch.Elapsed.TotalSeconds -lt $DurationSeconds -and -not $finished){
        $pending+=$port.ReadExisting()
        while(($newline=$pending.IndexOf("`n")) -ge 0){
            $line=$pending.Substring(0,$newline).TrimEnd("`r")
            $pending=$pending.Substring($newline+1)
            if(-not $line){continue}
            $lines.Add($line)
            $log.WriteLine([DateTime]::UtcNow.ToString('o')+' '+$line);$log.Flush()
            if($line -match '(?:^| )status=PASS(?: |$)'){Write-Host $line -ForegroundColor Green}
            elseif($line -match '(?:^| )status=FAIL(?: |$)' -or $line -match '^FAIL '){Write-Host $line -ForegroundColor Red}
            else{Write-Host $line}
            if($line -ceq 'READY' -and -not $sent){
                $boots=@($lines | Where-Object {$_ -ceq 'BOOT test=TEST08 fw=0.2 eth_cs=10 sd_cs=4 uart=115200'})
                if($boots.Count -ne 1){throw 'Expected one complete TEST08 fw=0.2 BOOT before READY; rebuild/upload if needed'}
                $port.WriteLine("RUN $token");$sent=$true
                Write-Host 'ШАГ: автоматически проверяю SPI, карту, запись, чтение, append, remount и удаление тестового файла.'
            }
            if($line -match '^RESULT '){$finished=$true}
        }
        if($watch.Elapsed.TotalSeconds -ge $nextProgress){
            Write-Host ('Ожидание / {0:N0}s осталось' -f ($DurationSeconds-$watch.Elapsed.TotalSeconds))
            $nextProgress+=5
        }
        if(-not $finished){Start-Sleep -Milliseconds 20}
    }
    if(-not $finished){$errors.Add('bounded run expired without terminal RESULT')}
}catch{$errors.Add($_.Exception.Message)}
finally{
    if($port.IsOpen){$port.Close()};$port.Dispose();$watch.Stop();$log.Dispose()
}
try{$verdict=Get-Test08Verdict -Lines $lines.ToArray() -Token $token}
catch{$verdict=[pscustomobject]@{status='FAIL';reasons=@('malformed/incomplete UART evidence: '+$_.Exception.Message);metrics=$null}}
foreach($reason in $verdict.reasons){$errors.Add($reason)}
$passed=$verdict.status -eq 'PASS' -and $errors.Count -eq 0
$summary=[ordered]@{test='TEST-08';type='HARDWARE';runner_version='0.2';status=$(if($passed){'PASS'}else{'FAIL'});
    utc=[DateTime]::UtcNow.ToString('o');port=$SerialPort;token=$token;requested_duration_s=$DurationSeconds;
    completed_duration_ms=$watch.ElapsedMilliseconds;reasons=@($errors | Select-Object -Unique);metrics=$verdict.metrics;
    source_hashes=$hashes;build=$build;test_file='T08CHECK.BIN';run_directory=$run}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $run 'summary.json') -Encoding utf8
Write-Host "RESULT $($summary.status) / TEST-08 / $run" -ForegroundColor $(if($summary.status -eq "PASS"){"Green"}else{"Red"})
foreach($reason in $summary.reasons){Write-Host "  $reason" -ForegroundColor Red}
if(-not $passed){Write-Host 'A failed run may leave T08CHECK.BIN for diagnosis; existing files are never overwritten.';exit 1}
exit 0

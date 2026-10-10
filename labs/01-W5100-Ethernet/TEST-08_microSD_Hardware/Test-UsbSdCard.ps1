#requires -Version 7.0
[CmdletBinding()]
param([ValidatePattern('^[A-Za-z]$')][string]$DriveLetter='F',
      [ValidateRange(0,128)][int]$ExpectedDiskNumber=2,
      [ValidateRange(1,64)][int]$SizeMiB=16,
      [ValidateRange(10,300)][int]$PhaseTimeoutSeconds=60,
      [string]$LogDirectory=(Join-Path $PSScriptRoot 'runs'))
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if(-not $IsWindows){throw 'Run the USB card procedure in PowerShell 7 on Windows.'}
$DriveLetter=$DriveLetter.ToUpperInvariant()
$id=[Guid]::NewGuid().ToString('N').Substring(0,12).ToUpperInvariant()
$run=Join-Path ([IO.Path]::GetFullPath($LogDirectory)) ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-USBSD-'+$id)
$null=New-Item -ItemType Directory -Path $run -Force
$transcript=Join-Path $run 'console.log'
$null=Start-Transcript -LiteralPath $transcript
$summary=[ordered]@{test='USB-SD';runner_version='0.1';type='USB_CARD_HARDWARE';status='FAIL';
    utc=[DateTime]::UtcNow.ToString('o');requested_bytes=([long]$SizeMiB*1MB);phase_timeout_s=$PhaseTimeoutSeconds;
    run_directory=$run;test_file=('USBSD_'+$id+'.BIN');seed=$null;volume=$null;
    write=$null;immediate_read=$null;reconnected_read=$null;disconnect_observed=$false;
    cleanup=$false;failure_stage=$null;reason=$null;source_hashes=[ordered]@{}}
$stage='PREPARATION';$path=$null;$ownsFile=$false
function Get-CardTarget([string]$Letter) {
    $volume=Get-Volume -DriveLetter $Letter
    $partition=@(Get-Partition -DriveLetter $Letter)
    if($partition.Count -ne 1){throw 'Expected exactly one partition for the selected letter.'}
    $disk=Get-Disk -Number $partition[0].DiskNumber
    if($disk.BusType -ne 'USB' -or $disk.IsOffline -or $disk.IsReadOnly){throw 'Expected an online writable USB disk.'}
    if($volume.FileSystem -notin @('FAT','FAT16','FAT32')){throw 'Expected FAT16/FAT32 for this card procedure.'}
    if(-not $volume.UniqueId){throw 'Volume identity is unavailable.'}
    [pscustomobject]@{Letter=$Letter;UniqueId=[string]$volume.UniqueId;DiskNumber=[int]$disk.Number;
        DiskName=[string]$disk.FriendlyName;DiskSerial=[string]$disk.SerialNumber;
        FileSystem=[string]$volume.FileSystem;Size=[long]$volume.Size;Free=[long]$volume.SizeRemaining}
}
function Assert-SameVolume($Target,$Initial) {
    if($Target.UniqueId -cne $Initial.UniqueId -or $Target.Size -ne $Initial.Size -or
       $Target.FileSystem -cne $Initial.FileSystem -or $Target.DiskSerial -cne $Initial.DiskSerial){
        throw 'The selected volume is different; the test will not access it.'
    }
}
function Convert-Measurement($Value) {
    [ordered]@{bytes=$Value.Bytes;sha256=$Value.Sha256;elapsed_ms=$Value.ElapsedMs}
}
try {
    foreach($name in @('Test-UsbSdCard.ps1','UsbSdProbe.cs')){
        $summary.source_hashes[$name]=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
    }
    if(-not ('ArduinoUsbSdProbe' -as [type])){Add-Type -Path (Join-Path $PSScriptRoot 'UsbSdProbe.cs')}
    if([ArduinoUsbSdProbe]::Version -ne '0.1'){throw 'Restart PowerShell to load the expected helper version.'}
    $initial=Get-CardTarget $DriveLetter
    if($initial.DiskNumber -ne $ExpectedDiskNumber){throw "Expected USB disk $ExpectedDiskNumber, found $($initial.DiskNumber)."}
    if($initial.Free -lt $summary.requested_bytes+1MB){throw 'Insufficient free space for the test file.'}
    $summary.volume=$initial
    $summary.seed=[Security.Cryptography.RandomNumberGenerator]::GetInt32(1,[int]::MaxValue)
    $path=Join-Path ($DriveLetter+':\') $summary.test_file
    Write-Host "`nUSB microSD / $DriveLetter`: / USB disk $($initial.DiskNumber) / $($initial.FileSystem) / $SizeMiB MiB" -ForegroundColor Cyan
    Write-Host "Логи: $run"
    Write-Host "Создам отдельный файл $($summary.test_file), запишу $SizeMiB MiB и проверю каждый байт и SHA-256."
    $null=Read-Host 'Оставь картридер подключённым, закрой открытые на карте файлы. Нажми Enter для начала'
    Assert-SameVolume (Get-CardTarget $DriveLetter) $initial

    $stage='WRITE'
    Write-Host "`nШАГ 1. Запись и сброс буферов на накопитель." -ForegroundColor Cyan
    $summary.write=Convert-Measurement ([ArduinoUsbSdProbe]::Write($path,$summary.requested_bytes,$summary.seed,$PhaseTimeoutSeconds))
    $ownsFile=$true
    Write-Host "WRITE PASS / bytes=$($summary.write.bytes) / ms=$($summary.write.elapsed_ms)" -ForegroundColor Green

    $stage='IMMEDIATE_READ'
    Write-Host "`nШАГ 2. Файл закрыт. Открываю заново и сравниваю данные." -ForegroundColor Cyan
    $summary.immediate_read=Convert-Measurement ([ArduinoUsbSdProbe]::Verify($path,$summary.requested_bytes,$summary.seed,$PhaseTimeoutSeconds))
    if($summary.immediate_read.sha256 -cne $summary.write.sha256){throw 'Immediate SHA-256 differs.'}
    Write-Host "READ PASS / SHA256=$($summary.immediate_read.sha256)" -ForegroundColor Green
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'summary.json') -Encoding utf8

    $stage='DISCONNECT'
    Write-Host "`nШАГ 3. Проверка сохранности после переподключения." -ForegroundColor Cyan
    Write-Host 'Все файлы скрипта закрыты. Выполни Безопасное извлечение картридера в Windows, затем отключи его USB.'
    $null=Read-Host 'Оставь картридер отключённым и нажми Enter'
    $stillPresent=@(Get-Volume | Where-Object {$_.UniqueId -ceq $initial.UniqueId -and $_.DriveLetter})
    if($stillPresent.Count){throw 'The card volume is still mounted; disconnection was not confirmed.'}
    $summary.disconnect_observed=$true

    $stage='RECONNECT'
    $null=Read-Host 'Теперь подключи тот же картридер с той же картой обратно. Дождись появления диска в Windows и нажми Enter'
    $candidates=@(Get-Volume | Where-Object {$_.UniqueId -ceq $initial.UniqueId -and $_.DriveLetter})
    if($candidates.Count -ne 1){throw 'The same card volume was not found after reconnection.'}
    $current=Get-CardTarget ([string]$candidates[0].DriveLetter)
    Assert-SameVolume $current $initial
    $path=Join-Path ($current.Letter+':\') $summary.test_file
    $summary.reconnected_drive=$current.Letter

    $stage='RECONNECTED_READ'
    Write-Host "`nШАГ 4. Повторное чтение с $($current.Letter): после переподключения." -ForegroundColor Cyan
    $summary.reconnected_read=Convert-Measurement ([ArduinoUsbSdProbe]::Verify($path,$summary.requested_bytes,$summary.seed,$PhaseTimeoutSeconds))
    if($summary.reconnected_read.sha256 -cne $summary.write.sha256){throw 'Reconnected SHA-256 differs.'}
    Write-Host "RECONNECTED READ PASS / SHA256=$($summary.reconnected_read.sha256)" -ForegroundColor Green

    $stage='CLEANUP'
    Assert-SameVolume (Get-CardTarget $current.Letter) $initial
    if(-not $ownsFile){throw 'Test file ownership was not established.'}
    Remove-Item -LiteralPath $path
    if(Test-Path -LiteralPath $path){throw 'Test file cleanup failed.'}
    $summary.cleanup=$true
    $summary.status='PASS'
} catch {
    $summary.failure_stage=$stage
    $summary.reason=$_.Exception.Message
} finally {
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'summary.json') -Encoding utf8
    Write-Host "`nRESULT $($summary.status) / USB-SD / $run" -ForegroundColor $(if($summary.status -eq 'PASS'){'Green'}else{'Red'})
    if($summary.reason){Write-Host "  $($summary.failure_stage): $($summary.reason)" -ForegroundColor Red}
    if($summary.status -ne 'PASS' -and $path){Write-Host "Если тест успел создать файл, он сохранён для диагностики: $path"}
    Write-Host 'Объём проверки ограничен тестовым файлом. Это не результат испытаний W5100/SPI на UNO.'
    $null=Stop-Transcript
}
if($summary.status -ne 'PASS'){exit 1}
exit 0

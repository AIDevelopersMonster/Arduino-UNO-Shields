#requires -Version 7.0
[CmdletBinding()]
param([string]$ArduinoCli='arduino-cli',
      [string]$BuildDirectory=(Join-Path $PSScriptRoot '../../../../../build/W5100_COMPARE'),
      [string]$UploadPort)
$ErrorActionPreference='Stop'
$build=[IO.Path]::GetFullPath($BuildDirectory)
$null=New-Item -ItemType Directory -Path $build -Force
$raw=& $ArduinoCli compile --fqbn arduino:avr:uno --warnings all --build-path $build --json $PSScriptRoot
$code=$LASTEXITCODE
$raw | Set-Content -LiteralPath (Join-Path $build 'compile.json') -Encoding utf8
if($code -ne 0){throw "Compile failed ($code); see $build/compile.json"}
$result=($raw -join "`n") | ConvertFrom-Json
if(-not $result.success){throw 'Arduino CLI reports unsuccessful compilation'}
$platform=$result.builder_result.build_platform
if($platform.id -ne 'arduino:avr' -or $platform.version -notin @('1.8.6','1.8.8')){throw 'Expected arduino:avr 1.8.6 or 1.8.8'}
$library=@($result.builder_result.used_libraries | Where-Object name -eq 'SD')
if($library.Count -ne 1 -or $library[0].version -ne '1.3.0'){throw 'Expected SD library 1.3.0'}
$ethernet=@($result.builder_result.used_libraries | Where-Object name -eq 'Ethernet')
if($ethernet.Count -ne 1 -or $ethernet[0].version -ne '2.0.2'){throw 'Expected Ethernet library 2.0.2'}
$sections=$result.builder_result.executable_sections_size
$flash=@($sections | Where-Object name -eq 'text')
$sram=@($sections | Where-Object name -eq 'data')
if($flash.Count -ne 1 -or $sram.Count -ne 1){throw 'Missing Flash/SRAM measurements'}
$passed=$flash[0].size -le 29000 -and $sram[0].size -le 1200
$hashes=[ordered]@{}
foreach($name in @('W5100_Compare.ino','W5100Compare.psm1','Test-W5100Compare.ps1')){
    $hashes[$name]=(Get-FileHash (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
}
$hex=Join-Path $build 'W5100_Compare.ino.hex'
$summary=[ordered]@{test='W5100_COMPARE';type='BUILD_ONLY';firmware_version='0.1';scope='UNO_W5100_READ_COMPARE';ethernet_spi_hz=4000000;ethernet_init_spi_hz=8000000;
    sd_init_spi_hz=250000;sd_data_spi_hz=4000000;status=$(if($passed){'PASS'}else{'FAIL'});
    utc=[DateTime]::UtcNow.ToString('o');fqbn='arduino:avr:uno';core=$platform.version;sd=$library[0].version;ethernet=$ethernet[0].version;
    flash_bytes=$flash[0].size;flash_limit=29000;sram_static_bytes=$sram[0].size;sram_static_limit=1200;
    hex_sha256=(Get-FileHash $hex -Algorithm SHA256).Hash;source_hashes=$hashes;hardware_status='PENDING'}
$summary | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $build 'build-summary.json') -Encoding utf8
Write-Host "BUILD $($summary.status) / W5100_COMPARE / Flash=$($flash[0].size)/32256 / static SRAM=$($sram[0].size)/2048 / AVR=$($platform.version) / SD=1.3.0 / raw/LIB RTR at 4 MHz" -ForegroundColor $(if($summary.status -eq "PASS"){"Green"}else{"Red"})
if(-not $passed){throw 'Flash <=29000 and static SRAM <=1200 bytes required'}
if($UploadPort){
    & $ArduinoCli upload -p $UploadPort --fqbn arduino:avr:uno --input-dir $build $PSScriptRoot
    if($LASTEXITCODE -ne 0){throw 'Upload failed'}
    Write-Host 'Measured binary uploaded; hardware result remains PENDING.'
}

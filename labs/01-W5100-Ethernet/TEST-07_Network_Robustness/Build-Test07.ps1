#requires -Version 7.0
[CmdletBinding()]
param(
    [string]$ArduinoCli='arduino-cli',
    [string]$BuildDirectory=(Join-Path $PSScriptRoot '../../../build/TEST07'),
    [string]$UploadPort
)
$ErrorActionPreference='Stop'
$build=[IO.Path]::GetFullPath($BuildDirectory)
$null=New-Item -ItemType Directory -Path $build -Force
$raw=& $ArduinoCli compile --fqbn arduino:avr:uno --warnings all --build-path $build --json $PSScriptRoot
$compileCode=$LASTEXITCODE
$raw | Set-Content -LiteralPath (Join-Path $build 'compile.json') -Encoding utf8
if($compileCode -ne 0){throw "Compile failed ($compileCode). See $build/compile.json"}
$result=($raw -join "`n") | ConvertFrom-Json
if(-not $result.success){throw 'Arduino CLI reports unsuccessful compile'}
$sections=$result.builder_result.executable_sections_size
$flash=@($sections | Where-Object name -eq 'text')[0]
$sram=@($sections | Where-Object name -eq 'data')[0]
$eth=@($result.builder_result.used_libraries | Where-Object name -eq 'Ethernet')
if($eth.Count -ne 1 -or $eth[0].version -ne '2.0.2'){throw 'Selected Ethernet library must be 2.0.2'}
if($result.builder_result.build_platform.version -ne '1.8.6'){throw 'Selected AVR core must be 1.8.6'}
$passed=($flash.size -le 29000 -and $sram.size -le 1200)
$hashes=[ordered]@{}
foreach($name in @('TEST-07_Network_Robustness.ino','NetworkRobustness.psm1','Test-NetworkRobustness.ps1')) {
    $hashes[$name]=(Get-FileHash (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash
}
$hex=Join-Path $build 'TEST-07_Network_Robustness.ino.hex'
$summary=[ordered]@{
    test='TEST-07'; type='BUILD_ONLY'; status=$(if($passed){'PASS'}else{'FAIL'})
    utc=[DateTime]::UtcNow.ToString('o'); cli=((& $ArduinoCli version) -join ' ')
    fqbn='arduino:avr:uno'; core='1.8.6'; ethernet='2.0.2'
    flash_bytes=$flash.size; flash_limit=29000; sram_static_bytes=$sram.size; sram_static_limit=1200
    static_headroom_bytes=(2048-$sram.size); hex_sha256=(Get-FileHash $hex -Algorithm SHA256).Hash
    source_hashes=$hashes; hardware_status='PENDING'
}
$summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $build 'build-summary.json') -Encoding utf8
Write-Host "BUILD $($summary.status) / Flash=$($flash.size)/32256 / static SRAM=$($sram.size)/2048"
if(-not $passed){throw 'Memory budget exceeded (Flash <=29000, static SRAM <=1200 bytes)'}
if($UploadPort) {
    & $ArduinoCli upload -p $UploadPort --fqbn arduino:avr:uno --input-dir $build $PSScriptRoot
    if($LASTEXITCODE -ne 0){throw 'Upload failed'}
    Write-Host 'Uploaded the measured binary. Hardware verification remains PENDING.'
}

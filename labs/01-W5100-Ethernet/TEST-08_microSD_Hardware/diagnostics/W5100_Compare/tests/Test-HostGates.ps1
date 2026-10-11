#requires -Version 7.0
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../W5100Compare.psm1') -Force
$token='A1B2C3D4'
$names=@('CARD_INIT','ETH_DRIVER_INIT','RTR_BEFORE','CARD_INFO','FAT_VOLUME','ROOT_OPEN',
    'EXCLUSIVE_CREATE','WRITE_2048','REOPEN_VERIFY_2048','SEEK_BOUNDARIES','APPEND_64','REOPEN_VERIFY_2112',
    'RTR_AFTER_IO','REMOUNT_VERIFY','RTR_AFTER_REMOUNT','REMOVE_TEST_FILE','RAM')
$fixture=@('BOOT test=W5100_COMPARE fw=0.1 eth_cs=10 sd_cs=4 uart=115200 eth_spi_hz=4000000 eth_init_spi_hz=8000000 sd_init_spi_hz=250000 sd_data_spi_hz=4000000','READY',"START token=$token free=900",
    "CARD token=$token error_code=0 error_data=0","CARD_INFO token=$token type=3 blocks=8388608",
    "VOLUME token=$token fat=32","ETH_INIT token=$token rc=1 chip=51 init_spi_hz=8000000","RTR_BASE token=$token value=07D0")
for($i=0;$i -lt $names.Count;$i++){$fixture+="CHECK token=$token index=$($i+1) name=$($names[$i]) status=PASS free=850"}
$phases=@('BEFORE','AFTER_IO','AFTER_REMOUNT');$methods=@('RAW','LIB','LIB','RAW')
for($i=0;$i -lt 36;$i++){
    $phase=$phases[[int][math]::Floor($i/12)];$cycle=1+[int][math]::Floor(($i%12)/4);$order=1+($i%4)
    $fixture+="RREG token=$token sample=$($i+1) phase=$phase cycle=$cycle order=$order method=$($methods[$i%4]) value=07D0 spi_hz=4000000 spcr=50 spi2x=0 status=PASS"
}
$fixture+=@("VERIFY token=$token bytes=2048 crc16=$(Get-W5100CompareCrc 2048)",
    "VERIFY token=$token bytes=2112 crc16=$(Get-W5100CompareCrc 2112)",
    "VERIFY token=$token bytes=2112 crc16=$(Get-W5100CompareCrc 2112)",
    "RESULT token=$token status=PASS checks=17 failures=0 planned_bytes=2112 bytes=2112 crc16=$(Get-W5100CompareCrc 2112) free=900 min_free=800 bus_fault=0 baseline=07D0 rtr_samples=36 compare_failures=0 elapsed_ms=300")
$count=0
if((Get-W5100CompareCrc 2048) -cne 'A535' -or (Get-W5100CompareCrc 2112) -cne '2B28'){
    throw 'CRC differs from independent Python binascii.crc_hqx reference'
}
function Assert-Verdict([string]$Name,[string[]]$Lines,[string]$Expected){
    $actual=Get-W5100CompareVerdict -Lines $Lines -Token $token
    if($actual.status -ne $Expected){throw "$Name expected $Expected, got $($actual.status): $($actual.reasons -join '; ')"}
    $script:count++
}
Assert-Verdict 'complete model evidence' $fixture 'PASS'
Assert-Verdict 'no terminal result' @($fixture | Where-Object {$_ -notmatch '^RESULT '}) 'FAIL'
Assert-Verdict 'duplicate boot' ($fixture+@($fixture[0])) 'FAIL'
Assert-Verdict 'wrong request token' @($fixture -replace 'token=A1B2C3D4','token=12345678') 'FAIL'
Assert-Verdict 'failed card initialization' @($fixture -replace 'name=CARD_INIT status=PASS','name=CARD_INIT status=FAIL') 'FAIL'
Assert-Verdict 'missing removal evidence' @($fixture | Where-Object {$_ -notmatch 'name=REMOVE_TEST_FILE'}) 'FAIL'
Assert-Verdict 'duplicate check' ($fixture+@($fixture | Where-Object {$_ -match 'name=RAM'})) 'FAIL'
Assert-Verdict 'wrong pin banner' @($fixture -replace 'sd_cs=4','sd_cs=5') 'FAIL'
Assert-Verdict 'wrong firmware version' @($fixture -replace 'fw=0.1','fw=0.4') 'FAIL'
Assert-Verdict 'unsupported filesystem' @($fixture -replace 'fat=32','fat=12') 'FAIL'
Assert-Verdict 'zero capacity' @($fixture -replace 'blocks=8388608','blocks=0') 'FAIL'
Assert-Verdict 'bad readback CRC' @($fixture -replace 'crc16=[0-9A-F]{4}','crc16=0000') 'FAIL'
Assert-Verdict 'wrong readback size' @($fixture -replace 'VERIFY token=A1B2C3D4 bytes=2048','VERIFY token=A1B2C3D4 bytes=2047') 'FAIL'
Assert-Verdict 'low SRAM' @($fixture -replace 'min_free=800','min_free=511') 'FAIL'
Assert-Verdict 'SRAM drift' @($fixture -replace 'free=900 min_free=800','free=820 min_free=800') 'FAIL'
Assert-Verdict 'malformed terminal fields' @($fixture -replace 'min_free=800','min_free=invalid') 'FAIL'
Assert-Verdict 'missing terminal fields' @($fixture -replace ' min_free=800','') 'FAIL'
Assert-Verdict 'terminal FAIL' @($fixture -replace 'RESULT token=A1B2C3D4 status=PASS','RESULT token=A1B2C3D4 status=FAIL') 'FAIL'
Assert-Verdict 'wrong check index' @($fixture -replace 'index=9 name=REOPEN_VERIFY_2048','index=10 name=REOPEN_VERIFY_2048') 'FAIL'
Assert-Verdict 'command rejected' ($fixture+@('COMMAND_REJECTED')) 'FAIL'
Assert-Verdict 'truncated check fields' @($fixture -replace 'name=RAM status=PASS free=850','name=RAM status=PASS') 'FAIL'
Assert-Verdict 'standalone startup noise' (@('???GR????')+$fixture) 'PASS'
Assert-Verdict 'noise glued to boot' @($fixture -replace '^BOOT ','???GR????BOOT ') 'FAIL'
Assert-Verdict 'second boot hidden by noise' ($fixture+@('???BOOT test=W5100_COMPARE fw=0.1 eth_cs=10 sd_cs=4 uart=115200')) 'FAIL'
Assert-Verdict 'wrong Ethernet comparison clock' @($fixture -replace 'eth_spi_hz=4000000','eth_spi_hz=1000000') 'FAIL'
Assert-Verdict 'missing Ethernet-disabled banner' @($fixture -replace ' eth_spi_hz=4000000','') 'FAIL'
Assert-Verdict 'bus ownership fault' @($fixture -replace 'bus_fault=0','bus_fault=1') 'FAIL'
Assert-Verdict 'missing bus ownership result' @($fixture -replace ' bus_fault=0','') 'FAIL'
Assert-Verdict 'missing SD init clock' @($fixture -replace ' sd_init_spi_hz=250000','') 'FAIL'
Assert-Verdict 'unexpected SD data clock' @($fixture -replace 'sd_data_spi_hz=4000000','sd_data_spi_hz=8000000') 'FAIL'
Assert-Verdict 'raw-only substituted for driver' @($fixture -replace 'method=LIB','method=RAW') 'FAIL'
Assert-Verdict 'full TEST-08 firmware is not W5100_COMPARE' @($fixture -replace 'test=W5100_COMPARE','test=TEST08') 'FAIL'
Assert-Verdict 'CARD diagnostics disagree with check' @($fixture -replace 'error_code=0','error_code=1') 'FAIL'
Assert-Verdict 'missing CARD diagnostics' @($fixture | Where-Object {$_ -notmatch '^CARD '}) 'FAIL'
Assert-Verdict 'unverified planned length is not read bytes' @($fixture -replace 'planned_bytes=2112 bytes=2112','planned_bytes=2112 bytes=0') 'FAIL'
Assert-Verdict 'missing token in readback evidence' @($fixture -replace 'VERIFY token=A1B2C3D4','VERIFY') 'FAIL'
Assert-Verdict 'first bad raw sample survives good later samples' @($fixture -replace 'sample=1 phase=BEFORE cycle=1 order=1 method=RAW value=07D0','sample=1 phase=BEFORE cycle=1 order=1 method=RAW value=0750') 'FAIL'
Assert-Verdict 'library disagreement' @($fixture -replace 'method=LIB value=07D0','method=LIB value=0750') 'FAIL'
Assert-Verdict 'all reads FFFF' @($fixture -replace '07D0','FFFF') 'FAIL'
Assert-Verdict 'all reads zero' @($fixture -replace '07D0','0000') 'FAIL'
Assert-Verdict 'missing samples' @($fixture | Where-Object {$_ -notmatch 'sample=36 '}) 'FAIL'
Assert-Verdict 'duplicate samples' ($fixture+@($fixture | Where-Object {$_ -match 'sample=1 '})) 'FAIL'
Assert-Verdict 'missing phase' @($fixture -replace 'phase=AFTER_REMOUNT','phase=AFTER_IO') 'FAIL'
Assert-Verdict 'wrong cycle' @($fixture -replace 'cycle=3','cycle=2') 'FAIL'
Assert-Verdict 'wrong SPI register bits' @($fixture -replace 'spcr=50','spcr=52') 'FAIL'
Assert-Verdict 'double speed enabled' @($fixture -replace 'spi2x=0','spi2x=1') 'FAIL'
Assert-Verdict 'row FAIL preserved' @($fixture -replace '(RREG .*)status=PASS','$1status=FAIL') 'FAIL'
Assert-Verdict 'bad driver init' @($fixture -replace 'rc=1 chip=51','rc=0 chip=51') 'FAIL'
Assert-Verdict 'wrong Ethernet chip' @($fixture -replace 'chip=51','chip=55') 'FAIL'
Assert-Verdict 'missing library baseline' @($fixture | Where-Object {$_ -notmatch '^RTR_BASE '}) 'FAIL'
Assert-Verdict 'baseline not first LIB' @($fixture -replace 'RTR_BASE token=A1B2C3D4 value=07D0','RTR_BASE token=A1B2C3D4 value=1234') 'FAIL'
Assert-Verdict 'comparison failure counter' @($fixture -replace 'compare_failures=0','compare_failures=1') 'FAIL'
Assert-Verdict 'missing comparison result count' @($fixture -replace ' rtr_samples=36','') 'FAIL'
Assert-Verdict 'missing token in comparison' @($fixture -replace 'RREG token=A1B2C3D4','RREG') 'FAIL'
Write-Host "HOST CHECKS PASS / $count verdict cases. UART evidence is synthetic; no card hardware result."

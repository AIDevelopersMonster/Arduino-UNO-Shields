#requires -Version 7.0
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../MicroSDHardware.psm1') -Force
$token='A1B2C3D4'
$names=@('ETH_SPI_BEFORE','CARD_INIT','CARD_INFO','FAT_VOLUME','ROOT_OPEN','EXCLUSIVE_CREATE',
    'WRITE_2048','REOPEN_VERIFY_2048','SEEK_BOUNDARIES','APPEND_64','REOPEN_VERIFY_2112',
    'REMOUNT_VERIFY','REMOVE_TEST_FILE','ETH_SPI_AFTER','RAM')
$fixture=@('BOOT test=TEST08 fw=0.1 eth_cs=10 sd_cs=4 uart=115200','READY',"START token=$token free=900",
    "CARD token=$token error_code=0 error_data=0","CARD_INFO token=$token type=3 blocks=8388608",
    "VOLUME token=$token fat=32")
for($i=0;$i -lt $names.Count;$i++){$fixture+="CHECK token=$token index=$($i+1) name=$($names[$i]) status=PASS free=850"}
$fixture+=@("VERIFY token=$token bytes=2048 crc16=$(Get-Test08Crc 2048)",
    "VERIFY token=$token bytes=2112 crc16=$(Get-Test08Crc 2112)",
    "VERIFY token=$token bytes=2112 crc16=$(Get-Test08Crc 2112)",
    "RESULT token=$token status=PASS checks=15 failures=0 bytes=2112 crc16=$(Get-Test08Crc 2112) free=900 min_free=800 elapsed_ms=300")
$count=0
if((Get-Test08Crc 2048) -cne 'A535' -or (Get-Test08Crc 2112) -cne '2B28'){
    throw 'CRC differs from independent Python binascii.crc_hqx reference'
}
function Assert-Verdict([string]$Name,[string[]]$Lines,[string]$Expected){
    $actual=Get-Test08Verdict -Lines $Lines -Token $token
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
Assert-Verdict 'wrong firmware version' @($fixture -replace 'fw=0.1','fw=0.2') 'FAIL'
Assert-Verdict 'unsupported filesystem' @($fixture -replace 'fat=32','fat=12') 'FAIL'
Assert-Verdict 'zero capacity' @($fixture -replace 'blocks=8388608','blocks=0') 'FAIL'
Assert-Verdict 'bad readback CRC' @($fixture -replace 'crc16=[0-9A-F]{4}','crc16=0000') 'FAIL'
Assert-Verdict 'wrong readback size' @($fixture -replace 'VERIFY token=A1B2C3D4 bytes=2048','VERIFY token=A1B2C3D4 bytes=2047') 'FAIL'
Assert-Verdict 'low SRAM' @($fixture -replace 'min_free=800','min_free=511') 'FAIL'
Assert-Verdict 'SRAM drift' @($fixture -replace 'free=900 min_free=800','free=820 min_free=800') 'FAIL'
Assert-Verdict 'malformed terminal fields' @($fixture -replace 'min_free=800','min_free=invalid') 'FAIL'
Assert-Verdict 'missing terminal fields' @($fixture -replace ' min_free=800','') 'FAIL'
Assert-Verdict 'terminal FAIL' @($fixture -replace 'RESULT token=A1B2C3D4 status=PASS','RESULT token=A1B2C3D4 status=FAIL') 'FAIL'
Assert-Verdict 'wrong check index' @($fixture -replace 'index=8 name=REOPEN_VERIFY_2048','index=9 name=REOPEN_VERIFY_2048') 'FAIL'
Assert-Verdict 'command rejected' ($fixture+@('COMMAND_REJECTED')) 'FAIL'
Assert-Verdict 'truncated check fields' @($fixture -replace 'name=RAM status=PASS free=850','name=RAM status=PASS') 'FAIL'
Write-Host "HOST CHECKS PASS / $count verdict cases. UART evidence is synthetic; no card hardware result."

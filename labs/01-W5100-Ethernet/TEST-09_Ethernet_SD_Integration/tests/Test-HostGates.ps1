#requires -Version 7.0
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot '../EthernetSD.psm1') -Force
$token='0123ABCD';$sizes=@(16,32,64,128)
$probes=@(1..60 | ForEach-Object {[pscustomobject]@{sequence=$_;bytes=$sizes[($_-1)%4];code='PASS'}})
$crc=Get-Test09Crc (New-Test09Payload -Length 128 -Sequence 60 -Token $token)
$lines=@(
 'BOOT test=TEST09 fw=0.1 eth_cs=10 sd_cs=4 uart=115200','READY',
 "START token=$token duration_s=30","CARD token=$token error_code=0 error_data=0",
 "SD token=$token type=3 blocks=30536704 fat=32","NET token=$token ip=192.168.1.85 udp=5001 chip=W5100 rtr=07D0 free=560",
 "STAT token=$token elapsed_ms=5000 rx=10 verified=10 tx=10 errors=0 free=550 min_free=520",
 "STAT token=$token elapsed_ms=10000 rx=20 verified=20 tx=20 errors=0 free=550 min_free=520",
 "RESULT token=$token status=PASS rx=60 verified=60 tx=60 bytes=3600 errors=0 cleanup=1 expected_rtr=07D0 rtr1=07D0 rtr2=07D0 crc16=$crc initial_free=560 free=560 min_free=520 max_sd_ms=55 elapsed_ms=30100"
)
$count=0
function Assert-Verdict([string]$Name,[string[]]$Evidence,[object[]]$Transactions,[double]$Seconds,[string]$Expected){
    $v=Get-Test09Verdict -Lines $Evidence -Token $token -DurationSeconds 30 -Probes $Transactions -LoadSeconds $Seconds
    if($v.status -ne $Expected){throw "$Name expected $Expected got $($v.status): $($v.reasons -join '; ')"}
    $script:count++
}
Assert-Verdict 'complete synthetic evidence' $lines $probes 30.1 PASS
Assert-Verdict 'separate startup bytes' (@('{????B??')+$lines) $probes 30.1 PASS
Assert-Verdict 'extra boot' ($lines+@($lines[0])) $probes 30.1 FAIL
Assert-Verdict 'glued boot' ($lines -replace '^BOOT','garbageBOOT') $probes 30.1 FAIL
Assert-Verdict 'missing result' $lines[0..7] $probes 30.1 FAIL
Assert-Verdict 'empty UART' @() $probes 30.1 FAIL
Assert-Verdict 'early completion' $lines $probes 29 FAIL
Assert-Verdict 'no probes' $lines @() 30.1 FAIL
foreach($pair in @(
 @('bad token','token=0123ABCD','token=DEADBEEF'),@('SD init','error_code=0','error_code=1'),
 @('zero capacity','blocks=30536704','blocks=0'),@('unsupported FAT','fat=32','fat=12'),
 @('no W5100','chip=W5100','chip=OTHER'),@('no IP','ip=192.168.1.85','ip=0.0.0.0'),
 @('RTR corruption','rtr2=07D0','rtr2=07D1'),@('CRC corruption',"crc16=$crc",'crc16=0000'),
 @('RAM minimum','min_free=520','min_free=511'),@('RAM drift','initial_free=560','initial_free=700'),
 @('cleanup failure','cleanup=1','cleanup=0'),@('terminal counter','verified=60','verified=59'),
 @('byte total','bytes=3600','bytes=3599'),@('board duration','elapsed_ms=30100','elapsed_ms=1000'),
 @('bad terminal','status=PASS','status=FAIL'),@('malformed terminal','errors=0 cleanup=1','errors=oops cleanup=1')
)){
    $changed=@($lines | ForEach-Object {$_.Replace($pair[1],$pair[2])})
    Assert-Verdict $pair[0] $changed $probes 30.1 FAIL
}
$badProbes=@($probes | ForEach-Object {[pscustomobject]@{sequence=$_.sequence;bytes=$_.bytes;code=$_.code}})
$badProbes[1].code='TIMEOUT';Assert-Verdict 'UDP timeout' $lines $badProbes 30.1 FAIL
$badProbes[1].code='CORRUPT';Assert-Verdict 'UDP corruption' $lines $badProbes 30.1 FAIL
$badProbes[1].code='PASS';$badProbes[1].sequence=1;Assert-Verdict 'duplicate sequence' $lines $badProbes 30.1 FAIL
Assert-Verdict 'DHCP failure' ($lines+@("DHCP_MAINTAIN token=$token rc=1")) $probes 30.1 FAIL
Assert-Verdict 'renew observed' ($lines+@("DHCP_MAINTAIN token=$token rc=2")) $probes 30.1 PASS
Write-Host "HOST GATES PASS / $count cases / synthetic evidence, no hardware claim"

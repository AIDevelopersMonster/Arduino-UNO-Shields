Set-StrictMode -Version Latest

function Get-Test08Crc {
    param([ValidateRange(1,2112)][int]$Bytes)
    [int]$crc=65535
    for($position=0; $position -lt $Bytes; $position++) {
        $value=(($position*73) -bxor ($position -shr 3) -bxor 165) -band 255
        $crc=$crc -bxor ($value -shl 8)
        for($bit=0; $bit -lt 8; $bit++) {
            if($crc -band 32768){$crc=(($crc -shl 1) -bxor 4129) -band 65535}
            else{$crc=($crc -shl 1) -band 65535}
        }
    }
    '{0:X4}' -f $crc
}
function ConvertFrom-Test08Line {
    param([string]$Line)
    if($Line -notmatch '^(?<kind>[A-Z_]+)(?: (?<fields>.*))?$'){return $null}
    $record=@{kind=$Matches.kind}
    if($Matches.ContainsKey('fields') -and $Matches.fields) {
        foreach($pair in $Matches.fields.Split(' ',[StringSplitOptions]::RemoveEmptyEntries)) {
            if($pair -notmatch '^(?<key>[a-z][a-z0-9_]*)=(?<value>[^= ]+)$'){return $null}
            if($record.ContainsKey($Matches.key)){return $null}
            $record[$Matches.key]=$Matches.value
        }
    }
    foreach($key in @('token','test','fw','eth_cs','sd_cs','uart','free','index','name','status',
        'type','blocks','fat','bytes','crc16','checks','failures','min_free','elapsed_ms',
        'phase','expected','observed1','observed2','probe','restored')){
        if(-not $record.ContainsKey($key)){$record[$key]=$null}
    }
    $record
}
function Get-Test08Verdict {
    param([string[]]$Lines,[string]$Token)
    $reasons=[Collections.Generic.List[string]]::new()
    $records=@($Lines | ForEach-Object {ConvertFrom-Test08Line $_} | Where-Object {$null -ne $_})
    if($Token -notmatch '^[0-9A-F]{8}$'){$reasons.Add('invalid request token')}
    foreach($record in $records) {
        if($record.token -and $record.token -cne $Token){$reasons.Add('unexpected request token')}
    }
    $boots=@($records | Where-Object kind -eq 'BOOT')
    if($boots.Count -ne 1){$reasons.Add('exactly one BOOT required')}
    elseif($boots[0].test -ne 'TEST08' -or $boots[0].fw -ne '0.2' -or
           $boots[0].eth_cs -ne '10' -or $boots[0].sd_cs -ne '4' -or $boots[0].uart -ne '115200'){
        $reasons.Add('unexpected firmware/pin banner')
    }
    $bootMarkers=0
    foreach($line in $Lines){
        $bootMarkers+=[regex]::Matches($line,'BOOT test=TEST08(?: |$)').Count
        if($line -match 'BOOT test=TEST08(?: |$)' -and $line -notmatch '^BOOT test=TEST08(?: |$)'){
            $reasons.Add('BOOT banner is not on its own line')
        }
    }
    if($bootMarkers -ne 1){$reasons.Add('exactly one TEST08 BOOT marker required')}
    if(@($records | Where-Object kind -eq 'READY').Count -ne 1){$reasons.Add('exactly one READY required')}
    if(@($records | Where-Object kind -eq 'COMMAND_REJECTED').Count){$reasons.Add('command rejected')}
    $start=@($records | Where-Object kind -eq 'START')
    $end=@($records | Where-Object kind -eq 'RESULT')
    if($start.Count -ne 1 -or $start[0].token -cne $Token){$reasons.Add('matching START required')}
    if($end.Count -ne 1 -or $end[0].token -cne $Token){$reasons.Add('matching terminal RESULT required')}
    $expected=@('ETH_SPI_BEFORE','CARD_INIT','CARD_INFO','FAT_VOLUME','ROOT_OPEN',
        'EXCLUSIVE_CREATE','WRITE_2048','REOPEN_VERIFY_2048','SEEK_BOUNDARIES','APPEND_64',
        'REOPEN_VERIFY_2112','REMOUNT_VERIFY','REMOVE_TEST_FILE','ETH_SPI_AFTER','RAM')
    $checks=@($records | Where-Object kind -eq 'CHECK')
    if($checks.Count -ne $expected.Count){$reasons.Add('all 15 checks required')}
    else {
        for($i=0;$i -lt $expected.Count;$i++) {
            if($checks[$i].index -ne [string]($i+1) -or $checks[$i].name -cne $expected[$i] -or
               $checks[$i].status -cne 'PASS' -or $checks[$i].token -cne $Token){
                $reasons.Add("failed/missing/out-of-order check: $($expected[$i])")
            }
            [int]$free=0
            if(-not [int]::TryParse($checks[$i].free,[ref]$free) -or $free -lt 512 -or $free -gt 2048){
                $reasons.Add("invalid/low sampled SRAM: $($expected[$i])")
            }
        }
    }
    $info=@($records | Where-Object kind -eq 'CARD_INFO')
    [uint32]$blocks=0
    if($info.Count -ne 1 -or $info[0].type -notin @('1','2','3') -or
       -not [uint32]::TryParse($info[0].blocks,[ref]$blocks) -or $blocks -eq 0){$reasons.Add('valid card type/capacity required')}
    $fat=@($records | Where-Object kind -eq 'VOLUME')
    if($fat.Count -ne 1 -or $fat[0].fat -notin @('16','32')){$reasons.Add('FAT16/FAT32 required')}
    $spi=@($records | Where-Object kind -eq 'ETH_RTR')
    if($spi.Count -ne 2){$reasons.Add('before/after RTR diagnostics required')}
    else {
        if($spi[0].phase -cne 'BEFORE' -or $spi[1].phase -cne 'AFTER' -or
           $spi[0].expected -cnotmatch '^[0-9A-F]{4}$' -or $spi[0].probe -cne '1234' -or
           $spi[0].restored -cne $spi[0].expected -or $spi[1].expected -cne $spi[0].expected){
            $reasons.Add('RTR probe/restore diagnostics inconsistent')
        }
        foreach($row in $spi){
            if($row.observed1 -cne $row.expected -or $row.observed2 -cne $row.expected){
                $reasons.Add('RTR preservation check failed')
            }
        }
    }
    $verified=@($records | Where-Object kind -eq 'VERIFY')
    $sizes=@(2048,2112,2112)
    if($verified.Count -ne 3){$reasons.Add('three complete readbacks required')}
    else {
        for($i=0;$i -lt 3;$i++) {
            if($verified[$i].bytes -ne [string]$sizes[$i] -or $verified[$i].crc16 -cne (Get-Test08Crc $sizes[$i])){
                $reasons.Add('readback length/CRC does not match deterministic payload')
            }
        }
    }
    $metrics=$null
    if($end.Count -eq 1 -and $start.Count -eq 1) {
        $last=$end[0]
        [int]$initial=0;[int]$free=0;[int]$minimum=0;[uint32]$elapsed=0
        $numbers=[int]::TryParse($start[0].free,[ref]$initial) -and
            [int]::TryParse($last.free,[ref]$free) -and [int]::TryParse($last.min_free,[ref]$minimum) -and
            [uint32]::TryParse($last.elapsed_ms,[ref]$elapsed)
        if($last.status -cne 'PASS' -or $last.checks -ne '15' -or $last.failures -ne '0' -or
           $last.bytes -ne '2112' -or $last.crc16 -cne (Get-Test08Crc 2112)){$reasons.Add('terminal result gates failed')}
        if(-not $numbers -or $initial -lt 512 -or $initial -gt 2048 -or $free -gt 2048 -or
           $minimum -lt 512 -or $minimum -gt $free -or $minimum -gt $initial -or
           $free -lt $initial-64){$reasons.Add('SRAM minimum/drift gates failed')}
        $metrics=[ordered]@{checks=$checks.Count;bytes=2112;crc16=$last.crc16;
            initial_free=$initial;final_free=$free;min_free=$minimum;elapsed_ms=$elapsed;
            card_blocks=$blocks;card_bytes=([uint64]$blocks*512);fat=$(if($fat.Count -eq 1){$fat[0].fat}else{$null});
            ethernet_rtr=@($spi | ForEach-Object {[pscustomobject]@{phase=$_.phase;expected=$_.expected;observed1=$_.observed1;observed2=$_.observed2}})}
    }
    [pscustomobject]@{status=$(if($reasons.Count -eq 0){'PASS'}else{'FAIL'});reasons=@($reasons | Select-Object -Unique);metrics=$metrics}
}
Export-ModuleMember -Function Get-Test08Crc,ConvertFrom-Test08Line,Get-Test08Verdict

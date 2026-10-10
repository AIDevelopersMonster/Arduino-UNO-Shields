Set-StrictMode -Version Latest

function Get-W5100CompareCrc {
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
function ConvertFrom-W5100CompareLine {
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
    foreach($key in @('token','test','fw','eth_cs','sd_cs','uart','eth_spi_hz','sd_init_spi_hz','sd_data_spi_hz','free','index','name','status',
        'type','blocks','fat','bytes','crc16','checks','failures','min_free','elapsed_ms','bus_fault',
        'planned_bytes','error_code','error_data','eth_init_spi_hz','init_spi_hz','rc','chip',
        'value','sample','phase','cycle','order','method','spi_hz','spcr','spi2x',
        'baseline','rtr_samples','compare_failures')){
        if(-not $record.ContainsKey($key)){$record[$key]=$null}
    }
    $record
}
function Get-W5100CompareVerdict {
    param([string[]]$Lines,[string]$Token)
    $reasons=[Collections.Generic.List[string]]::new()
    $records=@($Lines | ForEach-Object {ConvertFrom-W5100CompareLine $_} | Where-Object {$null -ne $_})
    if($Token -notmatch '^[0-9A-F]{8}$'){$reasons.Add('invalid request token')}
    foreach($record in $records) {
        if($record.token -and $record.token -cne $Token){$reasons.Add('unexpected request token')}
        if($record.kind -in @('START','CARD','CARD_INFO','VOLUME','CHECK','WRITE','VERIFY','RESULT','ETH_INIT','RTR_BASE','RREG') -and
           $record.token -cne $Token){$reasons.Add('missing/mismatched token in test evidence')}
    }
    $boots=@($records | Where-Object kind -eq 'BOOT')
    if($boots.Count -ne 1){$reasons.Add('exactly one BOOT required')}
    elseif($boots[0].test -ne 'W5100_COMPARE' -or $boots[0].fw -ne '0.1' -or
           $boots[0].eth_cs -ne '10' -or $boots[0].sd_cs -ne '4' -or $boots[0].uart -ne '115200' -or
           $boots[0].eth_spi_hz -ne '4000000' -or $boots[0].eth_init_spi_hz -ne '8000000' -or
           $boots[0].sd_init_spi_hz -ne '250000' -or $boots[0].sd_data_spi_hz -ne '4000000'){
        $reasons.Add('unexpected firmware/pin/SPI banner')
    }
    $bootMarkers=0
    foreach($line in $Lines){
        $bootMarkers+=[regex]::Matches($line,'BOOT test=W5100_COMPARE(?: |$)').Count
        if($line -match 'BOOT test=W5100_COMPARE(?: |$)' -and $line -notmatch '^BOOT test=W5100_COMPARE(?: |$)'){
            $reasons.Add('BOOT banner is not on its own line')
        }
    }
    if($bootMarkers -ne 1){$reasons.Add('exactly one W5100_COMPARE BOOT marker required')}
    if(@($records | Where-Object kind -eq 'READY').Count -ne 1){$reasons.Add('exactly one READY required')}
    if(@($records | Where-Object kind -eq 'COMMAND_REJECTED').Count){$reasons.Add('command rejected')}
    $start=@($records | Where-Object kind -eq 'START')
    $end=@($records | Where-Object kind -eq 'RESULT')
    if($start.Count -ne 1 -or $start[0].token -cne $Token){$reasons.Add('matching START required')}
    if($end.Count -ne 1 -or $end[0].token -cne $Token){$reasons.Add('matching terminal RESULT required')}
    $expected=@('CARD_INIT','ETH_DRIVER_INIT','RTR_BEFORE','CARD_INFO','FAT_VOLUME','ROOT_OPEN',
        'EXCLUSIVE_CREATE','WRITE_2048','REOPEN_VERIFY_2048','SEEK_BOUNDARIES','APPEND_64',
        'REOPEN_VERIFY_2112','RTR_AFTER_IO','REMOUNT_VERIFY','RTR_AFTER_REMOUNT','REMOVE_TEST_FILE','RAM')
    $checks=@($records | Where-Object kind -eq 'CHECK')
    if($checks.Count -ne $expected.Count){$reasons.Add('all 17 comparison/SD checks required')}
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
    $card=@($records | Where-Object kind -eq 'CARD')
    if($card.Count -ne 1 -or $card[0].error_code -ne '0' -or $card[0].error_data -ne '0'){
        $reasons.Add('one successful CARD initialization record required')
    }
    [uint32]$blocks=0
    if($info.Count -ne 1 -or $info[0].type -notin @('1','2','3') -or
       -not [uint32]::TryParse($info[0].blocks,[ref]$blocks) -or $blocks -eq 0){$reasons.Add('valid card type/capacity required')}
    $fat=@($records | Where-Object kind -eq 'VOLUME')
    if($fat.Count -ne 1 -or $fat[0].fat -notin @('16','32')){$reasons.Add('FAT16/FAT32 required')}
    $init=@($records | Where-Object kind -eq 'ETH_INIT')
    if($init.Count -ne 1 -or $init[0].rc -ne '1' -or $init[0].chip -ne '51' -or $init[0].init_spi_hz -ne '8000000'){
        $reasons.Add('one successful Ethernet 2.0.2 W5100 driver initialization required')
    }
    $base=@($records | Where-Object kind -eq 'RTR_BASE')
    $baseline=$null
    if($base.Count -ne 1 -or $base[0].value -cnotmatch '^[0-9A-F]{4}$' -or $base[0].value -in @('0000','FFFF')){
        $reasons.Add('valid single library RTR baseline required')
    }else{$baseline=$base[0].value}
    $samples=@($records | Where-Object kind -eq 'RREG')
    $phases=@('BEFORE','AFTER_IO','AFTER_REMOUNT');$methods=@('RAW','LIB','LIB','RAW')
    if($samples.Count -ne 36){$reasons.Add('all 36 raw/library observations required')}
    else{
        for($i=0;$i -lt 36;$i++){
            $phase=$phases[[int][math]::Floor($i/12)];$cycle=[string](1+[int][math]::Floor(($i%12)/4));$order=[string](1+($i%4))
            $row=$samples[$i]
            if($row.sample -ne [string]($i+1) -or $row.phase -cne $phase -or $row.cycle -ne $cycle -or
               $row.order -ne $order -or $row.method -cne $methods[$i%4]){
                $reasons.Add('missing/duplicate/out-of-order raw/library observation')
            }
            if(-not $baseline -or $row.value -cne $baseline -or $row.status -cne 'PASS'){
                $reasons.Add("RTR discrepancy: sample=$($i+1) phase=$phase method=$($row.method) value=$($row.value) baseline=$baseline")
            }
            if($row.spi_hz -ne '4000000' -or $row.spcr -cne '50' -or $row.spi2x -ne '0'){
                $reasons.Add('comparison must use actual AVR SPI mode0/MSB/master/4 MHz settings')
            }
        }
        if($samples[1].value -cne $baseline){$reasons.Add('baseline must be the first LIB observation')}
    }
    $verified=@($records | Where-Object kind -eq 'VERIFY')
    $sizes=@(2048,2112,2112)
    if($verified.Count -ne 3){$reasons.Add('three complete readbacks required')}
    else {
        for($i=0;$i -lt 3;$i++) {
            if($verified[$i].bytes -ne [string]$sizes[$i] -or $verified[$i].crc16 -cne (Get-W5100CompareCrc $sizes[$i])){
                $reasons.Add('readback length/CRC does not match deterministic payload')
            }
        }
    }
    $metrics=$null
    if($end.Count -eq 1 -and $start.Count -eq 1) {
        $last=$end[0]
        [int]$initial=0;[int]$free=0;[int]$minimum=0;[uint32]$elapsed=0;[uint32]$verifiedBytes=0
        $numbers=[int]::TryParse($start[0].free,[ref]$initial) -and
            [int]::TryParse($last.free,[ref]$free) -and [int]::TryParse($last.min_free,[ref]$minimum) -and
            [uint32]::TryParse($last.elapsed_ms,[ref]$elapsed) -and [uint32]::TryParse($last.bytes,[ref]$verifiedBytes)
        if($last.status -cne 'PASS' -or $last.checks -ne '17' -or $last.failures -ne '0' -or
           $last.baseline -cne $baseline -or $last.rtr_samples -ne '36' -or $last.compare_failures -ne '0' -or
           $last.bus_fault -ne '0' -or $last.planned_bytes -ne '2112' -or $last.bytes -ne '2112' -or $last.crc16 -cne (Get-W5100CompareCrc 2112)){$reasons.Add('terminal result gates failed')}
        if(-not $numbers -or $initial -lt 512 -or $initial -gt 2048 -or $free -gt 2048 -or
           $minimum -lt 512 -or $minimum -gt $free -or $minimum -gt $initial -or
           $free -lt $initial-64){$reasons.Add('SRAM minimum/drift gates failed')}
        $metrics=[ordered]@{checks=$checks.Count;planned_bytes=2112;bytes=$verifiedBytes;crc16=$last.crc16;
            initial_free=$initial;final_free=$free;min_free=$minimum;elapsed_ms=$elapsed;
            card_blocks=$blocks;card_bytes=([uint64]$blocks*512);fat=$(if($fat.Count -eq 1){$fat[0].fat}else{$null});
            scope='UNO_W5100_READ_COMPARE';rtr_samples=$samples.Count;baseline=$baseline}
    }
    [pscustomobject]@{status=$(if($reasons.Count -eq 0){'PASS'}else{'FAIL'});reasons=@($reasons | Select-Object -Unique);metrics=$metrics}
}
Export-ModuleMember -Function Get-W5100CompareCrc,ConvertFrom-W5100CompareLine,Get-W5100CompareVerdict

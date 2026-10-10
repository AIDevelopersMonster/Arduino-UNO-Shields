Set-StrictMode -Version Latest

function New-Test09Payload {
    param([ValidateSet(16,32,64,128)][int]$Length,[uint32]$Sequence,[string]$Token)
    if($Token -cnotmatch '^[0-9A-F]{8}$'){throw 'Expected an 8-character uppercase session token'}
    [byte[]]$data=[byte[]]::new($Length)
    for($i=0;$i -lt $Length;$i++){$data[$i]=[byte](($i*73+$Sequence)%256)}
    [Array]::Copy([Text.Encoding]::ASCII.GetBytes($Token),0,$data,0,8)
    for($i=0;$i -lt 4;$i++){$data[8+$i]=[byte](($Sequence -shr ($i*8)) -band 255)}
    return ,$data
}
function Get-Test09Crc {
    param([byte[]]$Data)
    [int]$crc=65535
    foreach($value in $Data){
        $crc=$crc -bxor ([int]$value -shl 8)
        for($bit=0;$bit -lt 8;$bit++){
            $crc=$(if($crc -band 32768){(($crc -shl 1) -bxor 4129) -band 65535}else{($crc -shl 1) -band 65535})
        }
    }
    '{0:X4}' -f $crc
}
function ConvertFrom-Test09Line {
    param([string]$Line)
    if($Line -notmatch '^(?<kind>[A-Z_]+)(?: (?<fields>.*))?$'){return $null}
    $row=@{kind=$Matches.kind}
    if($Matches.ContainsKey('fields') -and $Matches.fields){
        foreach($pair in $Matches.fields.Split(' ',[StringSplitOptions]::RemoveEmptyEntries)){
            if($pair -notmatch '^(?<key>[a-z][a-z0-9_]*)=(?<value>[^= ]+)$' -or $row.ContainsKey($Matches.key)){return $null}
            $row[$Matches.key]=$Matches.value
        }
    }
    foreach($key in @('token','test','fw','eth_cs','sd_cs','uart','duration_s','error_code','error_data','type','blocks','fat','ip','udp','chip','rtr','free','min_free','initial_free','elapsed_ms','rx','verified','tx','bytes','errors','cleanup','status','expected_rtr','rtr1','rtr2','crc16','max_sd_ms','rc')){
        if(-not $row.ContainsKey($key)){$row[$key]=$null}
    }
    return $row
}
function Get-Test09Verdict {
    param([string[]]$Lines,[string]$Token,[int]$DurationSeconds,[object[]]$Probes,[double]$LoadSeconds,[string]$Fatal='')
    $reasons=[Collections.Generic.List[string]]::new()
    if($Fatal){$reasons.Add($Fatal)}
    $records=@($Lines | ForEach-Object {ConvertFrom-Test09Line $_} | Where-Object {$null -ne $_})
    $banner='BOOT test=TEST09 fw=0.1 eth_cs=10 sd_cs=4 uart=115200'
    if(@($Lines | Where-Object {$_ -ceq $banner}).Count -ne 1){$reasons.Add('one complete TEST09 BOOT required')}
    $markers=0
    foreach($line in $Lines){$markers += [regex]::Matches($line,'BOOT test=TEST09(?: |$)').Count}
    if($markers -ne 1){$reasons.Add('duplicate/malformed TEST09 BOOT marker')}
    if(@($records | Where-Object kind -eq 'READY').Count -ne 1){$reasons.Add('one READY required')}
    if(@($records | Where-Object {$_.kind -in @('FAIL','COMMAND_REJECTED')}).Count){$reasons.Add('device failure or rejected command')}
    foreach($record in $records){
        if($record.kind -in @('START','CARD','SD','NET','STAT','RESULT','FAIL','DHCP_MAINTAIN') -and $record.token -cne $Token){$reasons.Add('session token mismatch')}
        if($record.kind -eq 'DHCP_MAINTAIN' -and $record.rc -in @('1','3')){$reasons.Add('DHCP maintenance failure')}
    }
    $rows=@{}
    foreach($kind in @('START','CARD','SD','NET','RESULT')){
        $items=@($records | Where-Object kind -eq $kind)
        if($items.Count -ne 1){$reasons.Add("one $kind required");$rows[$kind]=ConvertFrom-Test09Line $kind}
        else{$rows[$kind]=$items[0]}
    }
    if($rows.START.duration_s -ne [string]$DurationSeconds){$reasons.Add('duration declaration mismatch')}
    if($rows.CARD.error_code -ne '0'){$reasons.Add('SD init did not succeed')}
    [long]$blocks=0
    if($rows.SD.type -notin @('1','2','3') -or $rows.SD.fat -notin @('16','32') -or -not [long]::TryParse($rows.SD.blocks,[ref]$blocks) -or $blocks -le 0){$reasons.Add('valid SDHC/SD and FAT16/FAT32 metadata required')}
    $net=$rows.NET; $end=$rows.RESULT
    $ip=$null
    if($net.chip -ne 'W5100' -or $net.udp -ne '5001' -or -not [Net.IPAddress]::TryParse($net.ip,[ref]$ip) -or $ip.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or $ip.Equals([Net.IPAddress]::Any)){$reasons.Add('valid W5100 UDP service/IP required')}
    if($net.rtr -cnotmatch '^[0-9A-F]{4}$' -or $net.rtr -in @('0000','FFFF') -or $end.expected_rtr -cne $net.rtr -or $end.rtr1 -cne $net.rtr -or $end.rtr2 -cne $net.rtr){$reasons.Add('before/after RTR preservation required')}
    if($end.status -cne 'PASS' -or $end.errors -ne '0' -or $end.cleanup -ne '1'){$reasons.Add('successful terminal result and file cleanup required')}
    if($LoadSeconds -lt $DurationSeconds){$reasons.Add('requested load interval not completed')}
    $sizes=@(16,32,64,128);[long]$total=0
    $minimum=[Math]::Max(24,$DurationSeconds*2)
    if($Probes.Count -lt $minimum){$reasons.Add("at least $minimum verified SD/UDP transactions required")}
    for($i=0;$i -lt $Probes.Count;$i++){
        $p=$Probes[$i];$length=$sizes[$i%4]
        if($p.code -cne 'PASS' -or $p.sequence -ne $i+1 -or $p.bytes -ne $length){$reasons.Add('UDP corruption/timeout/sequence/size failure')}
        $total += $length
    }
    foreach($name in @('rx','verified','tx')){if($end[$name] -ne [string]$Probes.Count){$reasons.Add("$name differs from exact host echoes")}}
    if($end.bytes -ne [string]$total){$reasons.Add('device/host byte totals differ')}
    if($Probes.Count){
        $last=New-Test09Payload -Length $sizes[($Probes.Count-1)%4] -Sequence $Probes.Count -Token $Token
        if($end.crc16 -cne (Get-Test09Crc $last)){$reasons.Add('terminal CRC mismatch')}
    }
    $stats=@($records | Where-Object kind -eq 'STAT')
    if($stats.Count -lt 2){$reasons.Add('at least two UART SRAM/statistics samples required')}
    [long]$lastRx=-1;[long]$lastMs=-1
    foreach($row in $stats){
        [long]$r=0;[long]$t=0;[long]$free=0;[long]$minimumFree=0
        if(-not [long]::TryParse($row.rx,[ref]$r) -or -not [long]::TryParse($row.elapsed_ms,[ref]$t) -or $r -lt $lastRx -or $r -gt $Probes.Count -or $t -le $lastMs -or $row.errors -ne '0' -or $row.verified -ne [string]$r -or $row.tx -ne [string]$r){$reasons.Add('invalid/reset/failing statistics')}
        if(-not [long]::TryParse($row.free,[ref]$free) -or -not [long]::TryParse($row.min_free,[ref]$minimumFree) -or $minimumFree -lt 512 -or $free -lt $minimumFree -or $free -gt 2048){$reasons.Add('low or invalid sampled SRAM')}
        $lastRx=$r;$lastMs=$t
    }
    [long]$initial=0;[long]$final=0;[long]$min=0;[long]$elapsed=0
    if(-not [long]::TryParse($end.initial_free,[ref]$initial) -or -not [long]::TryParse($end.free,[ref]$final) -or -not [long]::TryParse($end.min_free,[ref]$min) -or $min -lt 512 -or $initial -gt 2048 -or $initial -lt $min -or $final -gt 2048 -or $final -lt $initial-64 -or $final -lt $min){$reasons.Add('sampled RAM gate failed')}
    if(-not [long]::TryParse($end.elapsed_ms,[ref]$elapsed) -or $elapsed -lt $DurationSeconds*1000 -or $elapsed -gt ($DurationSeconds+20)*1000){$reasons.Add('device load duration invalid')}
    [pscustomobject]@{status=$(if($reasons.Count){'FAIL'}else{'PASS'});reasons=@($reasons | Select-Object -Unique);metrics=@{packets=$Probes.Count;bytes=$total;min_free=$min;elapsed_ms=$elapsed;ip=$net.ip;card_blocks=$blocks}}
}

function Invoke-Test09Udp {
    param([string]$IP, [int]$Port, [byte[]]$Data, [int]$TimeoutMs)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $udp = [Net.Sockets.UdpClient]::new(0)
    $code = 'PASS'; $detail = ''
    try {
        $udp.Client.ReceiveTimeout = $TimeoutMs
        $udp.Client.SendTimeout = $TimeoutMs
        $target = [Net.IPEndPoint]::new([Net.IPAddress]::Parse($IP), $Port)
        if ($udp.Send($Data, $Data.Length, $target) -ne $Data.Length) { throw 'SEND_SIZE' }
        $peer = [Net.IPEndPoint]::new([Net.IPAddress]::Any, 0)
        [byte[]]$reply = $udp.Receive([ref]$peer)
        if (-not $peer.Address.Equals($target.Address) -or $peer.Port -ne $Port) { $code = 'SOURCE' }
        elseif ($reply.Length -ne $Data.Length) { $code = 'LENGTH' }
        else {
            for ($j = 0; $j -lt $Data.Length; $j++) {
                if ($Data[$j] -ne $reply[$j]) { $code = 'CORRUPT'; $detail = "offset=$j"; break }
            }
        }
    } catch [Net.Sockets.SocketException] {
        $code = if ($_.Exception.SocketErrorCode -eq [Net.Sockets.SocketError]::TimedOut) { 'TIMEOUT' } else { 'SOCKET' }
        $detail = $_.Exception.SocketErrorCode.ToString()
    } catch { $code = 'ERROR'; $detail = $_.Exception.Message }
    finally { $udp.Dispose() }
    [pscustomobject]@{ Code = $code; Detail = $detail; Ms = $timer.Elapsed.TotalMilliseconds }
}

Export-ModuleMember -Function New-Test09Payload,Get-Test09Crc,ConvertFrom-Test09Line,Get-Test09Verdict,Invoke-Test09Udp


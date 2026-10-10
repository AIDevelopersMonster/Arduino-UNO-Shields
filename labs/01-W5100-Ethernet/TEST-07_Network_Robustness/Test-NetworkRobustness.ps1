<#
 TEST-07: phase-based UDP Echo reachability / RJ45 recovery / bounded soak.
 Run from PowerShell 7. Operator removes ONLY RJ45, never USB.
 No retransmission. Per-probe sequence validates the exact response.
#>
param(
 [Parameter(Mandatory=$true)][string]$IP,
 [int]$Port=5002,
 [ValidateRange(30,3600)][int]$SoakSeconds=60,
 [ValidateRange(250,5000)][int]$TimeoutMs=600
)
$ErrorActionPreference='Stop'
$remote=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse($IP),$Port)
$udp=[System.Net.Sockets.UdpClient]::new(0)
$udp.Client.ReceiveTimeout=$TimeoutMs
$seq=0
function Probe([string]$phase){
 $script:seq++
 [byte[]]$data=[byte[]]::new(16)
 $id=[BitConverter]::GetBytes([int]$script:seq)
 [Array]::Copy($id,0,$data,0,4)
 for($j=4;$j -lt 16;$j++){$data[$j]=[byte](($j*13+$script:seq)%256)}
 $clock=[System.Diagnostics.Stopwatch]::StartNew()
 [void]$udp.Send($data,$data.Length,$remote)
 $source=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any,0)
 try{[byte[]]$got=$udp.Receive([ref]$source)}
 catch [System.Net.Sockets.SocketException]{
  if($_.Exception.SocketErrorCode -eq [System.Net.Sockets.SocketError]::TimedOut){
   return [pscustomobject]@{Phase=$phase;OK=$false;Kind='TIMEOUT';Ms=$clock.ElapsedMilliseconds}
  }
  throw
 }
 $ok=$source.Address.Equals($remote.Address) -and $source.Port -eq $Port -and $got.Length -eq $data.Length
 if($ok){for($j=0;$j -lt $data.Length;$j++){if($data[$j] -ne $got[$j]){$ok=$false;break}}}
 return [pscustomobject]@{Phase=$phase;OK=$ok;Kind=$(if($ok){'OK'}else{'MISMATCH'});Ms=$clock.ElapsedMilliseconds}
}
function Phase([string]$name,[int]$count){
 $results=@()
 for($i=1;$i -le $count;$i++){
  $r=Probe $name;$results+= $r
  Write-Host "$name $i/$count : $($r.Kind) $($r.Ms)ms"
  Start-Sleep -Milliseconds 200
 }
 return ,$results
}
try {
 Write-Host "TEST-07 / UDP port=$Port / Arduino IP=$IP"
 Write-Host "PHASE A: Keep RJ45 connected."
 $a=Phase 'BASELINE' 10
 Read-Host 'Unplug RJ45 ONLY (keep USB power), then press Enter' | Out-Null
 $b=Phase 'OUTAGE' 10
 Read-Host 'Reconnect RJ45, wait for LINK light, press Enter' | Out-Null
 $new=Read-Host "Enter current Arduino IP if changed (Enter keeps $IP)"
 if(-not [string]::IsNullOrWhiteSpace($new)){
  $IP=$new.Trim()
  $remote=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse($IP),$Port)
 }
 $c=Phase 'RECOVERY' 30
 Write-Host "SOAK: $SoakSeconds seconds"
 $d=@();$watch=[System.Diagnostics.Stopwatch]::StartNew()
 while($watch.Elapsed.TotalSeconds -lt $SoakSeconds){
  $r=Probe 'SOAK';$d+= $r
  Write-Host "SOAK $($d.Count) : $($r.Kind) $($r.Ms)ms"
  Start-Sleep -Milliseconds 1000
 }
 $ag=@($a|Where-Object {$_.OK}).Count
 $bf=@($b|Where-Object {-not $_.OK}).Count
 $cg=@($c|Where-Object {$_.OK}).Count
 $last=@($c|Select-Object -Last 10|Where-Object {$_.OK}).Count
 $dg=@($d|Where-Object {$_.OK}).Count
 $ratio=if($d.Count -gt 0){$dg/$d.Count}else{0}
 Write-Host "SUMMARY baseline=$ag/10 outage_failures=$bf/10 recovery=$cg/30 recovery_last10=$last/10 soak=$dg/$($d.Count)"
 if($ag -eq 10 -and $bf -ge 8 -and $last -eq 10 -and $ratio -ge 0.95){
  Write-Host 'RESULT: PASS / bounded TEST-07 criteria met'
 }else{
  Write-Host 'RESULT: FAIL / examine phase results';exit 1
 }
}finally{$udp.Dispose()}

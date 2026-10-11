<# TEST-09B Ethernet + SD integration. No COM access. #>
param(
 [Parameter(Mandatory=$true)][string]$IP,
 [ValidateRange(1,65535)][int]$Port=5003,
 [ValidateRange(1,80)][int]$Count=40,
 [ValidateRange(500,10000)][int]$TimeoutMs=3000
)
$ErrorActionPreference='Stop'
$remote=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse($IP),$Port)
$udp=[System.Net.Sockets.UdpClient]::new(0)
$udp.Client.ReceiveTimeout=$TimeoutMs
$success=0; $fail=0
try {
 Write-Host "TEST-09B / UDP + microSD / silent COM / $IP port $Port"
 for($seq=1;$seq -le $Count;$seq++){
   [byte[]]$data=[byte[]]::new(24)
   $data[0]=75; $data[1]=57; $data[2]=68; $data[3]=58
   $data[4]=[byte]($seq -band 255)
   $data[5]=[byte](($seq -shr 8) -band 255)
   for($j=6;$j -lt 24;$j++){$data[$j]=[byte](($seq*29+$j*17)%256)}
   [void]$udp.Send($data,$data.Length,$remote)
   $sender=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any,0)
   try {
     [byte[]]$echo=$udp.Receive([ref]$sender)
     $ok=$sender.Address.Equals($remote.Address) -and $sender.Port -eq $Port -and $echo.Length -eq 24
     if($ok) {for($j=0;$j -lt 24;$j++){if($echo[$j] -ne $data[$j]){$ok=$false;break}}}
     if($ok){$success++;Write-Host "PASS seq=$seq length=24"}
     else{$fail++;Write-Host "FAIL seq=$seq data/source"}
   }catch [System.Net.Sockets.SocketException]{
     $fail++;Write-Host "TIMEOUT seq=$seq"
   }
   Start-Sleep -Milliseconds 100
 }
 [byte[]]$end=[System.Text.Encoding]::ASCII.GetBytes("K9END")
 [void]$udp.Send($end,$end.Length,$remote)
 $sender=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any,0)
 $report=''
 try {$report=[System.Text.Encoding]::ASCII.GetString($udp.Receive([ref]$sender))}
 catch [System.Net.Sockets.SocketException]{Write-Host "REPORT TIMEOUT"}
 Write-Host "HOST: $success/$Count correct, failures=$fail"
 Write-Host "DEVICE: $report"
 $fields=@{}
 if($report.StartsWith('K9RESULT ') -and $sender.Address.Equals($remote.Address) -and $sender.Port -eq $Port){
  foreach($part in ($report -split ' ' | Select-Object -Skip 1)){
   $kv=$part -split '=',2
   if($kv.Count -eq 2){$fields[$kv[0]]=$kv[1]}
  }
 }
 $pass=($fail -eq 0 -and $success -eq $Count -and
  $fields['eth'] -eq '1' -and $fields['sd'] -eq '1' -and
  $fields['rx'] -eq "$Count" -and $fields['echo'] -eq "$Count" -and
  $fields['wr'] -eq "$Count" -and $fields['rd'] -eq "$Count" -and
  $fields['err'] -eq '0' -and $fields['bytes'] -eq "$(24*$Count)")
 if($pass){Write-Host 'RESULT: PASS / Ethernet + SD data integrity'}
 else{Write-Host 'RESULT: FAIL / review counters';exit 1}
}finally{$udp.Dispose()}

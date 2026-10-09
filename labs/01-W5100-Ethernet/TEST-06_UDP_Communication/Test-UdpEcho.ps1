<#
 TEST-06 PowerShell UDP client.
 Verifies source, length and exact binary payload. No automatic retries.
#>
param(
  [Parameter(Mandatory=$true)][string]$IP,
  [ValidateRange(1,65535)][int]$Port=5001,
  [ValidateRange(1,100)][int]$Count=20
)
$ErrorActionPreference='Stop'
$remote=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse($IP),$Port)
$udp=[System.Net.Sockets.UdpClient]::new(0)
$udp.Client.ReceiveTimeout=1500
$passed=0; $failed=0; $timeouts=0
function Check-Echo([byte[]]$data,[string]$name) {
  $sent=$udp.Send($data,$data.Length,$remote)
  if($sent -ne $data.Length){throw "SEND SIZE mismatch $name"}
  $peer=[System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any,0)
  try { [byte[]]$echo=$udp.Receive([ref]$peer) }
  catch [System.Net.Sockets.SocketException] {
    if($_.Exception.SocketErrorCode -eq [System.Net.Sockets.SocketError]::TimedOut) {
      throw "TIMEOUT: $name"
    }
    throw
  }
  if(-not $peer.Address.Equals($remote.Address) -or $peer.Port -ne $Port) {
    throw "SOURCE mismatch $name"
  }
  if($echo.Length -ne $data.Length){throw "LENGTH mismatch $name"}
  for($j=0;$j -lt $data.Length;$j++) {
    if($echo[$j] -ne $data[$j]){throw "BYTE mismatch $name offset=$j"}
  }
  Write-Host "PASS $name bytes=$($data.Length)"
}
try {
  Write-Host "TEST-06 / UDP Echo / IP=$IP Port=$Port Count=$Count"
  foreach($len in @(1,16,64,128)) {
    [byte[]]$v=[byte[]]::new($len)
    for($i=0;$i -lt $len;$i++){$v[$i]=[byte](($i*37+11)%256)}
    try {Check-Echo $v "boundary-$len";$passed++}
    catch {$failed++;if("$_" -like "TIMEOUT:*"){$timeouts++};Write-Warning "$_"}
    Start-Sleep -Milliseconds 100
  }
  for($seq=1;$seq -le $Count;$seq++) {
    [byte[]]$v=[byte[]]::new(32)
    $v[0]=[byte]($seq -band 255)
    $v[1]=[byte](($seq -shr 8) -band 255)
    for($i=2;$i -lt 32;$i++){$v[$i]=[byte](($i+$seq*13)%256)}
    try {Check-Echo $v "seq-$seq";$passed++}
    catch {$failed++;if("$_" -like "TIMEOUT:*"){$timeouts++};Write-Warning "$_"}
    Start-Sleep -Milliseconds 100
  }
  $total=$Count+4
  if($failed -eq 0){Write-Host "RESULT: PASS / sent=$total verified=$passed failures=0 timeouts=0"}
  else {Write-Host "RESULT: FAIL / sent=$total verified=$passed failures=$failed timeouts=$timeouts";exit 1}
} finally {$udp.Dispose()}

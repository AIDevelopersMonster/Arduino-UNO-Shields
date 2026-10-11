param(
  [Parameter(Mandatory=$true)][string]$IP,
  [ValidateSet('status','on','off','toggle','all-on','all-off')][string]$Action='status',
  [ValidateRange(1,4)][int]$Channel=1
)

$ErrorActionPreference='Stop'
$base="http://$IP"

switch($Action){
  'status'  { $uri="$base/api/status" }
  'all-on'  { $uri="$base/api/all/on" }
  'all-off' { $uri="$base/api/all/off" }
  default   { $uri="$base/api/r$Channel/$Action" }
}

try {
  $s=Invoke-RestMethod -Uri $uri -Method Get -TimeoutSec 3
  [pscustomobject]@{
    IP=$s.ip
    Relay1=if($s.relay[0]){'ON'}else{'OFF'}
    Relay2=if($s.relay[1]){'ON'}else{'OFF'}
    Relay3=if($s.relay[2]){'ON'}else{'OFF'}
    Relay4=if($s.relay[3]){'ON'}else{'OFF'}
    Logic=if($s.active_low){'ACTIVE LOW'}else{'ACTIVE HIGH'}
    Uptime_s=$s.uptime_s
    Requests=$s.requests
  } | Format-List
} catch {
  Write-Error "Controller $IP is not responding: $($_.Exception.Message)"
  exit 1
}

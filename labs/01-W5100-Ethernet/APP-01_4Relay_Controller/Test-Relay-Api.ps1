param([Parameter(Mandatory=$true)][string]$IP)

$ErrorActionPreference='Stop'
$base="http://$IP"

function Get-State {
  Invoke-RestMethod -Uri "$base/api/status" -TimeoutSec 3
}

function Show-State([string]$Label) {
  $s=Get-State
  $v=@()
  for($i=0;$i -lt 4;$i++){ $v += $(if($s.relay[$i]){'ON'}else{'OFF'}) }
  Write-Host ("{0}: R1={1} R2={2} R3={3} R4={4}" -f $Label,$v[0],$v[1],$v[2],$v[3])
}

Write-Host "KON APP-01 API TEST / $IP"
Show-State 'START'

1..4 | ForEach-Object {
  $n=$_
  Invoke-RestMethod -Uri "$base/api/r$n/on" -TimeoutSec 3 | Out-Null
  Show-State "R$n ON"
  Invoke-RestMethod -Uri "$base/api/r$n/off" -TimeoutSec 3 | Out-Null
  Show-State "R$n OFF"
}

Invoke-RestMethod -Uri "$base/api/all/on" -TimeoutSec 3 | Out-Null
Show-State 'ALL ON'

Invoke-RestMethod -Uri "$base/api/all/off" -TimeoutSec 3 | Out-Null
Show-State 'ALL OFF'

Write-Host 'RESULT: SOFTWARE CONTROL SEQUENCE COMPLETE'

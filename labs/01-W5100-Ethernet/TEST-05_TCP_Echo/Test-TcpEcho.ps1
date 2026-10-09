<#
.SYNOPSIS
  TEST-05 TCP Echo client and reconnect verification for Arduino UNO W5100.
.DESCRIPTION
  Opens independent connections, verifies exact ASCII echo, injects one
  incomplete transfer and then confirms reconnection. Exits nonzero on error.
  Runs on Windows PowerShell 5.1 or PowerShell 7; no extra dependencies.
.PARAMETER IP
  Current DHCP address shown by the Arduino serial monitor.
.PARAMETER Port
  TCP server port; default 5000.
.PARAMETER Count
  Number of independent verified sessions; default 20.
#>
param(
  [Parameter(Mandatory=$true)][string]$IP,
  [int]$Port = 5000,
  [ValidateRange(1,100)][int]$Count = 20
)
$ErrorActionPreference = 'Stop'
$pass = 0
$failed = 0
function Invoke-Echo([string]$Payload) {
  $tcp = [System.Net.Sockets.TcpClient]::new()
  try {
    $task = $tcp.ConnectAsync($IP, $Port)
    if (-not $task.Wait(3000)) { throw "Connection timed out" }
    $tcp.ReceiveTimeout = 3000
    $tcp.SendTimeout = 3000
    $stream = $tcp.GetStream()
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($Payload + "`n")
    $stream.Write($bytes, 0, $bytes.Length)
    $buf = [byte[]]::new(80)
    $received = [System.Collections.Generic.List[byte]]::new()
    while ($received.Count -lt 80) {
      $n = $stream.Read($buf, 0, $buf.Length)
      if ($n -eq 0) { throw "Server closed before LF" }
      for ($j=0; $j -lt $n; $j++) {
        if ($buf[$j] -eq 10) {
          $actual = [System.Text.Encoding]::ASCII.GetString($received.ToArray())
          $expected = "ECHO " + $Payload
          if ($actual -cne $expected) { throw "Echo mismatch: expected '$expected', got '$actual'" }
          return
        }
        $received.Add($buf[$j])
      }
    }
    throw "Response exceeds 80 bytes"
  } finally {
    $tcp.Dispose()
  }
}
Write-Host "TEST-05 / TCP Echo / IP=$IP Port=$Port Count=$Count"
try {
  for ($i=1; $i -le $Count; $i++) {
    $payload = "KON-TCP-" + $i.ToString('D3')
    Invoke-Echo $payload
    $pass++
    Write-Host "PASS session $i : $payload"
  }
  Invoke-Echo ('X' * 64)
  $pass++
  Write-Host "PASS 64-byte boundary"
  # Deliberate incomplete line: ensure the server can recover from an aborted session.
  $aborted = [System.Net.Sockets.TcpClient]::new()
  try {
    $task = $aborted.ConnectAsync($IP,$Port)
    if (-not $task.Wait(3000)) { throw "Abort-test connection timed out" }
    $data = [System.Text.Encoding]::ASCII.GetBytes('ABORT-WITHOUT-NEWLINE')
    $s = $aborted.GetStream()
    $s.Write($data,0,$data.Length)
  } finally {
    $aborted.Dispose()
  }
  Write-Host "INFO aborted incomplete connection"
  Start-Sleep -Milliseconds 1800
  Invoke-Echo 'RECONNECT-OK'
  $pass++
  Write-Host 'PASS reconnect after abort'
  Write-Host "RESULT: PASS / verified=$pass, failures=0"
} catch {
  $failed++
  Write-Host "RESULT: FAIL / verified=$pass, failures=$failed"
  Write-Error $_
  exit 1
}

param(
  [string]$Port = 'COM4',
  [int]$Baud = 115200
)

$ErrorActionPreference='Stop'

$sp = [System.IO.Ports.SerialPort]::new($Port,$Baud,'None',8,'One')
$sp.NewLine = [Environment]::NewLine
$sp.ReadTimeout = 300
$sp.WriteTimeout = 1000
$sp.DtrEnable = $false
$sp.RtsEnable = $false

function Read-Available {
  Start-Sleep -Milliseconds 120
  $text = $sp.ReadExisting()
  if ($text) { Write-Host $text -NoNewline }
}

try {
  $sp.Open()
  Start-Sleep -Milliseconds 250
  Read-Available
  Write-Host "KON Relay Serial CLI / $Port @ $Baud"
  Write-Host "Commands: HELP, STATUS, R1 ON/OFF/TOGGLE ... R4, ALL ON/OFF"
  Write-Host "Type EXIT to quit."
  while ($true) {
    $cmd = Read-Host 'relay'
    if ($cmd.Trim().ToUpperInvariant() -eq 'EXIT') { break }
    if ([string]::IsNullOrWhiteSpace($cmd)) { continue }
    $sp.WriteLine($cmd)
    Read-Available
  }
} finally {
  if ($sp.IsOpen) { $sp.Close() }
  $sp.Dispose()
}

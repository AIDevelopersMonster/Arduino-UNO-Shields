param(
    [string]$Port = "COM4",
    [string]$Source = ".\labs\05-UNO-KON-OS\apps\KAP2\MULTI.KAP",
    [string]$Remote = "/HOSTAPP.KAP"
)

$ErrorActionPreference = "Stop"

$src = (Resolve-Path $Source).Path
[byte[]]$data = [System.IO.File]::ReadAllBytes($src)

function Get-Crc16Ccitt([byte[]]$Bytes) {
    [int]$crc = 0xFFFF
    foreach ($b in $Bytes) {
        $crc = $crc -bxor ([int]$b -shl 8)
        for ($i = 0; $i -lt 8; $i++) {
            if ($crc -band 0x8000) {
                $crc = (($crc -shl 1) -bxor 0x1021) -band 0xFFFF
            } else {
                $crc = ($crc -shl 1) -band 0xFFFF
            }
        }
    }
    return $crc
}

function Read-ProtocolLine {
    while ($true) {
        $line = $sp.ReadLine().Trim()
        if ($line.Length -eq 0) { continue }
        Write-Host $line
        return $line
    }
}

function Send-Expect([string]$Command, [string]$Pattern) {
    Write-Host "> $Command"
    $sp.WriteLine($Command)

    while ($true) {
        $line = Read-ProtocolLine
        if ($line -match '^@ERR') {
            throw "HOST1 error: $line"
        }
        if ($line -match $Pattern) {
            return $line
        }
    }
}

$crc = Get-Crc16Ccitt $data
$srcHash = (Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash
$out = Join-Path (Get-Location).Path "HOSTAPP.roundtrip.KAP"

Write-Host "SOURCE = $src"
Write-Host "SIZE   = $($data.Length)"
Write-Host ("CRC    = {0:X4}" -f $crc)
Write-Host "SHA256 = $srcHash"
Write-Host ""

$sp = [System.IO.Ports.SerialPort]::new(
    $Port,
    115200,
    [System.IO.Ports.Parity]::None,
    8,
    [System.IO.Ports.StopBits]::One
)

$sp.DtrEnable = $true
$sp.RtsEnable = $true
$sp.NewLine = [string][char]10
$sp.ReadTimeout = 10000

try {
    $sp.Open()
    Start-Sleep -Seconds 3
    $sp.DiscardInBuffer()

    [void](Send-Expect "@PING" '^@OK PONG HOST1$')
    [void](Send-Expect "@PUTB $Remote" '^@OK PUTB$')

    # Keep the complete PUTD line below the AVR HardwareSerial RX buffer.
    # 20 data bytes -> 40 hex chars; complete line including command/path/LF is
    # about 60 bytes, safely below the 64-byte ATmega328P RX buffer.
    $chunkSize = 20

    for ($off = 0; $off -lt $data.Length; $off += $chunkSize) {
        $count = [Math]::Min($chunkSize, $data.Length - $off)
        $hex = ""

        for ($i = 0; $i -lt $count; $i++) {
            $hex += $data[$off + $i].ToString("X2")
        }

        [void](Send-Expect "@PUTD $Remote $hex" '^@OK PUTD \d+$')
    }

    [void](Send-Expect (
        "@PUTE {0} {1} {2:X4}" -f $Remote, $data.Length, $crc
    ) '^@OK PUTE \d+ [0-9A-F]{4}$')

    Write-Host ""
    Write-Host "> @GET $Remote"
    $sp.WriteLine("@GET $Remote")

    $hexAll = New-Object System.Text.StringBuilder
    $beginSize = -1
    $endSize = -1
    $endCrc = ""

    while ($true) {
        $line = Read-ProtocolLine

        if ($line -match '^@ERR') {
            throw "HOST1 error: $line"
        }

        if ($line -match '^@BEGIN GET (\d+)$') {
            $beginSize = [int]$Matches[1]
            continue
        }

        if ($line -match '^@DATA ([0-9A-Fa-f]+)$') {
            [void]$hexAll.Append($Matches[1])
            continue
        }

        if ($line -match '^@END GET (\d+) ([0-9A-Fa-f]{4})$') {
            $endSize = [int]$Matches[1]
            $endCrc = $Matches[2].ToUpper()
            break
        }
    }

    [byte[]]$rx = @(
        for ($i = 0; $i -lt $hexAll.Length; $i += 2) {
            [Convert]::ToByte($hexAll.ToString($i, 2), 16)
        }
    )

    [System.IO.File]::WriteAllBytes($out, $rx)
    $rxHash = (Get-FileHash -LiteralPath $out -Algorithm SHA256).Hash

    Write-Host ""
    Write-Host "> @LS /"
    $sp.WriteLine("@LS /")

    while ($true) {
        $line = Read-ProtocolLine
        if ($line -match '^@ERR') {
            throw "HOST1 error: $line"
        }
        if ($line -match '^@END LS \d+$') { break }
    }
}
finally {
    if ($sp.IsOpen) { $sp.Close() }
}

Write-Host ""
Write-Host "BEGIN SIZE = $beginSize"
Write-Host "END SIZE   = $endSize"
Write-Host "RX SIZE    = $($rx.Length)"
Write-Host "END CRC    = $endCrc"
Write-Host "SRC SHA256 = $srcHash"
Write-Host "RX  SHA256 = $rxHash"

$match = (
    $beginSize -eq $data.Length -and
    $endSize -eq $data.Length -and
    $rx.Length -eq $data.Length -and
    $endCrc -eq ("{0:X4}" -f $crc) -and
    $srcHash -eq $rxHash
)

Write-Host "MATCH      = $match"

if (-not $match) {
    throw "TEST-09 transfer verification failed"
}

Write-Host ""
Write-Host "TEST-09 HOST1 TRANSFER ROUND-TRIP: PASS"
Write-Host "Remote file remains installed as $Remote"

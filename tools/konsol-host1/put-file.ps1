param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Remote,

    [string]$Port = "COM4"
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

        # KonSol human prompt has no trailing newline.
        # After DTR reset the first HOST1 reply can arrive as:
        # A:/> @OK PONG HOST1
        if ($line -match '^A:/> (@.*)$') {
            $line = $Matches[1]
        }

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

Write-Host "SOURCE = $src"
Write-Host "REMOTE = $Remote"
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

    # Generic installer uses a deliberately conservative payload.
    # 8 data bytes -> 16 hex chars, keeping a /BOOT.TXT PUTD record near
    # 33 bytes total. This gives substantially more margin for periods when
    # TFT/SD work delays the 1 ms Serial task on the ATmega328P.
    $chunkSize = 8

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
Write-Host "PUT FILE: PASS"
Write-Host "Installed $Remote"

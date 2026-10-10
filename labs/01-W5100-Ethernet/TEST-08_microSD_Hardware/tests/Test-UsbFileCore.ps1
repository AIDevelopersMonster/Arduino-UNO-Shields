#requires -Version 7.0
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot '../UsbSdProbe.cs')
$directory=Join-Path ([IO.Path]::GetTempPath()) ('usb-sd-core-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $directory
$path=Join-Path $directory 'probe.bin'
$cases=0
function Expect-Failure([scriptblock]$Action,[string]$Name) {
    $failed=$false
    try{& $Action | Out-Null}catch{$failed=$true}
    if(-not $failed){throw "Expected rejection: $Name"}
    $script:cases++
}
try {
    foreach($length in @(1,65535,65536,65537,16777216)){
        $written=[ArduinoUsbSdProbe]::Write($path,$length,12345,60)
        $read=[ArduinoUsbSdProbe]::Verify($path,$length,12345,60)
        $independent=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if($read.Bytes -ne $length -or $read.Sha256 -cne $written.Sha256 -or $read.Sha256 -cne $independent){throw 'Hash/length mismatch.'}
        $cases++
        Remove-Item -LiteralPath $path
    }
    $null=[ArduinoUsbSdProbe]::Write($path,65537,12345,60)
    $originalHash=(Get-FileHash -LiteralPath $path).Hash
    Expect-Failure {[ArduinoUsbSdProbe]::Write($path,65537,12345,60)} 'existing filename'
    if((Get-FileHash -LiteralPath $path).Hash -cne $originalHash){throw 'Existing file was modified.'}
    Expect-Failure {[ArduinoUsbSdProbe]::Verify($path,65537,54321,60)} 'wrong seed'
    $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite)
    try{
        $null=$stream.Seek(65536,[IO.SeekOrigin]::Begin)
        $old=$stream.ReadByte()
        $null=$stream.Seek(65536,[IO.SeekOrigin]::Begin)
        $stream.WriteByte([byte]($old -bxor 128))
    }finally{$stream.Dispose()}
    Expect-Failure {[ArduinoUsbSdProbe]::Verify($path,65537,12345,60)} 'data corruption at chunk boundary'
    $stream=[IO.File]::OpenWrite($path)
    try{$stream.SetLength(65536)}finally{$stream.Dispose()}
    Expect-Failure {[ArduinoUsbSdProbe]::Verify($path,65537,12345,60)} 'truncation'
    $stream=[IO.File]::OpenWrite($path)
    try{$stream.SetLength(65538)}finally{$stream.Dispose()}
    Expect-Failure {[ArduinoUsbSdProbe]::Verify($path,65537,12345,60)} 'extra data'
    Expect-Failure {[ArduinoUsbSdProbe]::Verify($path,65538,12345,-1)} 'expired phase budget'
    Write-Host "USB FILE CORE PASS / $cases cases; local temporary filesystem, no USB/card hardware result."
} finally {Remove-Item -LiteralPath $directory -Recurse}

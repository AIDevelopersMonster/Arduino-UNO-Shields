#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CompileJson,
    [string]$UploadPort,
    [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$encoding=[Text.UTF8Encoding]::new($false)

function Replace-Exact {
    param([string]$Text,[string]$Old,[string]$New,[int]$Count=1)
    $observed=[regex]::Matches($Text,[regex]::Escape($Old)).Count
    if ($observed -ne $Count) { throw "Unexpected driver layout: expected $Count occurrence(s) of $Old, found $observed" }
    return $Text.Replace($Old,$New)
}
function Get-Calls {
    param([string]$Text)
    $counts=[ordered]@{}
    foreach ($name in @('readMR','writeMR','readVERSIONR_W5200','readVERSIONR_W5500')) {
        $counts[$name]=[regex]::Matches($Text,'\b'+$name+'\s*\(').Count
    }
    return $counts
}

$compilePath=(Resolve-Path -LiteralPath $CompileJson).Path
# Verify the installed files before generating a separate diagnostic copy.
& (Join-Path $PSScriptRoot 'Verify-LibrarySources.ps1') -CompileJson $compilePath
$original=Get-Content -LiteralPath $compilePath -Raw -Encoding utf8 | ConvertFrom-Json
$ethernet=@($original.builder_result.used_libraries | Where-Object name -eq 'Ethernet')[0]
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../../../../..')).Path
if (-not $OutputDirectory) { $OutputDirectory=Join-Path $repo 'build/LibraryStartupTrace' }
$OutputDirectory=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
$work=Join-Path $OutputDirectory ((Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,6))
$libs=Join-Path $work 'libraries'; $binary=Join-Path $work 'binary'
$null=New-Item -ItemType Directory -Path $libs,$binary -Force
$overlay=Join-Path $libs 'Ethernet'
Copy-Item -LiteralPath $ethernet.install_dir -Destination $overlay -Recurse
$driverPath=Join-Path $overlay 'src/utility/w5100.cpp'
$before=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($driverPath))
$text=$before
$text=Replace-Exact $text '#include "w5100.h"' "#include `"w5100.h`"`n#include <LibraryStartupTrace.h>"
foreach ($chip in @(51,52,55)) {
    $text=Replace-Exact $text "chip = $chip;" "chip = $chip;`n`tlibraryStartupTraceBegin($chip);"
}
$text=Replace-Exact $text 'uint8_t mr = readMR();' "uint8_t mr = readMR();`n`t`tlibraryStartupTraceReset(mr);"
foreach ($item in @(@('08','LS_MR08',2),@('10','LS_MR10',3),@('12','LS_MR12',1),@('00','LS_MR00',3))) {
    $value=$item[0];$stage=$item[1]
    $old="if (readMR() != 0x$value) return 0;"
    $new="{ const uint8_t observed = readMR(); libraryStartupTraceObserve($stage, 0x$value, observed); if (observed != 0x$value) return 0; }"
    $text=Replace-Exact $text $old $new $item[2]
}
foreach ($item in @(@('5200',3),@('5500',4))) {
    $chip=$item[0];$expected=$item[1]
    $text=Replace-Exact $text "int ver = readVERSIONR_W$chip();" "int ver = readVERSIONR_W$chip();`n`tlibraryStartupTraceObserve(LS_VERSION, $expected, static_cast<uint8_t>(ver));"
}
foreach ($chip in @('5100','5200','5500')) {
    $old="//Serial.println(`"chip is W$chip`");"
    $text=Replace-Exact $text $old "$old`n`tlibraryStartupTraceDetected();"
}
$callsBefore=Get-Calls $before; $callsAfter=Get-Calls $text
if (($callsBefore|ConvertTo-Json -Compress) -ne ($callsAfter|ConvertTo-Json -Compress)) {
    throw 'Driver register call inventory changed; upload forbidden'
}
[IO.File]::WriteAllText($driverPath,$text,$encoding)
foreach ($file in @('LibraryStartupTrace.h','LibraryStartupTrace.cpp')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "trace/$file") -Destination (Join-Path $overlay "src/$file")
}

$cliArgs=@('compile','--fqbn','arduino:avr:uno','--warnings','all',
    '--build-path',$binary,'--library',$overlay,
    '--build-property','compiler.cpp.extra_flags=-DLIBRARY_STARTUP_TRACE=1','--json',$PSScriptRoot)
$json=@(& arduino-cli @cliArgs);$compileExit=$LASTEXITCODE
$jsonPath=Join-Path $work 'compile.json'
[IO.File]::WriteAllText($jsonPath,($json -join [Environment]::NewLine),$encoding)
if ($compileExit -ne 0) { throw "Trace compilation failed; see $jsonPath" }
$build=Get-Content -LiteralPath $jsonPath -Raw -Encoding utf8 | ConvertFrom-Json
if (-not $build.success) { throw 'Trace compiler did not report success' }
$platform=$build.builder_result.build_platform
if ($platform.id -ne $original.builder_result.build_platform.id -or $platform.version -ne $original.builder_result.build_platform.version) {
    throw 'Selected AVR platform changed; upload forbidden'
}
foreach ($name in @('SPI','Ethernet','SD')) {
    $chosen=@($build.builder_result.used_libraries | Where-Object name -eq $name)
    $prior=@($original.builder_result.used_libraries | Where-Object name -eq $name)[0]
    $expectedPath=if($name -eq 'Ethernet'){$overlay}else{$prior.install_dir}
    if ($chosen.Count -ne 1 -or $chosen[0].version -ne $prior.version -or
        [IO.Path]::GetFullPath($chosen[0].install_dir) -ne [IO.Path]::GetFullPath($expectedPath)) {
        throw "Unexpected selected $name library; upload forbidden"
    }
}
$sections=$build.builder_result.executable_sections_size
$flash=@($sections|Where-Object name -eq 'text');$ram=@($sections|Where-Object name -eq 'data')
if ($flash.Count -ne 1 -or $ram.Count -ne 1 -or $flash[0].size -gt 29000 -or $ram[0].size -gt 1536) {
    throw 'Trace build memory gate failed; upload forbidden'
}
$hexPath=Join-Path $binary 'LibraryStartup.ino.hex'
if (-not (Test-Path -LiteralPath $hexPath -PathType Leaf)) { throw 'Measured trace HEX missing; upload forbidden' }
$report=[ordered]@{
    test='LIBRARY_STARTUP_DETECTION_TRACE';status='BUILD_PASS';hardware='PENDING'
    platform=$platform;compile_json=$jsonPath;original_compile_json_sha256=(Get-FileHash -LiteralPath $compilePath -Algorithm SHA256).Hash
    overlay=$overlay;register_call_inventory_before=$callsBefore;register_call_inventory_after=$callsAfter
    patched_driver_sha256=(Get-FileHash -LiteralPath $driverPath -Algorithm SHA256).Hash
    sketch_sha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'LibraryStartup.ino') -Algorithm SHA256).Hash
    trace_helper_sha256=(Get-FileHash -LiteralPath (Join-Path $overlay 'src/LibraryStartupTrace.cpp') -Algorithm SHA256).Hash
    flash_bytes=$flash[0].size;static_sram_bytes=$ram[0].size;trace_buffer_bytes=61
    hex_sha256=(Get-FileHash -LiteralPath $hexPath -Algorithm SHA256).Hash
    scope='Buffered observation of existing Ethernet 2.0.2 detector reads; no added SPI calls. CPU/RAM/code-layout perturbation remains. Installed libraries unchanged.'
}
[IO.File]::WriteAllText((Join-Path $work 'trace-build.json'),($report|ConvertTo-Json -Depth 8),$encoding)
Write-Host "BUILD PASS / DETECTION TRACE / Flash=$($flash[0].size)/32256 / static SRAM=$($ram[0].size)/2048 / trace buffer=61 B / AVR=$($platform.version)" -ForegroundColor Green
Write-Host "Сборка и изолированная копия Ethernet: $work"
if ($UploadPort) {
    & arduino-cli upload --fqbn arduino:avr:uno --port $UploadPort --input-dir $binary $PSScriptRoot
    if ($LASTEXITCODE -ne 0) { throw 'Trace upload failed' }
    Write-Host 'Measured trace binary uploaded; hardware diagnosis remains PENDING.'
}

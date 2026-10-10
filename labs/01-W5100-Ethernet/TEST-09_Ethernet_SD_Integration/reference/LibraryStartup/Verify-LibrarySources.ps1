#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CompileJson,
    [string]$OutputDirectory=(Join-Path $PSScriptRoot 'runs')
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest

function Get-NormalizedSourceHash {
    param([Parameter(Mandatory)][string]$Path)
    $text=[Text.UTF8Encoding]::new($false,$true).GetString([IO.File]::ReadAllBytes($Path))
    $text=$text.TrimStart([char]0xFEFF).Replace("`r`n","`n").Replace("`r","`n").TrimEnd([char]10)
    $algorithm=[Security.Cryptography.SHA256]::Create()
    try {
        $bytes=$algorithm.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))
        return [BitConverter]::ToString($bytes).Replace('-','')
    } finally { $algorithm.Dispose() }
}

$compilePath=(Resolve-Path -LiteralPath $CompileJson).Path
$build=Get-Content -LiteralPath $compilePath -Raw -Encoding utf8 | ConvertFrom-Json
if ($build.success -ne $true) { throw 'A successful compile.json is required' }
$platform=$build.builder_result.build_platform
$manifestPath=Join-Path $PSScriptRoot 'LibrarySourceManifest.json'
$manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding utf8 | ConvertFrom-Json
if ($manifest.schema -ne 1) { throw 'Unsupported source manifest schema' }
if ($platform.id -ne 'arduino:avr' -or $platform.version -notin @($manifest.supported_avr_cores.PSObject.Properties.Name)) {
    throw 'Expected arduino:avr 1.8.6 or 1.8.8; no reference hashes for this platform'
}

$rows=[Collections.Generic.List[object]]::new()
$reasons=[Collections.Generic.List[string]]::new()
$selected=[Collections.Generic.List[object]]::new()
$expectedFiles=0
Write-Host 'Проверка исходников SPI / Ethernet / SD. Файлы библиотек только читаются.' -ForegroundColor Cyan

foreach ($package in $manifest.libraries) {
    $expectedFiles+=@($package.files).Count
    $before=$reasons.Count
    $libraries=@($build.builder_result.used_libraries | Where-Object name -eq $package.name)
    if ($libraries.Count -ne 1) {
        $reasons.Add("$($package.name): exactly one selected library required")
        Write-Host "SOURCE FAIL / $($package.name) / library selection missing or ambiguous" -ForegroundColor Red
        continue
    }
    $library=$libraries[0]
    $selected.Add([pscustomobject]@{name=$library.name;version=$library.version;install_dir=$library.install_dir})
    if ($library.version -ne $package.version) {
        $reasons.Add("$($package.name): expected version $($package.version), observed $($library.version)")
        Write-Host "SOURCE FAIL / $($package.name) / unexpected version" -ForegroundColor Red
        continue
    }
    $libraryRoot=[IO.Path]::GetFullPath($library.install_dir)
    if ($package.name -eq 'SPI') {
        $coreSpi=[IO.Path]::GetFullPath((Join-Path $platform.install_dir 'libraries/SPI'))
        if ($libraryRoot -ne $coreSpi) { $reasons.Add('SPI: selected directory is outside the selected AVR core') }
    }
    $expectedPaths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $package.files) {
        if ([IO.Path]::IsPathRooted($file.path) -or $file.path -match '(^|/)\.\.(/|$)') {
            throw 'Invalid relative path in source manifest'
        }
        $null=$expectedPaths.Add($file.path)
        $path=Join-Path $libraryRoot $file.path
        $actual='';$status='PASS';$detail=''
        try {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Missing file' }
            $actual=Get-NormalizedSourceHash -Path $path
            if ($actual -ne $file.normalized_sha256) { $status='FAIL';$detail='SOURCE_DIFF' }
        } catch { $status='FAIL';$detail=$_.Exception.Message }
        $rows.Add([pscustomobject]@{library=$package.name;path=$file.path;status=$status;expected_sha256=$file.normalized_sha256;actual_sha256=$actual;detail=$detail})
        if ($status -ne 'PASS') {
            $reasons.Add("$($package.name)/$($file.path): $detail")
            Write-Host "SOURCE FAIL / $($package.name)/$($file.path) / $detail" -ForegroundColor Red
        }
    }
    # Additional compilable sources/headers can change a library without changing its version.
    $sourceDir=Join-Path $libraryRoot 'src'
    if (Test-Path -LiteralPath $sourceDir -PathType Container) {
        foreach ($candidate in Get-ChildItem -LiteralPath $sourceDir -Recurse -File) {
            if ($candidate.Extension -notin @('.c','.cpp','.cxx','.cc','.s','.h','.hpp','.hh')) { continue }
            $relative=[IO.Path]::GetRelativePath($libraryRoot,$candidate.FullName).Replace('\','/')
            if (-not $expectedPaths.Contains($relative)) {
                $reasons.Add("$($package.name)/${relative}: UNEXPECTED_SOURCE")
                $rows.Add([pscustomobject]@{library=$package.name;path=$relative;status='FAIL';expected_sha256='';actual_sha256='';detail='UNEXPECTED_SOURCE'})
                Write-Host "SOURCE FAIL / $($package.name)/$relative / UNEXPECTED_SOURCE" -ForegroundColor Red
            }
        }
    }
    if ($reasons.Count -eq $before) {
        Write-Host "SOURCE PASS / $($package.name) $($package.version) / $(@($package.files).Count) files" -ForegroundColor Green
    }
}

$matched=@($rows | Where-Object status -eq 'PASS').Count
$pass=$reasons.Count -eq 0 -and $matched -eq $expectedFiles -and $rows.Count -eq $expectedFiles
$status=if($pass){'PASS'}else{'FAIL'}
$OutputDirectory=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
$null=New-Item -ItemType Directory -Path $OutputDirectory -Force
$reportPath=Join-Path $OutputDirectory ('source-audit-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,6)+'.json')
$report=[ordered]@{
    test='LIBRARY_SOURCE_AUDIT';status=$status;utc=[DateTime]::UtcNow.ToString('o')
    compile_json=$compilePath;compile_json_sha256=(Get-FileHash -LiteralPath $compilePath -Algorithm SHA256).Hash
    manifest_sha256=(Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash
    platform=$platform;selected_libraries=$selected.ToArray()
    normalization=$manifest.normalization;expected_files=$expectedFiles;matched_files=$matched
    reasons=$reasons.ToArray();files=$rows.ToArray()
    scope='Selected on-disk files match pinned official sources after declared text normalization. No hardware, full AVR-core, compiler-option, binary or library-correctness assessment.'
}
[IO.File]::WriteAllText($reportPath,($report|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
Write-Host "RESULT $status / LIBRARY SOURCE AUDIT / $matched/$expectedFiles files" -ForegroundColor $(if($pass){'Green'}else{'Red'})
Write-Host "Отчёт: $reportPath"
if (-not $pass) { throw 'Installed library sources differ or selection is invalid; see audit report' }

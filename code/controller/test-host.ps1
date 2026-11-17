[CmdletBinding()]
param([string]$Compiler='g++',[string]$JsonLibrary,[string]$OutputDirectory,[switch]$BuildOnly)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (!$JsonLibrary) { $JsonLibrary=Join-Path $repo '.local/n01/deps/ArduinoJson' }
if ((& git -C $JsonLibrary rev-parse HEAD) -ne '77771d3c07668e01d8f52acb03910c1110bb373f' -or
    (& git -C $JsonLibrary status --porcelain)) { throw 'Require a clean ArduinoJson 7.4.3 checkout' }
if (!$OutputDirectory) {
    $tag=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmss')+'_'+[guid]::NewGuid().ToString('N').Substring(0,8)
    $OutputDirectory=Join-Path $repo ".local/n01/host-builds/$tag"
}
$build=[IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $build) { throw 'OutputDirectory must be new; preserve frozen QA binaries' }
New-Item -ItemType Directory -Force -Path $build | Out-Null
$core=Join-Path $PSScriptRoot 'MiGardenController/src/core'
$sources=@(Get-ChildItem -LiteralPath $core -Filter '*.cpp' -File | ForEach-Object FullName)
$include=@('-I',$core,'-I',(Join-Path $JsonLibrary 'src'))
$compilerPath=(Get-Command $Compiler -CommandType Application).Source
$oldPath=$env:Path
try {
    # The installed MinGW compiler's child executables require its own DLLs first.
    $env:Path=(Split-Path $compilerPath)+[IO.Path]::PathSeparator+$env:Path
    foreach ($name in @('core_tests','sim')) {
        & $compilerPath -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -static @include @sources (Join-Path $PSScriptRoot "tests/$name.cpp") -o (Join-Path $build "$name.exe")
        if ($LASTEXITCODE) { throw "Compile failed: $name (exit $LASTEXITCODE)" }
    }
    if (!$BuildOnly) {
        & (Join-Path $build 'core_tests.exe') (Join-Path $repo 'contracts/zone-a/v1/vectors.json')
        if ($LASTEXITCODE) { throw 'Test failure' }
    }
    Write-Output "HOST_OUTPUT $build"
} finally {
    $env:Path=$oldPath
}

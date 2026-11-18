[CmdletBinding()]
param([string]$Compiler='g++',[string]$JsonLibrary,[Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if(!$JsonLibrary){$JsonLibrary=Join-Path $repo '.local/n01/deps/ArduinoJson'}
if((git -C $JsonLibrary rev-parse HEAD) -ne '77771d3c07668e01d8f52acb03910c1110bb373f' -or
   (git -C $JsonLibrary status --porcelain)){throw 'Require clean pinned ArduinoJson'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Output must be new; preserve receipts'}
New-Item -ItemType Directory -Path $out | Out-Null
$legacy=Join-Path $out 'legacy57b'
$fixtures=Join-Path $out 'fixtures'
New-Item -ItemType Directory -Path $legacy,$fixtures | Out-Null
$archive=Join-Path $out 'legacy57b.tar'
git -C $repo archive --format=tar --output=$archive 57b215660893d5413d1326d9cb69542ec0b46a43 -- code/controller/MiGardenController/src/core code/controller/tests/fixture.hpp
if($LASTEXITCODE){throw 'Pinned legacy source unavailable'}
tar -xf $archive -C $legacy
if($LASTEXITCODE){throw 'Legacy source extract failed'}
$compilerPath=(Get-Command $Compiler -CommandType Application).Source
$oldPath=$env:PATH
$env:PATH=(Split-Path $compilerPath)+[IO.Path]::PathSeparator+$env:PATH
try {
    $oldCore=Join-Path $legacy 'code/controller/MiGardenController/src/core'
    $newCore=Join-Path $PSScriptRoot 'MiGardenController/src/core'
    $includes=@('-I',(Join-Path $JsonLibrary 'src'))
    $argsBase=@('-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-static')
    $oldSources=@(Get-ChildItem -LiteralPath $oldCore -Filter '*.cpp' -File | ForEach-Object FullName)
    $sources=@(Get-ChildItem -LiteralPath $newCore -Filter '*.cpp' -File | ForEach-Object FullName)
    & $compilerPath @argsBase @includes -I $oldCore @oldSources (Join-Path $PSScriptRoot 'tests/legacy_reboot_fixture.cpp') -o (Join-Path $out 'legacy_fixture.exe')
    if($LASTEXITCODE){throw 'Legacy fixture compile failed'}
    & (Join-Path $out 'legacy_fixture.exe') $fixtures
    if($LASTEXITCODE){throw 'Legacy fixture generation failed'}
    & $compilerPath @argsBase @includes -I $oldCore -DMG_RECOVERY_BASELINE=1 @oldSources (Join-Path $PSScriptRoot 'tests/recovery_tests.cpp') -o (Join-Path $out 'baseline_recovery.exe')
    if($LASTEXITCODE){throw 'Baseline oracle compile failed'}
    & (Join-Path $out 'baseline_recovery.exe') 2>&1 | Tee-Object -FilePath (Join-Path $out 'expected-baseline-failure.log')
    if($LASTEXITCODE -ne 1){throw 'Expected missing recovery behavior did not FAIL on 57b'}
    foreach($name in @('recovery_tests','core_tests','sim')) {
        & $compilerPath @argsBase @includes -I $newCore @sources (Join-Path $PSScriptRoot "tests/$name.cpp") -o (Join-Path $out "$name.exe")
        if($LASTEXITCODE){throw "Compile failed: $name"}
    }
    & (Join-Path $out 'recovery_tests.exe') $fixtures
    if($LASTEXITCODE){throw 'Recovery regression failed'}
    & (Join-Path $out 'core_tests.exe') (Join-Path $repo 'contracts/zone-a/v1/vectors.json') --recovery-delta
    if($LASTEXITCODE){throw 'Affected core regression failed'}
    Write-Output "RECOVERY_OUTPUT $out"
} finally {$env:PATH=$oldPath}

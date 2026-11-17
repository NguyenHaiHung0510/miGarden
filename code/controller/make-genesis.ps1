[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('lab','release','both')][string]$Profile,
    [string]$Simulator,
    [string]$MkLittleFs
)
# Creates a local file only. This script never opens a device or writes a partition.
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (!$Simulator) { $Simulator=Join-Path $repo '.local/n01/host/sim.exe' }
if (!$MkLittleFs) {
    $installed=Get-Command mklittlefs -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($installed) { $MkLittleFs=$installed.Source }
    elseif ($env:LOCALAPPDATA) {
        $MkLittleFs=Join-Path $env:LOCALAPPDATA 'Arduino15/packages/esp32/tools/mklittlefs/4.0.2-db0513a/mklittlefs.exe'
    }
}
if (!$MkLittleFs -or !(Test-Path -LiteralPath $Simulator -PathType Leaf) -or
    !(Test-Path -LiteralPath $MkLittleFs -PathType Leaf)) {
    throw 'Specify the built host -Simulator and installed -MkLittleFs paths'
}
$sim=(Resolve-Path -LiteralPath $Simulator).Path
$MkLittleFs=(Resolve-Path -LiteralPath $MkLittleFs).Path
$stamp=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmss')+'_'+[guid]::NewGuid().ToString('N').Substring(0,8)
$out=Join-Path $repo ".local/n01/genesis/$($Profile)_$stamp"
$src=Join-Path $out 'blank'
$roundtrip=Join-Path $out 'unpacked'
New-Item -ItemType Directory -Force -Path $src,$roundtrip | Out-Null
$profiles=@($Profile)
if ($Profile -eq 'both') { $profiles=@('lab','release') }
foreach ($item in $profiles) {
    & $sim genesis (Join-Path $src "$item-v1.journal")
    if ($LASTEXITCODE) { throw 'Journal genesis failed' }
}
$img=Join-Path $out 'littlefs-genesis.bin'
Push-Location -LiteralPath $out
try {
    # On Windows the tool prepends './' to its unpack directory.
    & $MkLittleFs -c blank -p 256 -b 4096 -s 1441792 littlefs-genesis.bin
    if ($LASTEXITCODE -or (Get-Item -LiteralPath $img).Length -ne 0x160000) { throw 'Filesystem packing failed' }
    & $MkLittleFs -u unpacked -p 256 -b 4096 -s 1441792 littlefs-genesis.bin
    if ($LASTEXITCODE) { throw 'Filesystem unpack failed' }
} finally { Pop-Location }
foreach ($item in $profiles) {
    if ((Get-FileHash -LiteralPath (Join-Path $src "$item-v1.journal")).Hash -ne
        (Get-FileHash -LiteralPath (Join-Path $roundtrip "$item-v1.journal")).Hash) { throw 'Journal roundtrip mismatch' }
}
$hash=(Get-FileHash -LiteralPath $img).Hash.ToLowerInvariant()
$record=[ordered]@{artifact=$img; sha256=$hash; byte_count=0x160000; offset=0x290000;
    profile_files=$profiles; empty_config=$true; no_credentials_or_reports=$true;
    tool=( & $MkLittleFs --version) -join ' '; source_head=(& git -C $repo rev-parse HEAD);
    created_at=(Get-Date).ToString('o'); flashed=$false }
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding UTF8
Write-Output "OFFLINE GENESIS $img bytes=1441792 sha256=$hash"

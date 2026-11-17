[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('lab','release')][string]$Profile,
    [string]$Cli='C:\Users\os\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe',
    [string]$CliConfig='C:\Users\os\Desktop\ai_eng_path\migarden\.local\bench\S01\arduino-cli.yaml',
    [string]$JsonLibrary,
    [ValidateRange(1,8)][int]$Jobs=4
)
$ErrorActionPreference='Stop'
$nl=[Environment]::NewLine
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$staging=Join-Path $repo '.local/n01'
if (!$JsonLibrary) { $JsonLibrary=Join-Path $staging 'deps/ArduinoJson' }
$expected='77771d3c07668e01d8f52acb03910c1110bb373f'
if ((& git -C $JsonLibrary rev-parse HEAD) -ne $expected -or
    (& git -C $JsonLibrary status --porcelain)) { throw 'Require a clean ArduinoJson 7.4.3 checkout' }
if (!(Test-Path -LiteralPath $Cli -PathType Leaf) -or !(Test-Path -LiteralPath $CliConfig -PathType Leaf)) {
    throw 'Arduino CLI/config missing'
}
$head=& git -C $repo rev-parse HEAD
$fqbn='esp32:esp32:esp32:CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,PSRAM=disabled'
$id='n01_'+$Profile+'_'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmss')+'_'+[guid]::NewGuid().ToString('N').Substring(0,12)
$out=Join-Path $staging "build/$id"
foreach ($dir in @($out,(Join-Path $staging 'tmp'),(Join-Path $staging 'cache'),(Join-Path $staging 'downloads'),(Join-Path $staging 'arduino-user'))) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
}
$envKeys=@('TEMP','TMP','ARDUINO_BUILD_CACHE_PATH','ARDUINO_DIRECTORIES_DOWNLOADS','ARDUINO_DIRECTORIES_USER')
$saved=@{}
foreach ($k in $envKeys) { $saved[$k]=[Environment]::GetEnvironmentVariable($k,'Process') }
$env:TEMP=Join-Path $staging 'tmp'
$env:TMP=$env:TEMP
$env:ARDUINO_BUILD_CACHE_PATH=Join-Path $staging 'cache'
$env:ARDUINO_DIRECTORIES_DOWNLOADS=Join-Path $staging 'downloads'
$env:ARDUINO_DIRECTORIES_USER=Join-Path $staging 'arduino-user'
$utf8=New-Object System.Text.UTF8Encoding($false)
$f="-DMG_BUILD_ID=$id"
if ($Profile -eq 'lab') { $f+=' -DMG_LAB=1' }
$argsList=@('compile','--config-file',$CliConfig,'--fqbn',$fqbn,'--build-path',$out,
    '--library',$JsonLibrary,'--build-property',"compiler.cpp.extra_flags=$f",
    '--build-property','compiler.c.elf.extra_flags=-Wl,--wrap=esp_partition_erase_range,--wrap=esp_core_dump_write',
    '--warnings','all','--jobs',"$Jobs",(Join-Path $PSScriptRoot 'MiGardenController'))
function Hash-Entry([string]$path) {
    $item=Get-Item -LiteralPath $path
    [ordered]@{path=$item.FullName.Substring($repo.Length+1).Replace('\','/'); bytes=$item.Length;
        sha256=(Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$inputs=@(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'MiGardenController') -Recurse -File |
    Sort-Object FullName | ForEach-Object { Hash-Entry $_.FullName })
$inputs+=@(Get-ChildItem -LiteralPath (Join-Path $repo 'contracts/zone-a/v1') -File |
    Sort-Object FullName | ForEach-Object { Hash-Entry $_.FullName })
$inputs+=Hash-Entry $PSCommandPath
try {
    $version=(& $Cli version) -join ' '
    $installed=(& $Cli core list --config-file $CliConfig) -join $nl
    if ($installed -notmatch '(?m)^esp32:esp32\s+3\.3\.12\s') { throw 'Require installed Arduino-ESP32 3.3.12' }
    $start=(Get-Date).ToString('o')
    $old=$ErrorActionPreference
    $ErrorActionPreference='Continue'
    $compile=& $Cli @argsList 2>&1
    $exitCode=$LASTEXITCODE
    $ErrorActionPreference=$old
    [IO.File]::WriteAllText((Join-Path $out 'compile.log'),($compile -join $nl)+$nl,$utf8)
    $compile | Write-Output
    $artifacts=@()
    foreach ($name in @('MiGardenController.ino.bin','MiGardenController.ino.elf','MiGardenController.ino.map',
            'MiGardenController.ino.partitions.bin','partitions.csv','sdkconfig','build.options.json','flasher_args.json','compile.log')) {
        $path=Join-Path $out $name
        if (Test-Path -LiteralPath $path -PathType Leaf) { $artifacts+=Hash-Entry $path }
    }
    $gate='COMPILE_FAILED'
    $image=Join-Path $out 'MiGardenController.ino.bin'
    if ($exitCode -eq 0) {
        $raw=[IO.File]::ReadAllBytes($image)
        $body=[Text.Encoding]::ASCII.GetString($raw)
        $isLab=$body.Contains('MG-LAB-USB-PROTOCOL-1-N01')
        $partitions=Get-Content -Raw -LiteralPath (Join-Path $out 'partitions.csv')
        $map=Join-Path $out 'MiGardenController.ino.map'
        $guard=Select-String -LiteralPath $map -Pattern '^\s+0x[0-9a-f]+\s+__wrap_esp_partition_erase_range\s*$' -Quiet
        if ($raw.Length -gt 0x140000 -or $raw[0] -ne 0xe9 -or !$body.Contains($id) -or
            $isLab -ne ($Profile -eq 'lab') -or
            ($isLab -and (!$body.Contains('NETINFO LINK_PENDING') -or !$body.Contains('BLANK_FF'))) -or
            (!$isLab -and ($body.Contains('PROVISION_READY') -or $body.Contains('BOOTSTRAP_USB') -or
                           $body.Contains('NETINFO LINK_PENDING') -or $body.Contains('BLANK_FF'))) -or
            $partitions -notmatch '(?m)^app0,\s*app,\s*ota_0,\s*0x10000,\s*0x140000,?\r?$' -or
            $partitions -notmatch '(?m)^app1,\s*app,\s*ota_1,\s*0x150000,\s*0x140000,?\r?$' -or !$guard) {
            $gate='REJECTED_ARTIFACT'
        } else { $gate='PASS' }
    }
    $usbProtocol=$null
    if ($Profile -eq 'lab') { $usbProtocol=1 }
    $record=[ordered]@{
        build_id=$id; profile=$Profile; wire='migarden.zone-a.v1'; head_at_compile=$head
        toolchain=$version; esp32_core='3.3.12'; arduinojson_commit=$expected; command=@($Cli)+$argsList
        started_at=$start; ended_at=(Get-Date).ToString('o'); compile_exit=$exitCode; artifact_gate=$gate
        configured_flash_bytes=4194304; observed_physical_flash_bytes_parent=16777216
        app0=[ordered]@{start=0x10000; bytes=0x140000}; app1=[ordered]@{start=0x150000; bytes=0x140000}
        data=[ordered]@{start=0x290000; bytes=0x160000; mount='LittleFS (no format, no grow)'}
        usb_compatible=($Profile -eq 'lab' -and $gate -eq 'PASS'); usb_protocol=$usbProtocol
        gpio_outputs_in_profile=($Profile -eq 'release')
        device='NOT_RUN by builder'; bootloader_rollback='UNVERIFIED'
        source_inputs=$inputs; artifacts=$artifacts
    }
    $manifest=Join-Path $out 'manifest.json'
    [IO.File]::WriteAllText($manifest,($record|ConvertTo-Json -Depth 8)+$nl,$utf8)
    Write-Output "MANIFEST $manifest"
    if ($gate -ne 'PASS') { throw "$Profile artifact gate: $gate" }
    Write-Output "CANDIDATE $image"
} finally {
    foreach ($k in $envKeys) { [Environment]::SetEnvironmentVariable($k,$saved[$k],'Process') }
}

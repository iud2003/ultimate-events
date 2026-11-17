param([string]$PluginFolder = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/get-sdk.ps1"
cmake -S $root -B "$root/build/msvc" -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build "$root/build/msvc" --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed' }
if ($PluginFolder) {
    cmake --install "$root/build/msvc" --config Release --prefix $PluginFolder
    if ($LASTEXITCODE -ne 0) { throw 'Plugin installation failed' }
}

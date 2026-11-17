param([string]$Toolchain = 'C:/msys64/mingw64/bin')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$env:PATH = "$Toolchain;$env:PATH"
& "$PSScriptRoot/get-sdk.ps1"
cmake -S $root -B "$root/build/mingw" -G Ninja "-DCMAKE_C_COMPILER=$Toolchain/gcc.exe" "-DCMAKE_CXX_COMPILER=$Toolchain/g++.exe" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed' }
cmake --build "$root/build/mingw" --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
ctest --test-dir "$root/build/mingw" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Native checks failed' }

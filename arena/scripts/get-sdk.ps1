$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$sdk = Join-Path $root 'vendor/ffgl'
if (-not (Test-Path -LiteralPath $sdk)) {
    git clone https://github.com/resolume/ffgl.git $sdk
    if ($LASTEXITCODE -ne 0) { throw 'SDK clone failed' }
}
$expected = 'fda8d4a5904eaf09dd97ac0f95c246bd7f9f9a46'
$actual = git -C $sdk rev-parse HEAD
if ($actual -ne $expected) {
    git -C $sdk checkout $expected
    if ($LASTEXITCODE -ne 0) { throw 'Could not checkout the pinned FFGL 2.2 commit' }
}
Write-Host "Official SDK ready at $sdk ($expected)"

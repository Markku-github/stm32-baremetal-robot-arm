param(
    [string]$BuildDir = "build/bootstrap"
)

$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$toolchainFile = (Resolve-Path (Join-Path $PSScriptRoot "..\cmake\arm-none-eabi-toolchain.cmake")).Path
$fullBuildDir = [System.IO.Path]::GetFullPath((Join-Path $sourceDir $BuildDir))
$ninjaExe = (Get-Command ninja -ErrorAction Stop).Source

cmake -S $sourceDir -B $fullBuildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninjaExe" "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile"
exit $LASTEXITCODE
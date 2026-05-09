<#
.SYNOPSIS
Configure the firmware build directory on a Windows host.

.DESCRIPTION
This helper is currently intended for Windows PowerShell based development
environments. It uses repository-relative paths and does not rely on
hardcoded machine-specific absolute paths, but it is only validated on
Windows at this time.
#>

param(
    [string]$BuildDir = "build/bootstrap"
)

$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$toolchainFile = (Resolve-Path (Join-Path $PSScriptRoot "..\cmake\arm-none-eabi-toolchain.cmake")).Path
$fullBuildDir = [System.IO.Path]::GetFullPath((Join-Path $sourceDir $BuildDir))
$ninjaExe = (Get-Command ninja -ErrorAction Stop).Source

cmake -S $sourceDir -B $fullBuildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninjaExe" "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile"
exit $LASTEXITCODE
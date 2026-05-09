<#
.SYNOPSIS
Build the firmware target on a Windows host.

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
$fullBuildDir = [System.IO.Path]::GetFullPath((Join-Path $sourceDir $BuildDir))

if (-not (Test-Path $fullBuildDir)) {
    & (Join-Path $PSScriptRoot "configure.ps1") -BuildDir $BuildDir
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

cmake --build $fullBuildDir --target robot_arm_bootstrap
exit $LASTEXITCODE
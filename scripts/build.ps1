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
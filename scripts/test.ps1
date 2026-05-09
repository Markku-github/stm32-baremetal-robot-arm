<#
.SYNOPSIS
Build and run the host-native automated tests on a Windows host.

.DESCRIPTION
This helper is currently intended for Windows PowerShell based development
environments. It configures a dedicated host-test build directory with the
repository CMake entry point, builds the host-native test targets, and runs
them through CTest.
#>

param(
    [string]$BuildDir = "build/host-tests",
    [string]$Compiler = ""
)

$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$fullBuildDir = [System.IO.Path]::GetFullPath((Join-Path $sourceDir $BuildDir))
$cacheFile = Join-Path $fullBuildDir "CMakeCache.txt"

function Resolve-CompilerReference {
    param(
        [string]$CompilerReference
    )

    if (-not $CompilerReference) {
        return $null
    }

    if ([System.IO.Path]::IsPathRooted($CompilerReference) -or $CompilerReference.Contains("\\") -or $CompilerReference.Contains("/")) {
        if (-not (Test-Path $CompilerReference)) {
            return $null
        }

        return (Resolve-Path $CompilerReference).Path
    }

    $command = Get-Command $CompilerReference -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command) {
        return $null
    }

    return $command.Source
}

function Get-CompilerMachineTriple {
    param(
        [string]$CompilerPath
    )

    try {
        $machineTriple = & $CompilerPath -dumpmachine 2>$null | Select-Object -First 1
    }
    catch {
        return ""
    }

    if ($null -eq $machineTriple) {
        return ""
    }

    return $machineTriple.Trim().ToLowerInvariant()
}

function Test-SupportedHostCompiler {
    param(
        [string]$CompilerPath
    )

    $machineTriple = Get-CompilerMachineTriple -CompilerPath $CompilerPath
    if (-not $machineTriple) {
        return $false
    }

    if ($machineTriple.Contains("cygwin")) {
        return $false
    }

    if ($machineTriple.Contains("mingw") -or $machineTriple.Contains("windows")) {
        return $true
    }

    return $false
}

function Find-HostCompilerOnPath {
    $candidates = [ordered]@{}

    foreach ($compilerName in @("gcc.exe", "clang.exe")) {
        $commands = @(Get-Command $compilerName -All -CommandType Application -ErrorAction SilentlyContinue)
        foreach ($command in $commands) {
            $candidateKey = $command.Source.ToLowerInvariant()
            if (-not $candidates.Contains($candidateKey)) {
                $candidates[$candidateKey] = $command.Source
            }
        }
    }

    foreach ($candidate in $candidates.Values) {
        if (Test-SupportedHostCompiler -CompilerPath $candidate) {
            return $candidate
        }
    }

    return $null
}

$compilerPath = $null

if ($Compiler) {
    $compilerPath = Resolve-CompilerReference -CompilerReference $Compiler
    if (-not $compilerPath) {
        Write-Error "Could not resolve -Compiler '$Compiler' to an executable path. Pass a compiler name from PATH or an explicit path."
        exit 1
    }

    if (-not (Test-SupportedHostCompiler -CompilerPath $compilerPath)) {
        Write-Error "The selected compiler '$compilerPath' is not a supported Windows-native host compiler. Use MinGW-w64 GCC or Clang targeting Windows; Cygwin toolchains are intentionally rejected."
        exit 1
    }
}

if ((-not $compilerPath) -and $env:ROBOT_ARM_HOST_C_COMPILER) {
    $compilerPath = Resolve-CompilerReference -CompilerReference $env:ROBOT_ARM_HOST_C_COMPILER
    if (-not $compilerPath) {
        Write-Error "Could not resolve ROBOT_ARM_HOST_C_COMPILER='$env:ROBOT_ARM_HOST_C_COMPILER' to an executable path."
        exit 1
    }

    if (-not (Test-SupportedHostCompiler -CompilerPath $compilerPath)) {
        Write-Error "ROBOT_ARM_HOST_C_COMPILER points to '$compilerPath', but that compiler is not a supported Windows-native host compiler."
        exit 1
    }
}

if (-not $compilerPath) {
    $compilerPath = Find-HostCompilerOnPath
}

if (-not $compilerPath) {
    Write-Error "Could not find a supported Windows-native host C compiler in PATH. Add MinGW-w64 GCC or Clang targeting Windows to PATH, or pass -Compiler <compiler-name-or-path>."
    exit 1
}

$cmakeCompilerPath = $compilerPath.Replace("\", "/")

$shouldConfigure = -not (Test-Path $cacheFile)
if (-not $shouldConfigure) {
    $cachedCompilerLine = Get-Content $cacheFile | Select-String '^CMAKE_C_COMPILER:(FILEPATH|STRING)=' | Select-Object -First 1
    if (($null -eq $cachedCompilerLine) -or (($cachedCompilerLine.Line -replace '^CMAKE_C_COMPILER:(FILEPATH|STRING)=', '') -ne $cmakeCompilerPath)) {
        $shouldConfigure = $true
    }
}

if ($shouldConfigure) {
    if (Test-Path $fullBuildDir) {
        Remove-Item $fullBuildDir -Recurse -Force
    }

    $configureArgs = @(
        "-S", $sourceDir,
        "-B", $fullBuildDir,
        "-G", "Ninja",
        "-DROBOT_ARM_BUILD_HOST_TESTS=ON",
        "-DCMAKE_C_COMPILER=$cmakeCompilerPath"
    )

    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

cmake --build $fullBuildDir
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

ctest --test-dir $fullBuildDir --output-on-failure
exit $LASTEXITCODE
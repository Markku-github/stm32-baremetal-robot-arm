param(
    [string]$BuildDir = "build/bootstrap",
    [string]$FirmwareName = "robot_arm_bootstrap.elf",
    [string]$ProbeSerialNumber,
    [int]$ProbeIndex = 0,
    [int]$FrequencyKHz = 4000,
    [ValidateSet("NORMAL", "UR", "HOTPLUG", "POWERDOWN", "HWRSTPULSE")]
    [string]$ConnectionMode = "UR",
    [ValidateSet("SWrst", "HWrst", "Crst")]
    [string]$ResetMode = "HWrst",
    [switch]$ListProbes,
    [switch]$DryRun
)

function Resolve-ProgrammerCliPath {
    if ($env:STM32_PROGRAMMER_CLI) {
        if (Test-Path $env:STM32_PROGRAMMER_CLI) {
            return (Resolve-Path $env:STM32_PROGRAMMER_CLI).Path
        }

        throw "STM32_PROGRAMMER_CLI points to a missing file: '$env:STM32_PROGRAMMER_CLI'"
    }

    $command = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $registryPatterns = @(
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*'
    )

    $installRoots = New-Object System.Collections.Generic.List[string]

    foreach ($pattern in $registryPatterns) {
        $entries = Get-ItemProperty $pattern -ErrorAction SilentlyContinue |
            Where-Object { $_.DisplayName -match 'STM32CubeProgrammer|STM32CubeIDE' }

        foreach ($entry in $entries) {
            if (-not [string]::IsNullOrWhiteSpace($entry.InstallLocation)) {
                $installRoots.Add($entry.InstallLocation.Trim('"'))
            }
        }
    }

    foreach ($root in $installRoots | Select-Object -Unique) {
        if (-not (Test-Path $root)) {
            continue
        }

        $candidate = Get-ChildItem -Path $root -Recurse -File -Filter 'STM32_Programmer_CLI.exe' -ErrorAction SilentlyContinue |
            Select-Object -First 1 -ExpandProperty FullName

        if ($candidate) {
            return $candidate
        }
    }

    throw 'STM32_Programmer_CLI.exe was not found. Install STM32CubeProgrammer or STM32CubeIDE, or add STM32_Programmer_CLI.exe to PATH.'
}

$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$firmwarePath = [System.IO.Path]::GetFullPath((Join-Path (Join-Path $sourceDir $BuildDir) $FirmwareName))
$programmerCli = Resolve-ProgrammerCliPath

if ($ListProbes) {
    & $programmerCli -l stlink
    exit $LASTEXITCODE
}

if (-not (Test-Path $firmwarePath)) {
    throw "Firmware image was not found at '$firmwarePath'. Build the project first."
}

$arguments = @(
    '-c',
    'port=SWD',
    "freq=$FrequencyKHz",
    "index=$ProbeIndex",
    "mode=$ConnectionMode",
    "reset=$ResetMode"
)

if ($ProbeSerialNumber) {
    $arguments += "sn=$ProbeSerialNumber"
}

$arguments += @(
    '-w',
    $firmwarePath,
    '-v',
    '-rst'
)

if ($DryRun) {
    Write-Output $programmerCli
    Write-Output ($arguments -join ' ')
    exit 0
}

& $programmerCli @arguments
exit $LASTEXITCODE
# Bare-Metal 5-DoF Robotic Arm + Gripper Controller on STM32

This repository contains bare-metal firmware for an STM32 Nucleo-F767ZI based controller for a 5-DoF robotic arm with a separate gripper actuator. The codebase uses a command-line ARM GCC toolchain and direct register-level peripheral control.

## Current implementation status

The implemented baseline currently includes:

- repository scaffold for the planned firmware modules
- CMake and Ninja build configuration for `arm-none-eabi-gcc`
- PowerShell helper scripts for configure, build, and flashing
- freestanding startup and linker support
- HSI-based system initialization
- GPIO output support and Nucleo-F767ZI LD1 control
- USART6 transmit support for the external CH340 adapter
- interrupt-driven USART6 receive buffering with a simple echo test loop

## Hardware baseline

- target board: STM32 Nucleo-F767ZI
- debug LED: LD1 on `PB0`
- debug UART: `USART6`, `PG14` TX, `PG9` RX, `115200 8N1`
- application serial adapter: external CH340 USB-UART bridge
- flashing path: onboard ST-LINK over SWD via `STM32_Programmer_CLI.exe`

## Build requirements

- CMake 3.22 or newer
- Ninja
- GNU Arm Embedded Toolchain with `arm-none-eabi-gcc` in `PATH`
- STM32CubeProgrammer or STM32CubeIDE installed for `STM32_Programmer_CLI.exe`

## Build commands

Configure:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\configure.ps1
```

Build:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

List ST-LINK probes:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -ListProbes
```

Flash the current firmware through the onboard ST-LINK debugger:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1
```# Bare-Metal 5 DoF Robotic Arm Controller on STM32

This repository contains a bare-metal firmware project for the STM32 Nucleo-F767ZI board. The codebase is built with a command-line ARM GCC toolchain and uses direct register-level firmware code.

## Current status

The implemented baseline currently includes:

- repository scaffold for the planned firmware modules
- CMake and Ninja build configuration for `arm-none-eabi-gcc`
- PowerShell helper scripts for configure, build, and flashing
- freestanding startup and linker support
- HSI-based system initialization
- GPIO output support and Nucleo-F767ZI LD1 control
- USART6 transmit and interrupt-driven receive baseline for the external CH340 adapter

## Build requirements

- CMake 3.22 or newer
- Ninja
- GNU Arm Embedded Toolchain with `arm-none-eabi-gcc` in `PATH`
- STM32CubeProgrammer or STM32CubeIDE installed for `STM32_Programmer_CLI.exe`

## Build commands

Configure:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\configure.ps1
```

Build:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

List ST-LINK probes:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -ListProbes
```

Flash the current firmware through the onboard ST-LINK debugger:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1
```# Bare-Metal 5 DoF Robotic Arm Controller on STM32

This repository contains a bare-metal STM32 Nucleo-F767ZI robotic arm controller project for a 5 DoF arm with a separate gripper actuator. The firmware target is a CLI-driven workflow using CMake and Ninja, with CMSIS/register-level peripheral code instead of STM32 HAL as the main runtime layer.

## Current status

The repository is in bootstrap state. The first deliverable is V0 bring-up: repository scaffold, command-line build path, startup/linker integration, UART bring-up, I2C bring-up, and PCA9685 communication verification.

The original planning files are stored under `docs/` as reference snapshots and should not be edited directly during implementation.

## Working rules

- Build, flash, and debug from the command line.
- Keep runtime code bare-metal and register-level where practical.
- Use radians internally and degrees on the UART/user interface.
- Keep git history clean with small, demonstrable commits on focused feature branches.
- Do not falsify commit dates or rewrite history to simulate earlier progress.

## Bootstrap build

Configure:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\configure.ps1
```

Build:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

List ST-LINK probes:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -ListProbes
```

Flash the current firmware through the onboard ST-LINK debugger:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\flash.ps1
```

The current build produces a freestanding firmware ELF and the default flash path uses `STM32_Programmer_CLI` over ST-LINK SWD. The external CH340 adapter is reserved for the application UART path, not for flashing.

## Immediate next steps

1. Validate the ST-LINK flashing path on the physical Nucleo board.
2. Add startup code, linker script, and system clock initialization under `core/`.
3. Implement UART TX first, then interrupt-driven UART RX with a ring buffer.
4. Implement a minimal blocking register-level I2C transaction layer with timeouts.
5. Verify PCA9685 communication before any servo motion is enabled.
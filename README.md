# Bare-Metal 5-DoF Robotic Arm + Gripper Controller on STM32

This repository contains bare-metal firmware for an STM32 Nucleo-F767ZI based controller for a 5-DoF robotic arm with a separate gripper actuator. The codebase uses a command-line ARM GCC toolchain and direct register-level peripheral control.

## Current implementation status

The implemented V0 baseline currently includes:

- repository scaffold for the planned firmware modules
- CMake and Ninja build configuration for `arm-none-eabi-gcc`
- PowerShell helper scripts for configure, build, and flashing
- freestanding startup and linker support
- HSI-based system initialization
- GPIO output support and Nucleo-F767ZI LD1 control
- USART6 transmit support for the external CH340 adapter
- interrupt-driven USART6 receive buffering with a simple echo test loop
- I2C1 bring-up on `PB8`/`PB9` for the PCA9685 bus
- PCA9685 communication verification through a `MODE1` register smoke test at address `0x40`

## Hardware baseline

- target board: STM32 Nucleo-F767ZI
- debug LED: LD1 on `PB0`
- debug UART: `USART6`, `PG14` TX, `PG9` RX, `115200 8N1`
- PCA9685 test bus: `I2C1`, `PB8` SCL, `PB9` SDA, default address `0x40`
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
```
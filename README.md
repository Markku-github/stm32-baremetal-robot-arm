# Bare-Metal 5-DoF Robotic Arm + Gripper Controller on STM32

This repository contains bare-metal firmware for an STM32 Nucleo-F767ZI based controller for a 5-DoF robotic arm with a separate gripper actuator. The codebase uses a command-line ARM GCC toolchain and direct register-level peripheral control.

## Documentation

Detailed project guidance is tracked under `docs/`:

- [docs/README.md](docs/README.md)
- [docs/runtime_architecture.md](docs/runtime_architecture.md)
- [docs/testing_strategy.md](docs/testing_strategy.md)
- [docs/mvp_validation_coverage.md](docs/mvp_validation_coverage.md)
- [docs/development_workflow.md](docs/development_workflow.md)
- [docs/git_workflow.md](docs/git_workflow.md)
- [docs/documentation_guidelines.md](docs/documentation_guidelines.md)
- [docs/roadmap.md](docs/roadmap.md)

## Current implementation status

The current firmware baseline currently includes:

- repository scaffold for the planned firmware modules
- CMake and Ninja build configuration for `arm-none-eabi-gcc`
- PowerShell helper scripts for configure, build, and flashing
- freestanding startup and linker support
- HSI-based system initialization
- GPIO output support and Nucleo-F767ZI LD1 control
- USART6 transmit support for the external CH340 adapter
- interrupt-driven USART6 receive buffering with a line-based command shell
- I2C1 bring-up on `PB8`/`PB9` for the PCA9685 bus
- PCA9685 device initialization and PWM frequency setup on the custom I2C layer
- PCA9685 raw channel PWM and pulse-width based channel control helpers
- boot-time PCA9685 register self-test with readback verification and safe output disable
- servo abstraction on top of the PCA9685 driver
- conservative six-servo robot baseline with HOME and direct-pose support
- boot-time robot HOME and direct-pose integration self-tests with register readback validation
- runtime robot auto-HOME after boot-time self-tests complete
- repeatable MVP powered validation completed on the real target, with local evidence captured outside the tracked repository
- UART commands for `HELP`, `HOME`, `POSE`, `POSE_DELAY`, and `STATUS`
- host-native automated unit-test harness with CMake/CTest for servo logic

## Current UART command set

- `HELP`
- `HOME`
- `POSE <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>`
- `POSE_DELAY <delay_s> <base_deg> <shoulder_deg> <elbow_deg> <wrist_tilt_deg> <wrist_rotate_deg> <gripper_deg>`
- `STATUS`

## Current calibration anchors

The current robot-level calibration accepted for the MVP firmware is:

- base: `0..90 deg` mapped to `600..1800 us`, with HOME reported at `90 deg`
- shoulder: piecewise `0..180 deg` with `0 deg -> 1200 us`, `90 deg -> 2300 us`, `180 deg -> 3200 us`, with HOME reported at `0 deg`
- elbow: `0..180 deg` mapped to `450..2500 us`, with HOME reported at `180 deg`
- wrist_tilt: `0..180 deg` mapped to `2800..600 us` with reversed pulse endpoints, with HOME reported at `180 deg`
- wrist_rotate: `0..180 deg` mapped to `450..3000 us`, with HOME reported at `90 deg`
- gripper: `0..20 deg` mapped to `2450..1700 us` with reversed pulse endpoints for the current mechanism orientation, with HOME reported at `0 deg`

For later bring-up on similar servos in this same arm platform, these pulse windows are useful conservative starting points rather than universal limits. Current probing also established that pushing the elbow below `450 us` made the servo go limp on this mechanism; both `400 us` and `350 us` were rejected. Wrist-tilt probing on the reversed logical `0 deg` side remained stable through the currently accepted `2800 us` endpoint while preserving the accepted `600 us` logical `180 deg` side. Wrist-rotate probing accepted the current wider `450..3000 us` physical band as sufficient for the MVP even though it is not treated as a perfect endpoint-calibrated final range. The current accepted MVP HOME pose is `POSE 90 0 180 180 90 0`, and the firmware drives the robot to that HOME automatically during runtime startup after the boot-time self-tests complete.

## How to read angle, `us`, and `Hz` values

- UART commands, status output, and operator-facing examples use degrees because that is the clearest unit for commanding joint poses.
- Robot and servo calibration anchors use microseconds (`us`) because hobby-servo position is encoded by pulse width, not by changing the update frequency.
- The PCA9685 driver uses hertz (`Hz`) for the shared PWM frame rate because that value configures how often the pulse frame repeats for all channels.
- In practical terms, a typical servo frame of `50 Hz` is one `20 ms` cycle, and a command like `1500 us` means the signal stays high for `1.5 ms` inside each `20 ms` frame.
- Because of that split, entries such as `450..3000 us` in the calibration table describe the accepted pulse-width window for one joint, while the PCA9685 frequency remains a separate board-level setting.

## Hardware baseline

- target board: STM32 Nucleo-F767ZI
- debug LED: LD1 on `PB0`
- debug UART: `USART6`, `PG14` TX, `PG9` RX, `115200 8N1`
- PCA9685 test bus: `I2C1`, `PB8` SCL, `PB9` SDA, default address `0x40`
- application serial adapter: external CH340 USB-UART bridge
- flashing path: onboard ST-LINK over SWD via `STM32_Programmer_CLI.exe`

## Host tooling status

- the helper scripts in `scripts/` are currently Windows PowerShell tools for Windows development hosts
- `scripts/configure.ps1` and `scripts/build.ps1` use repository-relative paths and do not depend on hardcoded machine-specific absolute paths, but they are only validated in the Windows environment at this time
- `scripts/flash.ps1` is Windows-only as written because it resolves `STM32_Programmer_CLI.exe` through Windows PATH and registry discovery
- Linux host tooling parity is not implemented yet; matching Linux CLI helper scripts are planned as a future bonus item

## Windows host requirements

- CMake 3.22 or newer
- Ninja
- GNU Arm Embedded Toolchain with `arm-none-eabi-gcc` in `PATH`
- STM32CubeProgrammer or STM32CubeIDE installed for `STM32_Programmer_CLI.exe`

## Windows PowerShell commands

Preferred when local PowerShell policy allows direct repository-script invocation:

Configure:

```powershell
.\scripts\configure.ps1
```

Build:

```powershell
.\scripts\build.ps1
```

Run host-native automated tests:

```powershell
.\scripts\test.ps1
```

The host-test helper expects a Windows-native C compiler in `PATH` and intentionally rejects Cygwin toolchains.

If needed, override the detected Windows host compiler explicitly by compiler name or explicit path:

```powershell
.\scripts\test.ps1 -Compiler gcc.exe
```

List ST-LINK probes:

```powershell
.\scripts\flash.ps1 -ListProbes
```

Flash the current firmware through the onboard ST-LINK debugger:

```powershell
.\scripts\flash.ps1
```

If direct script invocation is blocked by the local PowerShell execution policy, use the explicit fallback form instead, for example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```
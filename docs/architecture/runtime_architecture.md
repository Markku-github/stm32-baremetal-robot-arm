# Runtime Architecture Guidance

## Purpose

This document defines the intended runtime-architecture direction for the STM32 bare-metal robotic arm controller.

Its role is to keep future refactors and feature branches aligned with a professional bare-metal design that stays readable, layered, and performance-aware without drifting toward hobby-style polling loops.

## Current baseline on `main`

The current firmware on `main` uses a hybrid runtime model:

- USART6 RX is interrupt-driven into a ring buffer.
- Command parsing and command execution run in main/control context.
- UART TX is currently blocking/polling.
- I2C transactions are currently blocking/polling.
- The main loop still includes an artificial busy-wait pacing delay.
- Servo pulse generation is correctly offloaded to the PCA9685 rather than being generated in software.

This baseline is acceptable for the current MVP direct-pose command path, but it is not the desired long-term runtime architecture.

## Target direction

The preferred architecture for this project is:

- bare-metal firmware
- interrupt-driven handling for asynchronous external events
- timer-driven periodic control work
- explicit state machines and bounded cooperative main/control-context work
- hardware offload where suitable, such as PCA9685-generated PWM

The project should avoid steady-state designs that depend on ad hoc busy loops for responsiveness or timing.

## Layering guidance

Keep the repository divided by responsibility from lower levels to higher levels.

The intended direction is:

1. CMSIS/register access, startup, and linker support
2. BSP and board support
3. device drivers such as the PCA9685 layer
4. generic servo control
5. robot-specific calibration and robot model data
6. application orchestration and command handling

Robot-specific behavior should not leak downward into generic driver layers.

### Architecture view

```text
Application orchestration and command handling
	|
Robot-specific calibration and robot model data
	|
Generic servo control
	|
Device drivers such as PCA9685
	|
BSP and board support
	|
CMSIS/register access, startup, and linker support
```

## Interrupt policy

Interrupts should be used aggressively for event capture, but conservatively for work content.

Good interrupt work in this project includes:

- capturing received UART bytes
- acknowledging hardware events
- setting flags
- pushing data into bounded buffers

Interrupt handlers should not become mini application executors.

In this document, main/control context means normal non-interrupt application execution.

Avoid inside ISRs:

- command parsing
- servo or robot command execution
- blocking UART transmit loops
- blocking I2C transactions
- long computations
- retry loops or recovery flows that belong in main/control context

## Timer and periodic-work policy

Any recurring control behavior should move toward an explicit timer- or tick-driven design rather than synthetic main-loop delays.

This especially applies to:

- smooth motion updates
- periodic state handling
- bounded retry or timeout supervision
- future command-rate or motion-rate control

The correct solution to future runtime-performance pressure is timer-driven control flow, not larger interrupt handlers.

## Busy-wait policy

The project should avoid hobby-style busy loops in steady-state runtime behavior.

Allowed exceptions are narrow and explicit:

- very short hardware stabilization waits during initialization
- bounded early bring-up waits that are isolated and documented
- temporary MVP-era compatibility code that already has a planned removal point

Busy-wait constructs should not become the permanent pacing mechanism for the operational control loop.

## Blocking-I/O policy

Blocking I/O is currently tolerated in some MVP paths because the firmware is still in the early direct-command stage.

However, the architectural direction is:

- isolate blocking I/O behind narrow interfaces
- keep blocking work out of interrupt context
- remove steady-state dependence on blocking control-path behavior as motion sophistication increases

This means the current blocking UART TX and blocking I2C implementation are not immediate MVP failures, but they are not the target end state for the control runtime.

## Observability and transport policy

V1 is expected to add structured observability rather than relying only on ad hoc shell prints.

The intended observability direction is:

- structured log levels such as `DEBUG`, `INFO`, `WARNING`, and `ERROR`
- build-time log filtering so lower-priority logs can be compiled out when appropriate
- a compact `STATUS` command that reports the current snapshot state instead of trying to act as a full history dump

On the Nucleo-F767ZI hardware used in this repository, the preferred V1 split is:

- operator command ingress on the existing application UART path
- log output on the ST-LINK virtual COM path when that board path is available cleanly

That split keeps transient log traffic away from the operator command channel without requiring a second external USB-UART adapter.

If the separated path is unavailable in a given environment, a fallback single-link mode is acceptable only if its limits are documented honestly.

Terminal color decisions should stay a host-side concern by default. The firmware should prefer tagged or structured severity output over terminal-specific formatting dependencies.

## Fault-handling policy

Unexpected faults should not disappear into a silent infinite loop in the intended V1 design.

The minimum acceptable V1 direction is:

- capture a small fault classification for unexpected exceptions
- preserve last reset or fault reason visibility where practical
- expose that information through boot reporting, focused status reporting, or both
- drive a distinct LED indication for latched fault states

Fault handlers still need to stay simple and bounded. Best-effort capture is the goal, not heavy formatting or recovery logic inside fault context.

## Current LED observability baseline

The early V1 observability baseline now routes basic runtime state indication through the three onboard Nucleo LEDs instead of a hard-coded single-LED heartbeat in `main`.

The current mapping is:

- startup or boot activity: LD2 blinking
- ready or idle: LD1 blinking
- degraded or not-ready runtime: LD1 steady on with LD3 blinking
- latched fault present from boot capture: LD3 steady on

This mapping is intentionally small. It is meant to make startup progress, ready state, degraded runtime, and latched-fault visibility distinguishable before the later timer-driven motion and richer runtime-state work arrives.

## Engineering-contract policy

The repository should document lightweight runtime criteria, not only behavioral descriptions.

For V1 this should include, where practical:

- the intended control tick frequency
- the expected motion update cadence
- any practical command-response upper bounds worth claiming

This does not require a heavy benchmarking subsystem. A small engineering contract supported by focused measurements or smoke checks is enough.

## Code quality and performance policy

The architecture is expected to stay readable as it evolves.

Prefer:

- descriptive function and variable names
- small, focused functions with clear ownership boundaries
- explicit interfaces between repository layers
- behavior-driven optimization only when measurement or real control needs justify it

Correctness, safety, determinism, and clarity come before premature optimization.

## Concurrency model guidance

The default concurrency model for this project is not an RTOS task model.

The preferred model is:

- bare-metal main/control flow
- interrupts for asynchronous ingress
- timer-driven periodic updates
- state machines for coordination

RTOS tasks or similar preemptive execution models should only be introduced if a later branch documents a concrete need that cannot be met cleanly with the simpler model above.

## Servo-control implication

Because the PCA9685 generates PWM autonomously, the firmware does not need software-timed pulse loops for servo control. That is a core architectural advantage of this project and should continue to be exploited.

The STM32 side should focus on:

- receiving commands
- validating and transforming them
- updating target outputs efficiently
- supervising state and timing cleanly

## Near-term application points

The current MVP baseline now keeps robot-specific calibration data in a dedicated robot-level module while leaving the generic servo layer free of robot-specific calibration tables.

The larger runtime-timing evolution should happen with early V1 control-foundation and smooth-motion work, where periodic timing semantics become part of the feature itself.

## Summary rule

Future branches should prefer this hierarchy:

1. hardware/peripheral offload when available
2. interrupts for bounded event capture
3. timer-driven periodic work for control updates
4. state-machine orchestration in main/control context

They should avoid using busy loops or oversized interrupt handlers as substitutes for a clear runtime architecture.
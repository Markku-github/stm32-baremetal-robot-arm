# Runtime Architecture Guidance

## Purpose

This document defines the intended runtime-architecture direction for the STM32 bare-metal robotic arm controller. Its role is to keep future refactors and feature branches aligned with a professional, performance-oriented embedded design instead of drifting toward hobby-style polling loops.

## Current baseline on `main`

The current firmware on `main` uses a hybrid runtime model:

- USART6 RX is interrupt-driven into a ring buffer.
- Command parsing and command execution run in thread context.
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
- explicit state machines and bounded cooperative thread-context work
- hardware offload where suitable, such as PCA9685-generated PWM

The project should avoid steady-state designs that depend on ad hoc busy loops for responsiveness or timing.

## Interrupt policy

Interrupts should be used aggressively for event capture, but conservatively for work content.

Good interrupt work in this project includes:

- capturing received UART bytes
- acknowledging hardware events
- setting flags
- pushing data into bounded buffers

Interrupt handlers should not become mini application threads.

Avoid inside ISRs:

- command parsing
- servo or robot command execution
- blocking UART transmit loops
- blocking I2C transactions
- long computations
- retry loops or recovery flows that belong in thread context

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

## Concurrency model guidance

The default concurrency model for this project is not RTOS threads.

The preferred model is:

- bare-metal main/control flow
- interrupts for asynchronous ingress
- timer-driven periodic updates
- state machines for coordination

RTOS tasks or thread-like execution models should only be introduced if a later branch documents a concrete need that cannot be met cleanly with the simpler model above.

## Servo-control implication

Because the PCA9685 generates PWM autonomously, the firmware does not need software-timed pulse loops for servo control. That is a core architectural advantage of this project and should continue to be exploited.

The STM32 side should focus on:

- receiving commands
- validating and transforming them
- updating target outputs efficiently
- supervising state and timing cleanly

## Planned adoption points

The intended sequencing for this guidance is:

1. `feature/test-harness-foundation`
2. `refactor/app-shell-split`
3. `refactor/helper-consolidation`
4. remaining MVP test backfill and system validation
5. `feature/v1-smooth-motion` for timer-driven runtime evolution

The main MVP refactor should improve structure and isolate the cooperative runtime flow.

The larger runtime-timing evolution should happen with V1 smooth-motion work, where periodic timing semantics become part of the feature itself.

## Summary rule

Future branches should prefer this hierarchy:

1. hardware/peripheral offload when available
2. interrupts for bounded event capture
3. timer-driven periodic work for control updates
4. state-machine orchestration in thread context

They should avoid using busy loops or oversized interrupt handlers as substitutes for a clear runtime architecture.
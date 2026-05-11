# Public Roadmap

## Purpose

This roadmap shows the main project phases, the current position, and the next planned milestones.

It is intentionally higher level than the local planning notes.

## Current position

The project has completed V0 bring-up and most of MVP direct-pose delivery.

The current focus is closing the remaining MVP refactor, powered validation, and closeout review work before opening V1.

## V0 - Bring-up and foundation

Completed:

- ~~Repository scaffold and CLI workflow baseline~~
- ~~ARM GCC CMake bootstrap build~~
- ~~Startup, linker, and early system initialization baseline~~
- ~~GPIO debug output and Nucleo-F767ZI LED support~~
- ~~USART6 transmit path~~
- ~~USART6 RX interrupt ring buffer~~
- ~~I2C1 bring-up and PCA9685 communication smoke test~~
- ~~Initial factual project documentation~~

## MVP - Direct-pose firmware baseline

Completed:

- ~~PCA9685 PWM control and safe output-disable behavior~~
- ~~Generic servo abstraction~~
- ~~Robot HOME and direct-pose baseline~~
- ~~Accepted MVP HOME pose update and runtime startup auto-HOME~~
- ~~Robot-specific calibration/config extraction into a dedicated robot-level module~~
- ~~UART command set: `HELP`, `HOME`, `POSE`, `POSE_DELAY`, `STATUS`~~
- ~~Boot-time PCA9685 and robot integration self-tests~~
- ~~Host-native unit and integration test harness~~
- ~~Command-shell and app-layer structural refactors~~
- ~~Six-joint mechanism-specific calibration anchors for base, shoulder, elbow, wrist tilt, wrist rotate, and gripper~~
- ~~README clarification for degrees, microseconds, and hertz~~
- ~~Tracked governance and workflow documentation under `docs/`~~

Current MVP closeout work:

- documented powered validation evidence
- final MVP closeout sync and broad review

Remaining before MVP is considered closed:

- perform repeatable powered validation and capture external evidence
- sync final MVP docs and justified residual gaps
- perform a final broad MVP review against the project rules and decisions

## V1 - Motion and runtime evolution

Planned:

- robot calibration module follow-up and later robot geometry/module-model work as needed
- timer-driven smooth motion support
- explicit runtime state handling
- additional command support around motion and recovery
- safer interruption and recovery validation

## V2 - Higher-level robot control

Planned:

- physical geometry freeze for kinematic work
- inverse kinematics for reachable targets
- Cartesian command path
- tool-orientation evolution within the platform limits
- waypoint or path support after direct Cartesian targets are stable

## Bonus

Possible later items:

- scripted pick-and-place demonstration
- expanded target-level automation where practical
- Linux host tooling parity
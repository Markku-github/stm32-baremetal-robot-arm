# Testing Strategy

## Purpose

This document defines the project-level testing architecture for the STM32 bare-metal robotic arm firmware. The goal is to make testing incremental, automatable where practical, and explicit about the remaining manual hardware checks.

## Adoption point

- The testing policy becomes active for new branches once the testing-policy documentation baseline is merged.
- The already-open branch `feature/mvp-uart-commands` is intentionally exempt from retroactive unit-test requirements.
- All later branches should follow the rules in this document unless an exception is explicitly documented in the branch itself.

## Testing goals

- Catch regressions close to the edited behavior.
- Prefer automated validation over manual validation whenever the project architecture allows it.
- Preserve manual hardware validation for target-specific behavior that cannot yet be automated safely or economically.
- Keep test evidence honest: distinguish unit, integration, system, and manual validation clearly.

## Test layers

### 1. Unit tests

Unit tests are the default requirement for new logic on future branches.

Use unit tests for:

- pure math and conversion helpers
- command parsing and argument validation
- state-transition logic
- servo and robot logic that can run with fakes instead of hardware access

Unit tests should:

- run on the host machine rather than on the STM32 target
- be wired into an automated test command
- stay narrow and behavior-focused
- use the repository host-test harness under `tests/` with CMake/CTest integration

### 2. Integration tests

Integration tests verify that multiple repository modules work together through their real interfaces while still avoiding full hardware dependence when possible.

Use integration tests for combinations such as:

- command shell plus robot API with faked UART or PCA9685 edges
- servo abstraction plus robot pose application
- self-test orchestration with stubbed board or driver seams

Integration tests are expected to grow after the initial unit-test harness exists.

### 3. System tests

System tests exercise the actual firmware behavior on the target board and attached hardware.

System-test scope includes:

- boot-time self-tests
- UART command-shell behavior on the real target
- powered servo motion flows
- error handling and recovery on the board

System tests should be automated when a practical harness exists. Until then they may remain manual, but the procedure must be documented.

### 4. Manual hardware tests

Manual testing is acceptable when hardware dependencies or safety constraints make automation impractical.

Every manual test procedure should state:

- required wiring, power, and safety preconditions
- exact command or action sequence
- expected UART output or physical behavior
- failure symptoms and safe recovery steps

Manual testing is not a waiver from automation forever. It is a documented fallback until an automated path becomes practical.

## Branch exit criteria

Before a branch is closed, verify the following for the touched behavior:

1. The firmware build is clean with the repository warning policy.
2. Relevant unit tests were added or updated when practical.
3. Relevant integration tests were added or updated when practical.
4. Feasible system-level checks were run.
5. If any required validation is still manual, the manual procedure and observed result are documented clearly.
6. Remaining test gaps are called out explicitly instead of being implied away.

## Coverage expectations

Until quantitative coverage tooling exists, coverage review is qualitative and change-focused:

- inspect the changed behavior
- identify the untested branches and error paths
- add tests for the meaningful gaps before branch closure

After a host-native test harness exists, add quantitative coverage reporting as a follow-up improvement rather than blocking the initial harness.

## Automation direction

The preferred automation path is:

1. Host-native unit-test harness under `tests/`
2. CMake/CTest integration for automated local execution
3. Optional CI automation for configure, build, and host tests
4. Gradual expansion toward automated integration and target-level system validation where practical

## Recommended test tree

The repository should grow toward this layout:

- `tests/unit/`
- `tests/integration/`
- `tests/system/`
- `docs/manual_tests/` or a clearly named equivalent manual-test location when procedures become numerous

## Test result collection

Generated test logs and local result artifacts should be collected under the ignored root directory `test_results/`.

Use one run directory per test invocation, named as:

- `YYYYMMDD_HHMMSS_<test-set>`

The `<test-set>` label should describe the executed suite clearly, for example:

- `host-ctest-full`
- `host-ctest-integration`
- `mvp-powered-validation`

Each run directory should contain the raw logs and a short summary that identifies:

- which script or command produced the run
- which test set was executed
- whether configure, build, and test phases passed or failed

Generated logs under `test_results/` are local evidence and should not be committed to the repository. Only durable procedures, summarized observed results, and justified residual gaps belong in tracked documentation.

## Planned rollout after the current UART branch

1. Finish `feature/mvp-uart-commands` without retrofitting this policy onto that already-open branch.
2. Create a dedicated test-harness branch to add host-native unit-test infrastructure and CTest wiring.
3. Use that harness to support the planned app-layer refactor before V1 motion/state work expands the surface further.
4. Add integration coverage for the command shell and robot-control seams as they stabilize.
5. Build out system and manual test procedures as powered motion and higher-level control features are introduced.
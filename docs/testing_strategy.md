# Testing Strategy

## Purpose

This document defines the project-level testing architecture for the STM32 bare-metal robotic arm firmware. The goal is to make testing incremental, automatable where practical, and explicit about the remaining manual hardware checks.

## Scope

These rules apply to current and future branches unless a concrete exception is documented explicitly.

## Testing goals

- Catch regressions close to the edited behavior.
- Prefer automated validation over manual validation whenever the project architecture allows it.
- Preserve manual hardware validation for target-specific behavior that cannot yet be automated safely or economically.
- Keep test evidence honest: distinguish unit, integration, system, and manual validation clearly.
- Use the test pyramid as the default shape of the test suite.
- Write tests in parallel with new features and refactors instead of deferring them to a later cleanup branch whenever practical.
- Exercise behavior from regression, black-box, white-box, and grey-box perspectives where each view adds unique value.
- Add boundary-value and error-path testing when the edited behavior has meaningful limits, clamps, retries, or failure handling.
- Use fault-injection-style testing where repository seams make it practical, especially around driver failures, communication errors, and recovery paths.
- Prefer meaningful coverage over duplicated tests that prove the same thing repeatedly.

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
- be added or updated in the same development slice as the behavior they cover when practical

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

## Current automated system-test status

The repository currently does not contain automated system tests.

That is a known limitation, not an implied hidden suite.

The current MVP plan treats target-level system validation as manual because:

- the project does not yet have a stable board-in-the-loop automation harness
- powered motion validation still carries real hardware and safety constraints
- the highest-value next testing work is to preserve the current host suite and extend it around upcoming V1 runtime changes

Automated system tests are still considered possible in this project, but they depend on a practical harness for scripting target interaction, observing results reliably, and doing so safely.

For the current post-closeout baseline, the plan is:

- keep unit and integration automation as the main automated safety net
- keep manual target-level system validation documented with clear procedures and summarized observed results for the hardware-dependent checks that still matter
- revisit automated system-test investment after MVP if a stable harness becomes worth the complexity

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
4. The full automated test suite available in the current environment passed before merge.
5. Feasible system-level checks were run.
6. If any required validation is still manual, the manual procedure and observed result are documented clearly.
7. Remaining test gaps are called out explicitly instead of being implied away.

## Coverage expectations

Until quantitative coverage tooling exists, the repository does not have line-, branch-, or function-coverage percentages as an automated gate.

Coverage review is therefore qualitative and change-focused:

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

The automated test tree should continue to grow toward this layout:

- `tests/unit/`
- `tests/integration/`
- `tests/system/` when an automated target-level harness becomes practical

## Manual validation documentation policy

Manual validation results and raw run logs do not belong in this repository.

The current default is:

- keep step-by-step operator procedures and raw observed results outside the tracked repository
- keep repository-tracked documentation focused on policy, scope, and durable test guidance

If the project later benefits from a tracked manual validation overview, keep it limited to a stable high-level case catalog or coverage map.

Do not turn the tracked repository into a storage location for filled result templates, operator run logs, or raw manual evidence.

## Test result collection

If test evidence is stored with this workspace, collect it under the ignored root directory `test_results/`.

Equivalent evidence may also be stored in a separate external results location, but it still must not be committed to this repository.

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

If the tested tree is later committed unchanged, add the resulting commit hash to the summary as a convenience link between the local evidence and the tracked history.

If the code changes after the recorded test run, rerun the tests instead of reusing the old result.

Generated test logs under `test_results/` are evidence that should be stored outside this repository. They must not be committed here.

Tracked documentation may reference summarized observed results, durable procedures, or justified residual gaps, but the raw result storage belongs outside this repository.

## Near-term testing priorities

1. Keep the current host-native suite healthy and fast enough to run before every branch merge.
2. Expand integration coverage around the robot, command, and startup/self-test seams as V1 runtime work lands.
3. Add targeted host-runnable tests around motion and state-handling behavior as those seams stabilize.
4. Revisit quantitative coverage tooling and target-level automation only after the current suite and harness direction are stable.
# Public Roadmap

## Purpose

This roadmap shows the main project phases, the current position, and the next planned milestones.

It is intentionally higher level than the local planning notes.

## Current position

The project has completed V0 bring-up and MVP direct-pose closeout.

The `v0.1.0` tag is the accepted MVP baseline.

The project is currently in a short pre-V1 transition phase focused on planning, facilitation, and environment improvements before officially opening V1 feature work.

After that transition, the next tracked feature phase is V1. Its first responsibility is to establish structured observability and a cleaner control foundation, then replace abrupt direct-pose jumps with a durable smooth-motion baseline and strengthen runtime control, operator workflow, and validation around that baseline.

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
- ~~Tracked MVP validation coverage map and evidence boundary~~
- ~~Repeatable powered validation execution with local external evidence capture~~
- ~~UART command set: `HELP`, `HOME`, `POSE`, `POSE_DELAY`, `STATUS`~~
- ~~Boot-time PCA9685 and robot integration self-tests~~
- ~~Host-native unit and integration test harness~~
- ~~Command-shell and app-layer structural refactors~~
- ~~Six-joint mechanism-specific calibration anchors for base, shoulder, elbow, wrist tilt, wrist rotate, and gripper~~
- ~~README clarification for degrees, microseconds, and hertz~~
- ~~Tracked governance and workflow documentation under `docs/`~~

Closed at `v0.1.0`.

## Pre-V1 transition - planning, facilitation, and environment improvements

This is a short transition between MVP closeout and official V1 feature work. It is intentionally not a robot-capability phase and it does not define its own planned version line or release baseline.

Focus:

- ~~shared workflow and environment improvements needed for clean V1 iteration~~
- finish roadmap, versioning, testing, and closeout-language cleanup so V1 opens from one internally consistent planning baseline
- reorganize the tracked `docs/` tree into clearer subdirectories once the roadmap work is finished, so V1-era documentation growth does not make navigation harder
- complete any small repository-facing facilitation work that materially improves day-to-day execution without pretending that V1 robot features have already started

Exit criteria:

- tracked planning and workflow guidance are internally consistent and no longer carry stale MVP-closeout or premature V1-opening language
- the tracked documentation layout is ready for additional V1-era material without avoidable navigation churn
- the shared automation and working environment are good enough to support V1 iteration cleanly
- the repository is ready to open V1 feature branches without treating this transition work itself as V1 scope

## V1 - Observability, safe motion, runtime control, and operator workflow

V1 begins after the pre-V1 transition above is complete. It is intended to make the current direct-pose system diagnosable, safe, and practical enough for frequent manual development and validation work. The early part of the phase is intended to establish structured observability and a cleaner bare-metal control foundation before the more invasive motion changes land. The smooth-motion path is intended to become the durable execution baseline through early V2 work rather than a throwaway prototype.

Testing is part of V1 itself, not a later side track. Each V1 slice should bring the test work needed to judge that slice honestly, rather than deferring validation until after the feature work appears complete.

Planned tagged baseline lines:

- `v0.2.x` - observability and control-foundation baseline
- `v0.3.x` - motion safety baseline
- `v0.4.x` - runtime state and recovery baseline
- `v0.5.x` - shell usability baseline
- `v1.0.0` - V1 closeout baseline

V1 feature and testing alignment:

- `v0.2.x`: open T1 around logging, fault visibility, and the early control-foundation seam; keep T2 limited to low-risk boot, status, and observability checks
- `v0.3.x`: open T1 motion-control host coverage and T2 manual powered motion-safety validation in parallel with the first smooth-motion work
- `v0.4.x`: expand T1 around state handling, timeout and fault supervision, and the focused recovery behavior that depends on the stronger runtime model
- `v0.5.x`: expand T1 shell regression coverage so operator-usability work reduces retest cost instead of increasing it
- V1 closeout review and validation: rerun and deepen the relevant T1 and T2 coverage, close justified evidence gaps, and decide whether `v1.0.0` is honest
- T3 exploration may begin late in V1 if the observability and runtime baselines are stable enough, but it is not a prerequisite for opening or completing the main V1 slices

### v0.2.x - Observability and control-foundation baseline

Planned:

- structured runtime logging with `DEBUG`, `INFO`, `WARNING`, and `ERROR` severities
- build-time log filtering so lower-priority logs can be compiled out of tighter builds
- a dedicated debug-log transport over the Nucleo ST-LINK VCP path when available, while keeping the operator command path separate on the application UART
- three-LED indication for boot, ready, activity, warning or degraded operation, and error states
- minimal unexpected-fault capture plus startup or runtime diagnostics, including last reset or fault reason visibility
- a cleaner bare-metal control foundation: timer or tick source, event handoff, and a documented engineering contract for control cadence and practical response bounds

Possible internal sub-phases:

- logging core: severity model, compile-time threshold, formatting contract, and a no-allocation implementation shape
- transport and host workflow: ST-LINK VCP logging plus the existing operator command UART, with a single-link fallback only if the separated path is unavailable
- board-enablement seam: explicit Nucleo-F767ZI mapping and BSP bring-up for LD1 `PB0`, LD2 `PB7`, LD3 `PB14`, `USART6` `PG14/PG9` operator commands, and the separated `USART3` ST-LINK VCP log path
- fault and visibility seam: latched unexpected-fault info, last reset or fault reason, focused `STATUS` snapshot behavior, and low-risk diagnostic coverage
- LED and control-foundation gate: three-LED state mapping, tick or event handoff, and validation that the early runtime contract is clear enough for later motion work
- engineering-contract gate: document control tick frequency, expected motion update cadence, and any practical command-response bounds needed for honest V1 claims

### v0.3.x - Motion safety baseline

Planned:

- timer- or tick-driven joint-space motion updates built on the V1 control foundation rather than synthetic main-loop pacing
- a motion controller that tracks current targets and bounded per-update progress
- conservative speed and interruption behavior so routine testing no longer depends on abrupt pose jumps
- a conservative default motion speed suitable for repeated manual validation and bench work
- explicit motion acceptance criteria for jerk, chatter, and in-motion interruption behavior
- safe `STOP` behavior that halts commanded motion without bypassing the main control architecture

Possible internal sub-phases:

- periodic motion seam: integrate the chosen tick source, ISR contract, and main-loop handoff into the motion path
- bounded motion core: current/target tracking, incremental updates, and deterministic completion rules for `HOME` and `POSE`
- operator-facing motion controls: conservative default speed, bounded user-selectable `SPEED`, `STOP`, and interruption semantics
- motion-quality gate: acceptance review against jerk/chatter criteria and the documented runtime-cadence contract, with the matching host and manual validation work

### v0.4.x - Runtime state and recovery

Planned:

- explicit `INIT`, `IDLE`, `HOMING`, `MOVING`, and `ERROR` states
- state-aware command acceptance, focused snapshot status reporting, and recovery behavior
- explicit interruption and recovery semantics around `STOP`, `HOME`, and `RESET_ERROR`
- timeout and fault supervision for recoverable versus latched failures
- runtime behavior aligned with the documented target direction: interrupt-driven ingress, timer-driven periodic work, and bounded cooperative main/control work with minimal ISR content

Possible internal sub-phases:

- state model definition: transition table, startup/homing path, and busy/error semantics
- command governance: allowed commands per state plus focused `STATUS` behavior
- error and recovery paths: `RESET_ERROR`, selected `HOME` recovery, explicit error reporting, and timeout supervision for recoverable versus non-recoverable failures
- runtime-control gate: host coverage and practical smoke checks around recovery behavior before closeout

### v0.5.x - Operator shell usability

Planned:

- command history on up/down arrows
- left/right cursor movement and mid-line editing
- safe handling of unsupported escape-sequence input
- preservation of the current line-based command protocol and parser boundary

Core v0.5.x scope is history/editing usability. Tab completion and similarly higher-complexity shell features stay deferred to a later follow-up if they still look justified after the core shell baseline is stable.

Possible internal sub-phases:

- terminal input foundation: escape-sequence parsing and safe fallback handling
- editable line buffer: cursor movement, insert/delete behavior, and redraw rules
- history and productivity features: command recall and repeat editing, while deferring tab completion and similarly tricky shell features to a later follow-up
- shell regression sweep: host-side shell cases that keep operator-quality improvements maintainable

### V1 closeout review and validation

Planned:

- rerun and extend host-native unit and integration coverage around motion, state handling, recovery, and shell behavior where needed to judge V1 readiness honestly
- continue local and GitHub-hosted automation for configure, build, and host tests as a routine safety net during closeout review
- perform limited board-level smoke automation or scripted register-level checks where practical
- if the earlier V1 baselines are stable enough, begin a low-risk T3 smoke harness around startup, logging, reset or fault reporting, status visibility, and other non-destructive checks without turning that work into a closeout blocker
- complete the remaining local manual powered validation needed to judge motion safety and runtime behavior honestly until a safer target-level harness exists
- review the full V1 completion criteria and any remaining justified hardware-only gaps before tagging `v1.0.0`
- if the closeout review finds missing V1 scope or weak validation, fix that work before `v1.0.0` instead of creating a separate validation version line

Possible internal sub-phases:

- criteria audit: compare the delivered V1 slices against the full closeout criteria and list any honest remaining gaps
- automated revalidation: rerun and deepen the relevant host and GitHub-hosted checks where the evidence is still weak
- manual and board-level evidence pass: finish the remaining motion-safety and runtime-control validation that still depends on real hardware judgment
- corrective loop and closeout decision: fix justified gaps first, then decide whether `v1.0.0` is warranted

V1 completion criteria:

- structured logging exists with documented severity levels and build-time filtering
- the preferred separated command and log paths are available on the target hardware baseline, or the fallback transport behavior is documented honestly
- `HOME` and `POSE` no longer rely on abrupt direct jumps in normal operation
- motion updates are timer/tick driven and the control or update contract is documented
- control tick frequency, expected motion update cadence, and any practical command-response bounds are documented at the engineering-contract level needed for V1
- default motion speed is conservative enough for routine manual validation unless intentionally changed by the operator
- normal commanded motion does not produce a violent startup jerk or sustained chatter in accepted operating conditions
- minimal fault capture, last reset or fault visibility, and LED indication make controller state and failures visible enough for iterative bench work
- runtime state handling governs command acceptance and recovery consistently
- `SPEED`, `STOP`, and `RESET_ERROR` are implemented, and `STATUS` provides the intended snapshot-level runtime view without trying to replace the log stream
- `STOP` transitions motion safely even when issued mid-motion
- shell usability is improved enough to support repeated manual testing productively
- host automation covers the main new runtime seams, and any remaining hardware-only gaps are explicitly documented

## V2 - Geometry, kinematics, and higher-level motion

V2 should build on the V1 motion/state baseline rather than bypassing it. New higher-level commands should still execute through the same documented safety and motion path.

Testing remains part of the phase here as well. New kinematic and path features should grow together with the reference tests and low-risk validation needed to trust them.

Planned tagged baseline lines:

- `v1.1.x` - geometry freeze and model groundwork
- `v1.2.x` - Cartesian target control baseline
- `v1.3.x` - orientation and path evolution baseline
- `v2.0.0` - V2 closeout baseline

The V2 slices below also use the planned tagged baseline lines directly so the roadmap has only one versioning scheme.

V2 feature and testing alignment:

- `v1.1.x`: expand T1 with geometry/model reference cases and keep T2 focused on low-risk smoke checks around startup, communication, and other non-destructive behavior
- `v1.2.x`: expand T1 with reachable, unreachable, and limit-sensitive Cartesian cases; use T2 cautiously for the first real Cartesian target-path validation where it is safe
- `v1.3.x`: expand T1 around orientation and path regressions; grow T3 only after direct Cartesian targets are stable enough to justify board-assisted regression work
- T4 remains later than the main V2 feature baselines and should not pull focus away from the earlier T1-T3 work needed to make V2 trustworthy

### v1.1.x - Geometry freeze and model groundwork

Planned:

- freeze coordinate-frame definitions, link lengths, offsets, and sign conventions
- document tool and wrist interpretation before orientation features are implemented
- keep joint calibration separate from geometry or model data

### v1.2.x - Cartesian target control

Planned:

- inverse kinematics for supported reachable targets
- a first Cartesian command path such as `GOTO <x_mm> <y_mm> <z_mm>`
- clean rejection of unreachable or unsafe targets
- reference test cases for reachable, unreachable, and joint-limit-sensitive targets

### v1.3.x - Orientation and path evolution

Planned:

- limited tool-orientation support within the platform's mechanical limits
- waypoint or linear Cartesian path support only after direct Cartesian targets are stable
- forward-kinematics or equivalent pose-tracking support when path planning needs it

V2 completion criteria:

- geometry assumptions are explicit and stable enough for durable kinematics work
- supported Cartesian targets execute through the V1 motion/state path
- impossible or unsafe targets fail cleanly
- documentation remains honest about pose-control limits, supported orientation behavior, and path semantics

## Testing roadmap

This testing roadmap cuts across V1, V2, and later phases.

The V1 and V2 sections above show where each testing stage should become active in the feature roadmap. The T1-T4 view below keeps the testing stream visible as its own cross-cutting work rather than as a separate afterthought.

### T1 - Host safety-net expansion

Focus:

- keep the current host suite fast and healthy
- add motion, state-machine, recovery, and shell tests as V1 seams stabilize
- continue local and GitHub-hosted configure, build, and host-test automation as a routine safety net

### T2 - Hardware smoke and evidence discipline

Focus:

- keep operator procedures and raw evidence under local-only storage such as `test_results/`
- add low-risk board-level smoke checks where practical
- continue manual powered validation for behaviors that still depend on real hardware judgment or safety supervision

### T3 - Board-assisted automation exploration

Focus:

- explore a stable board-in-the-loop or similar harness for repeatable low-risk regressions
- prefer startup, communication, self-test, and other non-destructive checks before aggressive motion automation
- treat this as late-V1 or early-V2 work, not as a blocker for opening the main V1 feature baselines after the pre-V1 transition

### T4 - Higher-cost target-level automation

Focus:

- expand target-level automation only after the runtime and kinematic interfaces are stable enough to support it
- revisit quantitative coverage tooling and larger automated system-validation ambitions once the harness direction is proven

## Later backlog and bonus

Possible later items:

- scripted pick-and-place demonstration
- expanded target-level automation after the earlier testing stages are stable
- Linux host tooling parity
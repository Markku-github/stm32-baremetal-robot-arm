# Development Workflow

## Purpose

This document defines the expected day-to-day workflow for building, testing, flashing, and documenting this repository.

The workflow is intentionally CLI-first and repository-script-first.

## Core rules

- Use the terminal for configure, build, test, flash, and diagnostics.
- Prefer the repository scripts under `scripts/` over ad hoc command variants when a standard script already exists.
- Keep validation and documentation updates close to the change that caused them.
- Add or update tests alongside new feature and refactor work when practical instead of deferring them to a later cleanup branch.
- Treat warnings as errors.

## Standard local loop

1. Configure the repository build outputs.
2. Build the firmware and host-test targets.
3. Add or update the relevant tests in parallel with the active change.
4. Run targeted automated checks while iterating on the active change.
5. Flash or perform manual hardware validation only when the touched behavior needs it.
6. Capture local evidence under `test_results/` when the run is worth preserving.
7. Before branch merge, run the full automated test suite available in the current environment.
8. Before branch merge, complete any required manual or hardware validation for the touched behavior.
9. Review `README.md`, affected files under `docs/`, and relevant source comments in the same branch as the change.

## Repository scripts

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

## Validation expectations

During development, narrow and targeted automated checks are encouraged so regressions are caught close to the edited behavior.

Before merging a branch, the merge candidate must pass the full automated test suite that is available in the current environment.

If the touched behavior still depends on real hardware, add the required manual or system-level validation before merge instead of relying on automation alone.

## GitHub-hosted automation

Tracked repository files under `.github/workflows/` may run GitHub-hosted checks for pushes to `main` and for manually triggered workflow runs.

These remote checks should mirror the repository validation policy where practical, but they do not replace the required local validation before merge.

## Flashing and runtime interfaces

The standard flashing path uses the onboard ST-LINK debugger over SWD.

The flashing interface and the application runtime UART interface should be treated as separate concerns in the workflow.

## Local-only artifacts

Keep generated local evidence under the ignored root directory `test_results/`.

Keep planning notes, diary entries, and other local working material under the ignored root directory `local_notes/`.

Neither directory should be committed to the repository.
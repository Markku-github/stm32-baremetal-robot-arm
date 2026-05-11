# MVP Validation Coverage

## Purpose

This document summarizes the stable validation coverage expected for the MVP direct-pose firmware baseline.

It keeps the tracked repository focused on durable validation scope and evidence boundaries rather than on raw manual run logs.

## Scope

This coverage map applies to the current MVP baseline on `main`, including:

- boot-time PCA9685 and robot integration self-tests
- runtime startup-HOME after the boot-time self-tests
- UART shell commands `HELP`, `HOME`, `POSE`, `POSE_DELAY`, and `STATUS`
- robot direct-pose behavior using the dedicated robot calibration module

## Automated coverage baseline

The automated merge-time baseline for this MVP firmware is the host-native CTest suite run through `scripts/test.ps1`.

The current automated suite covers:

- generic servo mapping, clamping, and PCA9685 error handling
- command parsing rules and invalid-input rejection
- visible command-handler behavior for `HELP`, `HOME`, `POSE`, `POSE_DELAY`, `STATUS`, and error cases
- UART shell line handling, buffer overflow handling, and transport-error recovery
- robot HOME behavior, direct-pose clamping, and joint pulse mapping
- runtime startup initialization plus automatic HOME orchestration
- shell-to-handler-to-robot command-path integration

This automated baseline is required before merge, but it does not replace powered mechanical validation on the real target.

## Manual MVP coverage map

The current MVP baseline still depends on real hardware validation for behavior that cannot be trusted from host-only automation alone.

### Boot readiness

Verify the boot-time self-test sequence and shell readiness with external servo power off.

Evidence expectation:

- summarized UART output confirming the self-test sequence
- confirmation that the runtime-ready message appears before the shell prompt

### Unpowered shell regression

Verify the visible UART shell behavior with external servo power off.

Evidence expectation:

- summarized pass or fail result for `HELP`, `HOME`, `STATUS`, the conservative `POSE` command, and representative invalid-command handling
- confirmation that reported pose values remain consistent with the accepted MVP HOME pose and the last successful conservative pose command

### Powered six-servo validation

Verify startup-HOME behavior, commanded HOME behavior, and a conservative direct-pose move with real servo power enabled.

Evidence expectation:

- summary of the first powered startup behavior after servo power is enabled
- summary of commanded `HOME` behavior and hold stability
- summary of the conservative `POSE 10 30 120 135 150 10` behavior
- explicit note of any unsafe motion, chatter, hard-stop contact, or direction mismatch

Detailed step-by-step operator procedures remain local-only material outside the tracked docs tree.

## Latest summarized observed result

One powered MVP validation run was completed on 2026-05-11 against `main` commit `04c7166` with local evidence captured outside the tracked repository.

Summarized result:

- boot self-tests and shell readiness passed
- unpowered UART shell regression passed
- powered startup-HOME, commanded `HOME`, and conservative `POSE 10 30 120 135 150 10` checks passed
- no unexpected motion, chatter, hard-stop contact, or direction mismatch was reported in the local run summary

Known limitation of that local summary:

- the stored hardware note recorded the use of 5 V lab power, but fuller hardware configuration details were not captured in the summary text

## Evidence collection boundary

Raw manual results, operator notes, UART captures, and other run artifacts must stay outside the tracked repository.

When evidence is stored with this workspace, keep it under:

- `test_results/YYYYMMDD_HHMMSS_mvp-powered-validation`

Each run summary should identify at least:

- tested commit
- test operator
- hardware configuration summary
- pass or fail result
- notable observations, failures, or residual safety concerns

If the tested tree is later committed unchanged, add the resulting commit hash to the local summary as a convenience link between the evidence and the tracked history.

## MVP closeout implication

This document provides the tracked high-level coverage map for MVP manual validation.

At least one repeatable powered validation run has now been executed on the real target and its local evidence has been captured outside the tracked repository.

Powered validation is therefore no longer the blocking MVP gap. The remaining MVP closeout work is the final tracked documentation sync and the final broad review.
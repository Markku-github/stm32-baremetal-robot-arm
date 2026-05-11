# Git Workflow

## Purpose

This document defines the repository-level git workflow for feature work, documentation work, testing branches, and refactors.

The goal is to keep `main` stable, branch scopes narrow, and the history easy to read afterward.

## Core rules

- Never push directly to `main`.
- Use one focused branch per coherent slice of work.
- Validate the slice on its own branch before merge.
- Merge accepted branches into `main` with `--no-ff` so branch boundaries remain visible in history.
- Keep commits small, honest, and testable.
- Use clear commit subjects that describe a real change.
- Treat warnings as errors across build, git, and development flow.

## Standard branch lifecycle

1. Branch from the current `main` baseline.
2. Choose a descriptive branch name with an approved top-level prefix.
3. Make small, reviewable commits while running targeted checks during development.
4. Before merge, run the full automated test suite available in the current environment.
5. Add any required manual or hardware validation for the touched behavior before merge.
6. Merge into `main` with `git merge --no-ff <branch-name>`.
7. Push the updated `main` branch after the accepted merge.

## Tags and releases

- Use version tags to mark durable baselines on `main` that should stay easy to find, compare, and revisit later.
- Create tags only after the relevant branch work has been merged and the chosen `main` commit has passed the required validation for that baseline.
- Use annotated tags rather than lightweight tags so the baseline has a clear human-readable message.
- Do not tag every commit or every branch merge. Most commits should remain untagged.
- Use GitHub releases only for milestone tags that deserve a durable human-readable summary, not for every tagged fix.

The default milestone points currently expected in this repository are:

- MVP closeout
- selected substantial pre-`1.0.0` capability baselines when they materially expand the firmware
- V1 closeout
- V2 closeout

## Version numbering guidance

Use tag names in the form `vMAJOR.MINOR.PATCH`.

- `MAJOR`: increment when the repository reaches a new named project-phase baseline or another intentionally major public baseline. For the current roadmap, `v1.0.0` is the expected V1 closeout tag and `v2.0.0` is the expected V2 closeout tag.
- `MINOR`: increment for a substantial new baseline within the current major line. Before `v1.0.0`, this is the main way to mark meaningful growth, for example `v0.1.0` for MVP closeout, `v0.2.0` for a substantial V1-era expansion, and `v0.3.0` for the next comparable step.
- `PATCH`: increment for a narrow corrective release on top of an already tagged baseline, for example `v0.1.1` or `v0.2.1`, when the goal is to preserve the same baseline while fixing defects, validation gaps, or similarly small follow-up issues.

Patch numbers are not used for every commit. They exist for occasional post-baseline correction releases, not for day-to-day development history.

## Tag and release flow

1. Merge the accepted branch work into `main`.
2. Confirm that the chosen `main` commit has the required automated and manual validation for the baseline being marked.
3. Create an annotated tag from that `main` commit, for example `git tag -a v0.1.0 -m "MVP closeout baseline"`.
4. Push the updated `main` branch and the new tag to the remote.
5. If the tag marks a milestone baseline, create a GitHub release from that tag with concise notes covering delivered scope, validation performed, remaining justified limits, and the next planned phase.

## Branch naming guidance

Keep the first path segment type-oriented and small in number.

Approved and recommended top-level branch prefixes are:

- `feature/`
- `docs/`
- `refactor/`
- `test/`
- `fix/`
- `build/`
- `chore/`
- `review/`
- `spike/`

Use the remainder of the branch name to describe the subsystem or goal clearly.

Examples:

- `feature/mvp-powered-validation`
- `docs/mvp-governance-foundation`
- `refactor/mvp-robot-calibration-module`

## Commit message guidance

Prefer the format `<prefix>: <imperative summary>` when a commit uses a prefix.

Common prefixes already used successfully in this repository include:

- `feat:`
- `docs:`
- `test:`
- `refactor:`
- `build:`
- `chore:`
- `scripts:`
- `bsp:`
- `core:`
- `drivers:`
- `robot:`
- `app:`

Commit subjects should:

- stay short and factual
- describe one dominant change
- avoid vague summaries such as `fix stuff` or `updates`

## History hygiene

- Do not mix unrelated changes into the same branch.
- Avoid rewriting published history unless a real correction is required.
- Keep branch cleanup separate from feature content when practical.
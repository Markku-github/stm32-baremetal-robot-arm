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
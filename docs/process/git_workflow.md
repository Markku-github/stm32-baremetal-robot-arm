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

When one roadmap baseline line such as `v0.2.x` is intentionally split into multiple durable published sub-baselines such as `v0.2.0`, `v0.2.1`, and `v0.2.2`, treat each published sub-baseline as its own coherent slice for branch purposes. Use one focused branch for each sub-baseline, merge that branch into `main`, and tag the resulting validated `main` commit. Do not keep one long-lived branch open across several published sub-baselines if each is meant to stand as its own reusable baseline.

## GitHub repository files

- Track repository-facing GitHub files under `.github/` when they define workflows or other repository behavior that should apply to everyone.
- Keep local tool-specific files out of tracked history when they are not part of the repository policy.
- Treat GitHub Actions checks as an additional safety net, not as a substitute for the local validation required before merge.

## Tags and releases

- Use version tags to mark durable baselines on `main` that should stay easy to find, compare, and revisit later.
- Every published named version baseline in this project should have an annotated git tag.
- Create tags only after the relevant branch work has been merged and the chosen `main` commit has passed the required validation for that baseline.
- Use annotated tags rather than lightweight tags so the baseline has a clear human-readable message.
- Do not tag every commit or every branch merge. Most commits should remain untagged.
- Use GitHub releases only for milestone tags that deserve a durable human-readable summary, not for every tagged fix.

By default in this repository:

- create a tag for each planned published version baseline
- create GitHub releases for `v0.1.0`, `v1.0.0`, and `v2.0.0`
- skip GitHub releases for intermediate baseline tags unless a later task explicitly decides otherwise

The default milestone points currently expected in this repository are:

- MVP closeout
- selected substantial pre-`1.0.0` capability baselines when they materially expand the firmware
- V1 closeout
- V2 closeout

## Version numbering guidance

Use tag names in the form `vMAJOR.MINOR.PATCH`.

- `MAJOR`: increment when the repository reaches a new named project-phase closeout baseline or another intentionally major public baseline. For the current roadmap, `v1.0.0` is the expected V1 closeout tag and `v2.0.0` is the expected V2 closeout tag.
- `MINOR`: increment for a substantial new baseline within the current major line. Before `v1.0.0`, this is the main way to mark meaningful growth, for example `v0.1.0` for MVP closeout, `v0.2.0` for a substantial V1-era expansion, and later `v0.10.0` or `v0.11.0` if many pre-V1-closeout baselines are needed. After `v1.0.0`, the same logic continues on the `v1.x.0` line for substantial V2-era baselines until V2 closeout.
- `PATCH`: increment for a narrow corrective release on top of an already tagged baseline, for example `v0.1.1`, `v0.3.1`, or `v1.2.1`, when the goal is to preserve the same baseline while fixing defects, validation gaps, or similarly small follow-up issues. The same patch field may also be used for a deliberately split published follow-up baseline inside one already-planned roadmap bucket, for example `v0.2.1` after `v0.2.0`, when that bucket intentionally spans more than one durable baseline.

Patch numbers are not used for every commit. They exist for occasional post-baseline correction releases, not for day-to-day development history.

Roadmaps and phase plans should usually describe version progression only at the baseline-line level, for example `v0.2.x`, `v0.3.x`, or `v1.2.x`, rather than pre-allocating exact patch numbers long before the work is finished.

When a roadmap slice maps to one planned tagged baseline line, name that slice with the baseline line itself, for example `v0.2.x` or `v1.1.x`, rather than introducing a second decimal label such as `V1.1` or `V2.1` for the same scope. Reserve `V1` and `V2` for the broad phase names.

Do not introduce a separate pre-closeout version line whose main purpose is to validate or review already-planned phase slices. When the remaining work is broad phase-closeout review and validation, use that review to decide whether the phase closeout tag such as `v1.0.0` is justified, rather than inventing another `v0.x.0` validation version.

### Practical interpretation for this repository

- `v0.1.0` is the expected MVP closeout baseline.
- While V1 is still in progress, substantial tracked baselines should normally stay on the `v0.x.0` line.
- There is no special upper limit at `v0.9.0`; version components are ordinary integers, so `v0.10.0`, `v0.11.0`, and higher are normal if the project needs them before V1 closeout.
- `v1.0.0` should mean that the planned V1 scope is intentionally declared complete, not merely that one large feature landed.
- While V2 is still in progress after `v1.0.0`, substantial tracked baselines should normally stay on the `v1.x.0` line.
- `v2.0.0` should follow the same logic for the V2 scope.

Examples:

- tag `v0.2.0` after V1 work has advanced enough to create a clearly stronger baseline than MVP, even if V1 is not fully complete yet
- tag `v0.10.0` if many substantial pre-V1-closeout baselines are needed before `v1.0.0`
- tag `v1.1.0` after the first substantial V2-era baseline once `v1.0.0` exists
- tag `v0.3.1` only if the already-tagged `v0.3.0` baseline needs a small corrective follow-up without redefining the baseline as a new milestone

### When to use a patch version

Use a patch tag when at least one of these controlled cases applies:

- the repository already has a tagged baseline that people may rely on, and the new work is mainly corrective rather than a new phase or major capability step
- one already-planned baseline line needs to be split deliberately into multiple durable sub-baselines after implementation begins, while still staying within the same higher-level roadmap bucket

Typical patch-tag cases include:

- a bug fix found shortly after a milestone tag
- a correction to validation or build wiring that does not materially change the project phase
- a narrowly scoped post-baseline documentation or packaging correction when it matters to the released baseline
- a consciously split follow-up baseline such as `v0.4.1` after `v0.4.0` when the same planned `v0.4.x` scope turns out to need more than one durable tagged step

Do not use a patch tag just because another commit was made. Most commits after a tag should remain ordinary history until there is a concrete reason to publish a corrected baseline.

### When not to create a tag

Do not create a new version tag for:

- ordinary in-progress feature commits
- every branch merge into `main`
- documentation-only cleanup that does not change the declared baseline
- intermediate states that are useful for development but not worth treating as a durable reference point
- a broad phase-closeout validation/review pass whose purpose is to decide whether the intended closeout tag is justified

If a change matters enough that future work, reports, or validation notes should refer to it as a named baseline, tagging is appropriate. If it does not meet that bar, an ordinary merge commit is enough.

## Tag and release flow

1. Merge the accepted branch work into `main`.
2. Confirm that the chosen `main` commit has the required automated and manual validation for the baseline being marked.
3. Create an annotated tag from that `main` commit, for example `git tag -a v0.1.0 -m "MVP closeout baseline"`.
4. Push the updated `main` branch and the new tag to the remote.
5. If the tag marks a milestone baseline, create a GitHub release from that tag with concise notes covering delivered scope, validation performed, remaining justified limits, and the next planned phase.

### Release-note content guidance

For milestone releases, keep the release notes concise and factual. The notes should normally include:

- what baseline the tag represents
- the main delivered capabilities or completed phase scope
- the validation summary at a high level
- the most important known limits that still remain by design
- the next intended project phase

Release notes should summarize stable project state. They should not duplicate raw local logs or local-only planning notes.

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
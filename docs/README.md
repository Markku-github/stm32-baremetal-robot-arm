# Documentation Index

## Purpose

This directory contains the tracked project documentation for durable architecture, testing, workflow, planning, and repository rules.

Use these files for long-lived project guidance.

Keep local planning notes, diary entries, manual operator procedures, and raw test evidence outside the tracked docs tree. When they are stored with this workspace, use the ignored `local_notes/` and `test_results/` directories instead of this directory.

## Topic layout

- `architecture/`: runtime design and technical architecture guidance
- `planning/`: roadmap and phase-planning material that belongs in tracked docs
- `process/`: workflow, git, and documentation-maintenance rules
- `validation/`: testing strategy and tracked validation-coverage summaries

## Document map

- [architecture/runtime_architecture.md](architecture/runtime_architecture.md): runtime model, layering, interrupt policy, timer-driven design direction, and blocking-work boundaries
- [validation/testing_strategy.md](validation/testing_strategy.md): test layers, merge-time validation rules, automation direction, and local evidence policy
- [validation/mvp_validation_coverage.md](validation/mvp_validation_coverage.md): high-level MVP validation coverage map, current summarized MVP validation status, and the boundary between tracked guidance and local run evidence
- [process/development_workflow.md](process/development_workflow.md): CLI-first day-to-day workflow, repository scripts, validation flow, and flashing path
- [process/git_workflow.md](process/git_workflow.md): branch lifecycle, merge policy, naming guidance, commit-message rules, and tag/release policy
- [process/documentation_guidelines.md](process/documentation_guidelines.md): what belongs in `README.md`, `docs/`, and `local_notes/`, plus documentation maintenance rules
- [planning/roadmap.md](planning/roadmap.md): public high-level project phases, current position, and next planned milestones

## Reader starting points

- Start with the repository `README.md` for the current status, hardware baseline, and quick commands.
- Read [planning/roadmap.md](planning/roadmap.md) for the high-level phase view of V0, MVP, V1, and V2.
- Read [validation/mvp_validation_coverage.md](validation/mvp_validation_coverage.md) for the current MVP validation scope, summarized observed result, and evidence boundary.
- Read [process/development_workflow.md](process/development_workflow.md) before day-to-day repository work.
- Read [process/git_workflow.md](process/git_workflow.md) before creating, naming, validating, merging, tagging, or releasing baselines.
- Read [architecture/runtime_architecture.md](architecture/runtime_architecture.md) and [validation/testing_strategy.md](validation/testing_strategy.md) before larger design, refactor, or testing changes.

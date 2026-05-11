# Documentation Index

## Purpose

This directory contains the tracked project documentation for durable architecture, testing, workflow, and repository rules.

Use these files for long-lived project guidance.

Keep local planning notes, diary entries, manual operator procedures, and raw test evidence outside the tracked docs tree. When they are stored with this workspace, use the ignored `local_notes/` and `test_results/` directories instead of this directory.

## Document map

- [runtime_architecture.md](runtime_architecture.md): runtime model, layering, interrupt policy, timer-driven design direction, and blocking-work boundaries
- [testing_strategy.md](testing_strategy.md): test layers, merge-time validation rules, automation direction, and local evidence policy
- [development_workflow.md](development_workflow.md): CLI-first day-to-day workflow, repository scripts, validation flow, and flashing path
- [git_workflow.md](git_workflow.md): branch lifecycle, merge policy, naming guidance, and commit-message rules
- [documentation_guidelines.md](documentation_guidelines.md): what belongs in `README.md`, `docs/`, and `local_notes/`, plus documentation maintenance rules
- [roadmap.md](roadmap.md): public high-level project phases, current position, and next planned milestones

## Reader starting points

- Start with the repository `README.md` for the current status, hardware baseline, and quick commands.
- Read [roadmap.md](roadmap.md) for the high-level phase view of V0, MVP, V1, and V2.
- Read [development_workflow.md](development_workflow.md) before day-to-day repository work.
- Read [git_workflow.md](git_workflow.md) before creating, naming, validating, or merging branches.
- Read [runtime_architecture.md](runtime_architecture.md) and [testing_strategy.md](testing_strategy.md) before larger design, refactor, or testing changes.
# Documentation Guidelines

## Purpose

This document defines how documentation is split across the repository and how it should be maintained as the project evolves.

The goal is to keep the tracked documentation professional, current, and easy to navigate.

## Documentation split

### Repository `README.md`

Use `README.md` as the root entry point for the project.

Keep it focused on:

- project summary
- current status at a glance
- quick configure, build, test, and flash commands
- hardware baseline summary
- current command-set summary
- concise current calibration summary when it still helps new readers
- links into `docs/`

### `docs/`

Use `docs/` for durable tracked guidance such as:

- architecture guidance under topic subdirectories such as `docs/architecture/`
- planning guidance under topic subdirectories such as `docs/planning/`
- process guidance under topic subdirectories such as `docs/process/`
- validation guidance under topic subdirectories such as `docs/validation/`
- runtime architecture
- testing policy
- development workflow
- git workflow
- documentation rules
- other long-lived technical or process guidance that should travel with the repository

### `local_notes/`

Use the ignored `local_notes/` directory for local-only material such as:

- planning notes
- working hypotheses
- project diary entries
- operator procedures
- manual run notes
- raw hardware observations

Content from `local_notes/` should be rewritten before promotion into tracked documentation.

## Quality rules

Tracked documentation should be:

- professional in tone
- factual and technically precise
- concise without becoming vague
- written in English
- kept free of machine-specific absolute filesystem paths

## Source comments and API documentation

Code comments are part of the project documentation and should follow the same quality bar as the tracked markdown documents.

The current repository style uses Doxygen-style block comments for file headers and for functions where the comment adds real value.

The expected pattern is:

- file-level blocks with tags such as `@file` and `@brief`
- function-level blocks with tags such as `@brief`, `@param`, and `@retval` when the interface benefits from explicit documentation
- concise English wording
- professional tone with no filler comments

Prefer comments that explain intent, interface contracts, non-obvious behavior, or important boundaries.

Avoid comments that merely restate obvious code line by line.

## Maintenance rules

- Review `README.md` and the relevant files under `docs/` whenever a branch changes behavior, structure, workflow, or validation expectations.
- Review and update `docs/planning/roadmap.md` whenever project progress changes the current position, remaining MVP closeout work, or planned milestones.
- Update documentation in the same branch as the feature, refactor, or policy change it describes.
- Review source comments and public API comments when a branch changes behavior or interfaces, because code comments are part of the maintained documentation set.
- Treat large documentation cleanups and policy clarifications as real implementation work, not as optional post-facto polish.
- Keep duplicate explanations to a minimum; prefer one canonical detailed document and lighter links elsewhere.

## Promotion rule for local notes

When information is promoted from `local_notes/` into tracked documentation:

- remove local-only scratch phrasing
- remove operator-specific working details that do not belong in the repository
- rewrite the content into durable project guidance
- check that the tracked version does not contradict the current code or workflow
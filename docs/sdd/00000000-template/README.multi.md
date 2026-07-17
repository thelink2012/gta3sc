---
inherits: docs/sdd
spec_id: yyyymmdd-slug
title: Spec title
status: draft
doc_role: slice-index
---

# Spec: <title>

**Status:** draft | review | approved | in_progress | done | cancelled

One-line summary of what this spec covers.

**Source code is the source of truth.** This directory is disposable; humans may delete it anytime after the work is no longer needed as scaffolding.

## Documents (read in order)

| Order | File | Role |
|-------|------|------|
| 1 | [01-context](01-context.md) | Problem, goals, non-goals, constraints |
| 2 | [02-requirements](02-requirements.md) | Observable behaviour / contracts |
| 3 | [03-design](03-design.md) | Decisions and invariants |
| 4 | [04-plan](04-plan.md) | Touch points, order, subtask index |

### Supporting docs (optional)

| Order | File | Role |
|-------|------|------|
| … | `reference-<topic>.md` | Scoped notes (opcodes, external formats, …) |

### Subtasks (optional)

| ID | File | Status | Branch / worktree note |
|----|------|--------|-------------------------|
| 01 | [subtasks/01-short-name](subtasks/01-short-name.md) | draft | `gta3sc-rewrite-branches/<spec-id>/short-name` |

## Review gate

- [ ] Human has set spec `status` to `approved` (or explicitly waived in chat) before implementation
- [ ] Subtasks marked independently implementable have clear **Depends on** / acceptance

## Code truth (fill as code lands; optional)

Paths that override stale prose:

- …

## Changelog (optional)

- YYYY-MM-DD — …

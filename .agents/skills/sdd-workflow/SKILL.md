---
name: sdd-workflow
description: >-
  Spec-Driven Development for gta3sc under docs/sdd/ — write specs, enforce the
  human review gate, split subtasks across worktrees, and implement only when
  approved. Use when creating or editing specs under docs/sdd/, when the user
  asks for an SDD spec, review gate, concurrent subtasks, or worktree-based
  implementation of a spec.
---

# SDD (`docs/sdd/`)

**Source code and tests are authoritative.** Specs under `docs/sdd/yyyymmdd-slug/`
are disposable scaffolding; humans may delete them anytime. Do not treat a
missing spec as an error.

## Before creating or rewriting a spec

1. Read [`docs/sdd/README.md`](../../../docs/sdd/README.md) (principles, status lifecycle).
2. Read [`docs/sdd/WORKFLOW.md`](../../../docs/sdd/WORKFLOW.md) (stages, review gate, worktrees/branches).

## New work

- **Fuzzy idea:** clarify in chat first (interactive); then distill into Context / Requirements.
- **Small:** copy [`docs/sdd/00000000-template/README.single.md`](../../../docs/sdd/00000000-template/README.single.md) to `docs/sdd/yyyymmdd-slug/README.md`.
- **Larger:** copy [`docs/sdd/00000000-template/README.multi.md`](../../../docs/sdd/00000000-template/README.multi.md) to `README.md`; add `01-context` … `04-plan` as needed; use [`docs/sdd/00000000-template/SUBTASK.md`](../../../docs/sdd/00000000-template/SUBTASK.md) for parallelizable units.
- Every spec directory has `README.md` as the entrypoint (small = full spec; large = index).
- Frontmatter: `inherits: docs/sdd`, `spec_id: yyyymmdd-slug`, `status: …`.

## Review gate

Do **not** implement from `status: draft` or `review` unless the user explicitly
overrides in the current chat. Wait for `approved` / `in_progress` (human
promotes to `approved`).

## While implementing

- Follow [`AGENTS.md`](../../../AGENTS.md); feature branches under `gta3sc-rewrite-branches/`.
- One subtask per worktree when running concurrent agents (prefer the tool's
  default worktree location; see WORKFLOW).

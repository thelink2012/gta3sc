---
inherits: docs/sdd
spec_id: yyyymmdd-slug
title: Subtask — short name
status: draft
doc_role: subtask
subtask_id: 01-short-name
---

# Subtask: <short name>

Parent spec: [`../README.md`](../README.md).

**Status:** draft | review | approved | in_progress | done | cancelled

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

One or two sentences: what this unit delivers.

## Scope

- In: …
- **Out:** …

## Depends on

- None | subtask `NN-…` must be merged first | soft dependency: …

## Design notes (delta only)

Only what is not already in the parent Design/Requirements—or link to those sections.

- …

## Implementation sketch

- Touch points: …
- Suggested order (interface → tests → impl per `AGENTS.md` when adding APIs): …

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/<spec-id>/short-name`
- Worktree: prefer tool default; for manual `git worktree add`, use `.worktrees/<short-name>/` (see WORKFLOW)

## Acceptance

- [ ] …
- [ ] Unit tests: …
- [ ] Full `gta3sc_unittest` green in this worktree before considering the subtask done (autonomous agents)

## Notes / TBD

- …

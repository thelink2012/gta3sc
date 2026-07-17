---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — integration tests
status: approved
doc_role: subtask
subtask_id: 08-integration-tests
---

# Subtask: integration tests

Parent spec: [`../README.md`](../README.md).

**Status:** approved — **blocked**

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Add a lit-style / argv-matrix harness that runs the built `gta3sc` binary
against fixture config trees and representative community command lines.

## Scope

- In: harness wiring, compile-path matrix once CLI flags stabilize enough.
- **Out:** implementing the decompiler; inventing new CLI product flags.

## Depends on

- Broader lit / integration-test infrastructure (`BACKLOG.md`).
- Decompile matrix additionally needs a decompiler (or legacy decompiler
  bridge) — still blocked.
- CLI binary already exists (`gta3sc-cli-exe`); that particular blocker is
  lifted for **compile** cases.

## Design notes (delta only)

- Parent Plan Phase 6 + `BACKLOG.md` Integration Tests section.

## Implementation sketch

- Deferred until lit scaffolding exists; then add compile fixtures first.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/integration-tests`
- Worktree: prefer tool default; manual → `.worktrees/integration-tests/`

## Acceptance

- [ ] Documented harness runs in CI or local recipe.
- [ ] At least a compile argv matrix green on fixtures.
- [ ] Full unit binary still green; lit failures attributed clearly.

## Notes / TBD

- **Do not implement yet** under this CLI spec alone — track lit work in
  backlog; return here when unblocked.

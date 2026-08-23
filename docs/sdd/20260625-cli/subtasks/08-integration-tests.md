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

**Status:** approved — **partially unblocked** (harness exists; CLI smoke tests still wait on flags)

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Once compile flags stabilize, add a few lit `RUN:` lines that exercise
realistic compiler command lines against a small fixture config tree
(the CLI contract: `--config`, `-o`, `--add-config`, `--datadir`, …).

The runner and the ported compiler suites already live in
[`../../20260822-integration-tests/`](../../20260822-integration-tests/).
Do not invent a second harness.

## Scope

- In: a handful of CLI smoke tests on the existing lit suite.
- **Out:** implementing the decompiler; inventing new CLI product flags;
  re-porting the legacy suite.

## Depends on

- Broader lit harness: landed — see the spec linked above.
- Decompile smoke tests additionally need a decompiler — still blocked.
- CLI binary already exists (`gta3sc-cli-exe`).
- Compile-path smoke tests wait on flag plumbing (parent Phase 4+).

## Design notes (delta only)

- Parent Plan Phase 6 + `BACKLOG.md` Integration Tests section.

## Implementation sketch

- Lit scaffolding exists under `test/` (`-DGTA3SC_INTEGRATION_TESTS=ON`).
  Add CLI smoke `RUN:` lines once flags stabilize.

## Branch / worktree

Harness + suite port (this work): `gta3sc-rewrite-branches/integration-tests`

CLI smoke tests later, if split: `gta3sc-rewrite-branches/20260625-cli/integration-tests`

## Acceptance

- [ ] Documented harness runs in CI or local recipe.
- [ ] At least a few compile CLI smoke tests green on fixtures.
- [ ] Full unit binary still green; lit failures attributed clearly.

## Notes / TBD

- Harness + suite port is `20260822-integration-tests`, not this subtask.
  Return here for CLI smoke `RUN:` lines once enough flags are honoured.

---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — query-models
status: approved
doc_role: subtask
subtask_id: 07-query-models
---

# Subtask: query-models

Parent spec: [`../README.md`](../README.md).

**Status:** approved — blocked on model loading

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Implement `gta3sc query-models`: bootstrap config/models and print
`=DEFAULT` / `=LEVEL` style output like legacy.

## Scope

- In: `run_query_models` in dedicated `lib/cli/run-query-models.cpp` (+
  matching header), shared bootstrap factoring (`load_command_table` /
  `load_model_table` free functions) if compile and query both need it,
  unit tests with fixtures.
- **Out:** lit integration harness (08), decompiler.

## Depends on

- Subtask `03-models-datadir` (real model loading).
- Soft: coordinate `run.cpp` dispatch with `05-query-config-path` if both
  open (separate runner files; no shared `run-query` module).

## Design notes (delta only)

- Parent Design: shared bootstrap as free functions when the second consumer
  appears — this subtask is that consumer.
- Dedicated file basename `run-query-models` (mirror `run-compile`), not a
  shared `run-query.cpp`.

## Implementation sketch

- Touch points: `include/gta3sc/cli/run-query-models.hpp`,
  `lib/cli/run-query-models.cpp`, factor helpers out of `run-compile.cpp`,
  `run.cpp` dispatch, tests.
- Order: factor bootstrap → query runner → tests.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/query-models`
- Worktree: prefer tool default; manual → `.worktrees/query-models/`

## Acceptance

- [ ] `query-models` prints model listings in the legacy-compatible shape
      agreed by parent Requirements/Design.
- [ ] Reuses bootstrap with compile (no duplicated load logic left adrift).
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Do not start until `03` lands.

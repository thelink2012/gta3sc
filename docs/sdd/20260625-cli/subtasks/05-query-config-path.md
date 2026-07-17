---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — query-config-path
status: approved
doc_role: subtask
subtask_id: 05-query-config-path
---

# Subtask: query-config-path

Parent spec: [`../README.md`](../README.md).

**Status:** approved — unblocked (parallel OK)

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Implement `gta3sc query-config-path`: print the resolved config root
(`find_config_root()`) to `out`, matching legacy usefulness for scripts.

## Scope

- In: `run_query_config_path` in dedicated
  `lib/cli/run-query-config-path.cpp` (+ matching header under
  `include/gta3sc/cli/`), wire `Action::query_config_path` in
  `lib/cli/run.cpp` (today falls through to “subcommand not yet
  implemented”), unit tests with `GTA3SC_CONFIG_ROOT` / temp dir.
- **Out:** `query-models`, decompile, compile flag work.

## Depends on

- None for behaviour (`config::find_config_root` exists).
- Soft: coordinate `run.cpp` switch edits with subtask `06-decompile-stub`.

## Design notes (delta only)

- Parent Design Phase 6: print `find_config_root()`.
- Streams already injectable via `cli::run`.

## Implementation sketch

- Touch points: `include/gta3sc/cli/run-query-config-path.hpp`,
  `lib/cli/run-query-config-path.cpp`, `run.cpp`, CMake lists,
  `unittest/cli/…`.
- Order: interface → tests → dispatch + impl.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/query-config-path`
- Worktree: prefer tool default; manual → `.worktrees/query-config-path/`

## Acceptance

- [ ] `query-config-path` prints the root path and exits success when found.
- [ ] Clear failure when config root missing (consistent with compile).
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Good concurrent partner for `01-storage-options` (minimal file overlap except
  `run.cpp`).

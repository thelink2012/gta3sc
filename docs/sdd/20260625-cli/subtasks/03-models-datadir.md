---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — models from datadir
status: approved
doc_role: subtask
subtask_id: 03-models-datadir
---

# Subtask: models from datadir

Parent spec: [`../README.md`](../README.md).

**Status:** approved

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Replace the empty model-table stub in `run_compile` by honouring
`--datadir` / `--levelfile`: load models via `config::load_models_from_level`
into the `ModelTable` used by `Compilation`.

## Scope

- In:
  - Parse `--datadir` / `--levelfile` on compile.
  - Infer level `.dat` (`gta.dat` / `gta3.dat` / `gta_vc.dat`) as in the plan.
  - Call `load_models_from_level` twice (`default.dat`, `objs_only=true`;
    level, `objs_only=false`) through `RelativeInsensitivePathResolver(datadir)`
    into one `ModelTable::Builder`.
  - Unit tests with temp datadir fixtures.
- **Out:** storage flags (01), `--add-config` (02), query-models UI (07),
  `commandline.txt`.

## Depends on

- Subtask `01-storage-options` merged first (shared `run-compile` surfaces).
- Soft: coordinate with `02-add-config` if overlapping in time.

## Design notes (delta only)

- Today `load_model_table` in `lib/cli/run-compile.cpp` builds an empty table
  with a Phase 4+ TODO.
- Parent Plan Phase 4 describes the double `load_models_from_level` call.

## Implementation sketch

- Touch points: `run-compile.hpp`/`.cpp`, `config/models.hpp` usage,
  `unittest/cli/…` and/or reuse models fixtures patterns from
  `unittest/config/`.
- Order: options fields → tests → load implementation → remove empty stub.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/models-datadir`
- Worktree: prefer tool default; manual → `.worktrees/models-datadir/`

## Acceptance

- [ ] With valid `--datadir`/`--levelfile`, compile uses a non-empty model
      table loaded from level data (as applicable to fixtures).
- [ ] Flags absent: behaviour documented (error vs empty — match plan:
      honour when present; do not invent new product rules).
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Unblocks subtask `07-query-models`.
- If plan text is ambiguous on “no `--datadir`” behaviour, keep today’s empty
  table unless requirements say otherwise — do not invent new flags.

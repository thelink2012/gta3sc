---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — storage options
status: approved
doc_role: subtask
subtask_id: 01-storage-options
---

# Subtask: storage options

Parent spec: [`../README.md`](../README.md).

**Status:** approved — **next to implement**

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Honour storage-limit CLI flags end-to-end: parse → pure mapper →
`driver::Compilation` uses non-default `codegen::StorageTable::Options`.

## Scope

- In:
  - Extend `Compilation` (or its codegen path) to accept
    `StorageTable::Options`, defaulted so existing call sites stay unchanged.
  - Pure `to_storage_options(...)` (or equivalent) + unit tests without argv.
  - Parse/honour `-flocal-var-limit`, `-fmission-var-*`, `-ftimer-index` in
    compile options (shared helper if it fits the Phase 4 group pattern).
  - Wire mapped options from `run_compile` into `Compilation`.
- **Out:** `--add-config`, `--datadir`/`--levelfile`, other `-f*` language
  flags, `commandline.txt`, query/decompile.

## Depends on

- None (Phases 1–3 done).

## Design notes (delta only)

- Honour-or-error: do not accept these flags until the mapper + Compilation
  path actually use them.
- Today: hardcoded `StorageTable::Options{}` in `Compilation::codegen`
  (`lib/driver/compilation.cpp`).
- Target fields live on `codegen::StorageTable::Options`
  (`include/gta3sc/codegen/storage-table.hpp`).

## Implementation sketch

- Touch points: `include/gta3sc/driver/compilation.hpp`,
  `lib/driver/compilation.cpp`, `include/gta3sc/cli/run-compile.hpp`,
  `lib/cli/run-compile.cpp`, new mapper header/source under `cli/` (or
  adjacent), `unittest/cli/…`, `unittest/driver/…` as needed.
- Order: interface (Compilation options + mapper) → unit tests → impl →
  CLI parse/honour tests.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/storage-options`
- Worktree: prefer tool default; manual → `.worktrees/storage-options/`

## Acceptance

- [ ] `Compilation` honours injected `StorageTable::Options` (defaults preserve
      prior behaviour).
- [ ] Mapper unit-tested (no argv).
- [ ] Flags parse and affect storage options used for compile; unknown other
      flags still hard-error.
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Parallel-safe with subtasks 05/06; **not** with 02/03 (same `run-compile`
  surfaces — finish or merge 01 first).

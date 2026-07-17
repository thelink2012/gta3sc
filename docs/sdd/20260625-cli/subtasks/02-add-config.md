---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — add-config
status: approved
doc_role: subtask
subtask_id: 02-add-config
---

# Subtask: add-config

Parent spec: [`../README.md`](../README.md).

**Status:** approved

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Honour `--add-config` on the compile path: parse into `ConfigOptions` and load
extra config XML(s) when building the command table.

## Scope

- In:
  - `ConfigOptions::add_config_files` (or equivalent) + `parse_config_option`
    / compile parse loop.
  - Pass added configs through `config::load_config` (or successive loads)
    per existing config APIs — honour, do not stub.
  - Unit tests for parse + load behaviour with temp fixtures.
- **Out:** storage flags (01), `--datadir`/`--levelfile` (03),
  `commandline.txt` (04).

## Depends on

- Subtask `01-storage-options` should be merged first (shared
  `CompileOptions` / `run-compile.cpp` / parse loop — avoid concurrent edit).

## Design notes (delta only)

- See parent Design § shared `ConfigOptions` / `parse_config_option`.
- Honour-or-error: no parse-only field.

## Implementation sketch

- Touch points: `run-compile.hpp`/`.cpp`, possibly `config::load_config`
  call sites, `unittest/cli/run-compile.cpp` (+ config fixtures).
- Order: extend options struct → tests → parse → load path.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/add-config`
- Worktree: prefer tool default; manual → `.worktrees/add-config/`

## Acceptance

- [ ] `--add-config` accepted and applied when loading commands.
- [ ] Missing/invalid add-config paths fail clearly via CLI/diag channels.
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Serialize vs `03-models-datadir` if both touch the same parse loop in the
  same window.

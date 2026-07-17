---
inherits: docs/sdd
spec_id: 20260625-cli
title: CLI implementation
status: in_progress
doc_role: slice-index
---

# Spec: CLI implementation

**Status:** in_progress

Implementation plan for the **gta3sc** command-line frontend on the
`gta3sc-rewrite` branch. Meant to be executed by an autonomous agent with
minimal human intervention. It supersedes the brainstorming draft
`plans/cli/initial-discussion.md` (not migrated; outdated) where they disagree —
**this is the plan of record**.

**Source code is the source of truth.** This directory is disposable; humans may delete it anytime after the work is no longer needed as scaffolding.

## Progress — where we are

Phases **1–3 are done** in-tree. **Next work is Phase 4** (flag plumbing), starting with storage options. Phases 5–6 remain, with some Phase 6 stubs unblocked for concurrent work.

| Phase | Deliverable | Status | Evidence |
|-------|-------------|--------|----------|
| **1** | `OptionParser` + `gta3sc-cli` lib | **done** | `include/gta3sc/cli/option-parser.hpp`, `lib/cli/option-parser.cpp`, `unittest/cli/option-parser.cpp` |
| **2** | Router, `cli::run`, help/version | **done** | `lib/cli/run.cpp`, `run-help.cpp`, `run-version.cpp`; thin `src/gta3sc/main.cpp`; `unittest/cli/run.cpp` |
| **3** | `run_compile` + config-root discovery | **done** | `lib/cli/run-compile.cpp`; discovery in `gta3sc::config` (`config/config-path.hpp`); `unittest/cli/run-compile.cpp`, `unittest/config/config-path.cpp` |
| **4** | Flag plumbing (storage, `--add-config`, models) | **next** | `Compilation` still hardcodes `StorageTable::Options{}` (`lib/driver/compilation.cpp`); model table empty stub in `run-compile.cpp` |
| **5** | `commandline.txt` merge | remaining (blocked) | Needs honoured flags + config-tree `commandline.txt` |
| **6** | Query/decompile stubs + integration tests | remaining (partially unblocked) | Router already names actions; bodies return "not yet implemented" |

### Checklist (phases)

- [x] Phase 1 — `OptionParser` primitives
- [x] Phase 2 — Router, `run` shell, generic help/version
- [x] Phase 3 — `run_compile` + config discovery
- [ ] Phase 4 — Flag plumbing
- [ ] Phase 5 — `commandline.txt` merge
- [ ] Phase 6 — Remaining subcommands + integration tests

### Status notes (vs original plan text)

- Config-root discovery landed under **`gta3sc::config`** (`include/gta3sc/config/config-path.hpp`), not `cli::` — same behaviour, shared with non-CLI callers.
- Help/version live in `run-help.cpp` / `run-version.cpp` (plan TODO resolved that way). Router treats `-h`/`--help`/`-v`/`--version` as first-token actions; `run_compile` also accepts them mid-args.
- Phase 3 unit tests cover parse/validation/routing; there is **no** fixture e2e success case for `run_compile` writing `.scm` yet (optional hardening, not a Phase 4 blocker).
- `BACKLOG.md` CLI checkboxes may lag this README — prefer this Progress table.

## Next steps (ordered)

1. **`subtasks/01-storage-options`** — Wire `-flocal-var-limit` / `-fmission-var-*` / `-ftimer-index` end-to-end (`to_storage_options` → `Compilation` options → honour). **Start here.**
2. **`subtasks/02-add-config`** — Honour `--add-config` (after / coordinated with 01; same `run-compile` surfaces).
3. **`subtasks/03-models-datadir`** — Honour `--datadir` / `--levelfile` model loading (after 01; same surfaces).
4. **`subtasks/04-commandline-txt`** — Only once enough flags are honoured + config ships `commandline.txt`.
5. **`subtasks/07-query-models`** — After model loading (03); factor shared bootstrap if needed.
6. **`subtasks/08-integration-tests`** — Lit/argv harness (blocked on broader lit/decompiler backlog).

**Can run now in parallel with step 1** (light `run.cpp` touch — merge carefully):

- **`subtasks/05-query-config-path`** — print config root
- **`subtasks/06-decompile-stub`** — stub error “decompiler not yet implemented”

## Documents (read in order)

| Order | File | Role |
|-------|------|------|
| 1 | [01-context](01-context.md) | Goal, guiding constraints, reference material |
| 2 | [02-requirements](02-requirements.md) | Observable CLI surface, honour-or-error, flag inventory |
| 3 | [03-design](03-design.md) | Architecture, component design, resolved decisions |
| 4 | [04-plan](04-plan.md) | Phases 1–6 detail, DoD, CMake, fixtures, follow-ups |

### Subtasks

| ID | File | Status | Depends on | Concurrency |
|----|------|--------|------------|-------------|
| 01 | [subtasks/01-storage-options](subtasks/01-storage-options.md) | approved | — | **Next;** parallel OK with 05/06 |
| 02 | [subtasks/02-add-config](subtasks/02-add-config.md) | approved | 01 (same files) | After 01 |
| 03 | [subtasks/03-models-datadir](subtasks/03-models-datadir.md) | approved | 01 (same files) | After 01; serialize vs 02 if both open |
| 04 | [subtasks/04-commandline-txt](subtasks/04-commandline-txt.md) | approved | 01–03 + config `commandline.txt` | **Blocked** |
| 05 | [subtasks/05-query-config-path](subtasks/05-query-config-path.md) | approved | — | Parallel with 01; coordinate `run.cpp` with 06 |
| 06 | [subtasks/06-decompile-stub](subtasks/06-decompile-stub.md) | approved | — | Parallel with 01; coordinate `run.cpp` with 05 |
| 07 | [subtasks/07-query-models](subtasks/07-query-models.md) | approved | 03 | After models |
| 08 | [subtasks/08-integration-tests](subtasks/08-integration-tests.md) | approved | lit + CLI maturity | **Blocked** |

## Code truth

Paths / symbols that override stale phase prose:

- Library: `gta3sc-cli` — `lib/cli/{option-parser,run,run-help,run-version,run-compile}.cpp`
- Headers: `include/gta3sc/cli/{option-parser,run,run-help,run-version,run-compile}.hpp`
- Config root: `gta3sc::config::find_config_root` / `config_search_paths` — `include/gta3sc/config/config-path.hpp`
- Entry: `src/gta3sc/main.cpp` → `gta3sc::cli::run`
- Executable CMake target: `gta3sc-cli-exe` (`OUTPUT_NAME gta3sc`)
- Pipeline: `gta3sc::driver::Compilation` — storage options still default-constructed inside `codegen()`

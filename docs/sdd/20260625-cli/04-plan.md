---
inherits: docs/sdd
spec_id: 20260625-cli
title: CLI — Plan and Acceptance
status: in_progress
doc_role: stage-plan
---

# 4. Plan and Acceptance

Parent: [`README.md`](README.md).

Live status and subtask index: [README Progress](README.md#progress--where-we-are).
Implement via [subtasks/](subtasks/), not by re-deriving phases from scratch.

## Phased implementation

Each phase is independently landable and ends with the **Definition of Done**
below.

### Phase 1 — `OptionParser` primitives  *(done)*

- `include/gta3sc/cli/option-parser.hpp`, `lib/cli/option-parser.cpp`.
- Create the `gta3sc-cli` library target (see CMake); register tests.
- Tests `unittest/cli/option-parser.cpp`: short/long value forms (`-o x`,
  `-ox`, `--config=x`, `--config x`), valueless `option`, `-f`/`-fno-` and
  `-m`/`-mno-` `toggle`, `integer` success/overflow/garbage/missing, positional
  detection, missing-value failure (no fall-through), end-of-args.

### Phase 2 — Router, `run` shell, generic help/version  *(done)*

- `include/gta3sc/cli/run.hpp` + `lib/cli/run.cpp`: argv→views, help/version
  as first-token actions (and mid-args inside `run_compile`), `split_action`
  (no subcommand ⇒ `compile`), config-root discovery call, dispatch.
- Help/version implemented as `run-help.cpp` / `run-version.cpp` (plan TODO).
- `--version` prints a placeholder (no git-describe infra yet); `# TODO` to wire
  `project(gta3sc VERSION …)`.
- Thin `src/gta3sc/main.cpp` entry.
- Tests `unittest/cli/run.cpp`: routing table; help/version via captured streams.

### Phase 3 — `run_compile` + config discovery  *(done)*

- Config discovery: `include/gta3sc/config/config-path.hpp` +
  `lib/config/config-path.cpp` (`config_search_paths` + `find_config_root`;
  also `GTA3SC_CONFIG_ROOT` override). *(Plan said `cli::`; landed in `config::`.)*
- `include/gta3sc/cli/run-compile.hpp` + `lib/cli/run-compile.cpp`
  (`CompileOptions`, `ConfigOptions`, `parse_compile_options`, `run_compile`):
  validate (input, config name, config root) → `config::load_config` →
  `SourceManager::scan_directory` (input-stem dir when present) →
  `driver::Compilation` → write output.
- Tests: `unittest/config/config-path.cpp`; `unittest/cli/run-compile.cpp`
  (parse + validation). Optional gap: fixture e2e success write of `.scm`.

After Phase 3, `gta3sc compile --config=<name> main.sc` compiles end-to-end.

### Phase 4 — Flag plumbing (each flag fully wired)  *(next — see subtasks 01–03)*

Honour-or-error: flags are added one coherent group at a time, each wired
end-to-end (parse via a shared `parse_*_option` helper → pure mapper → honoured
by the pipeline) and unit-tested. No parse-only fields land.

Subtasks: [01-storage-options](subtasks/01-storage-options.md) →
[02-add-config](subtasks/02-add-config.md) /
[03-models-datadir](subtasks/03-models-datadir.md).

- Shared groups + helpers: `LanguageOptions`/`parse_language_option`,
  `MachineOptions`/`parse_machine_option`, `ConfigOptions`/`parse_config_option`.
- Pure mappers, each unit-tested without argv. First group is **unblocked
  today** (target struct exists):
  `to_storage_options(...) -> codegen::StorageTable::Options`
  (`-flocal-var-limit`, `-fmission-var-*`, `-ftimer-index`).
- Extend `driver::Compilation` to accept a mapped options bundle instead of the
  hardcoded `StorageTable::Options{}` at `lib/driver/compilation.cpp` (codegen),
  defaulted so existing call sites/tests are unaffected.
- Fully implementable now (no codegen/sema change): `--add-config`;
  `--datadir`/`--levelfile` model loading — infer level `.dat`
  (`gta.dat`/`gta3.dat`/`gta_vc.dat`) and call `config::load_models_from_level`
  twice (`default.dat`, `objs_only=true`; level, `objs_only=false`) through
  `RelativeInsensitivePathResolver(datadir)` into one `ModelTable::Builder`.
- **Blocked groups** (stay hard errors until the backend gains the option):
  `-m*` machine/header, `--cs`/`--cm`, most `-f*` language flags,
  `-fsyntax-only`, guesser-gated flags. Add a short blocker note per group.

### Phase 5 — `commandline.txt` merge  *(remaining — subtask 04, blocked)*

Meaningful only once the flags a real `commandline.txt` references are honoured
(else it hard-errors on the first unimplemented flag — by design).

- Whitespace-tokenise the file into `string_view`s over its buffer and re-enter
  the same parser when `--config=<name>` is seen, with the legacy recursion
  guard (skip re-reading the active config). Tokeniser in core lib, unit-tested;
  file reading via `FilePool`/`SourceManager`.
- **Coordination prereq:** ship a `commandline.txt` per game in the rewrite
  config tree (external `gta3script-config`) referencing **only honoured flags**;
  expand it as Phase 4 wires more. *(`BACKLOG.md`: move config dir to community
  submodule.)*
- Tests: tokeniser, recursion guard, merge precedence.

### Phase 6 — Remaining subcommands + integration tests  *(remaining — subtasks 05–08)*

- Factor shared bootstrap (`load_command_table`, optional `load_model_table`)
  now that `query-models` joins `compile` as a consumer.
- `run-decompile.cpp`: argv routing + stub erroring "decompiler not yet
  implemented" (full decompile **blocked** on the decompiler backend; stub is
  unblocked — [06](subtasks/06-decompile-stub.md)).
- `run-query-config-path.cpp`: `run_query_config_path` (print
  `find_config_root()` — [05](subtasks/05-query-config-path.md)).
- `run-query-models.cpp`: `run_query_models` (bootstrap + print
  `=DEFAULT`/`=LEVEL` like legacy — [07](subtasks/07-query-models.md),
  needs models).
- Integration tests: a `lit`-style / argv-matrix harness running the built
  `gta3sc` binary against fixture config trees and real community command lines.
  (**Blocked** — [08](subtasks/08-integration-tests.md); CLI binary exists;
  decompiler still blocks decompile matrix.)
## Definition of Done (every phase) — Acceptance

Per `AGENTS.md` autonomous-workflow rules:

1. Built **interface → unit tests → implementation → edge-case tests**.
2. `clang-format` applied to all changed C/C++ (80 cols, Allman, pointer-left).
3. Full unit binary green:
   `cmake --build build --target gta3sc_unittest && ./build/unittest/gta3sc_unittest`.
4. CLI rebuilds via `--target gta3sc-cli-exe` (**not** `--target gta3sc`).
5. Document notable decisions in the commit message.

## CMake wiring

- Static library in `lib/CMakeLists.txt` (current):

  ```cmake
  add_library(gta3sc-cli
    cli/option-parser.cpp
    cli/run.cpp
    cli/run-help.cpp
    cli/run-version.cpp
    cli/run-compile.cpp
    # Phase 5/6: cli/run-decompile.cpp cli/run-query-config-path.cpp
    #            cli/run-query-models.cpp …
  )
  target_compile_features(gta3sc-cli PUBLIC cxx_std_26)
  target_link_libraries(gta3sc-cli PUBLIC gta3sc gta3sc-config)
  target_compile_options(gta3sc-cli PRIVATE ${GTA3SC_PEDANTIC_COMPILE_OPTIONS})
  ```

- Config-path sources live on `gta3sc-config` / `gta3sc` as appropriate
  (`lib/config/config-path.cpp`), not on `gta3sc-cli`.
- `src/gta3sc/CMakeLists.txt`: executable is `gta3sc-cli-exe` (OUTPUT_NAME
  `gta3sc`); `target_link_libraries(gta3sc-cli-exe gta3sc-cli)`. `main.cpp`
  only.
- `unittest/CMakeLists.txt`: `cli/option-parser.cpp`, `cli/run.cpp`,
  `cli/run-compile.cpp` (+ `config/config-path.cpp` under config tests); link
  `gta3sc-cli`.
- No new third-party dependencies. Keep each test `.cpp` under ~500 lines.

## Test fixtures

Reuse `WithTempDirFixture` (config/input trees) and `WithDiagnosticFixture`
(pipeline diags); add captured `std::ostringstream`s for `out`/`err`. Most CLI
logic is pure and needs no fixture — `OptionParser`, `parse_*_options`,
`config_search_paths`, `split_subcommand`, and the Phase-4 mappers take plain
inputs. Test-case names describe behaviour, e.g.
`"value option accepts --opt=value form"`,
`"split_subcommand defaults to compile"`,
`"config_search_paths orders exe-adjacent first"`,
`"run_compile writes output for valid input"`.

## Follow-ups to file in BACKLOG.md

- AGENTS.md note: rebuild the CLI binary with `--target gta3sc-cli-exe`.
- Ship `commandline.txt` per game in the config submodule (Phase 5 prereq).
- Proper version string via `project(gta3sc VERSION …)` + git-describe.
- Diagnostic-to-string formatting (already in backlog) unblocks the real CLI
  renderer that Phase 3 stubs.

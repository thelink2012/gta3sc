---
inherits: docs/sdd
spec_id: 20260625-cli
title: CLI — Context
status: in_progress
doc_role: stage-context
---

# 1. Context

Parent: [`README.md`](README.md).

## Goal and guiding constraints

Close the gap with legacy gta3sc by giving the rewrite a real CLI, while:

- **Preserving the legacy argv contract** (subcommands, flag spelling,
  `commandline.txt`) so existing build scripts keep working. Error wording and
  help text may differ; behaviour for the flags we actually honour must not.
- **Implementing only what the current pipeline supports today.**
  `driver::Compilation` compiles a multifile script with a fixed set of codegen
  options. A flag is either **honoured** or it is a **hard error** — there is no
  "parse-only stub" limbo. We grow the surface incrementally, fully wiring each
  flag (parse → map → honour) before the parser accepts it.
- **Matching the rewrite's design and conventions** — modular, testable, pure
  at the edges, arena/diagnostics idioms, naming per `AGENTS.md`. Legacy
  `main.cpp` / `argv.hpp` are a *behavioural* reference only; their code style
  is explicitly **not** a model.
- **Not over-engineering.** Smallest clean thing that satisfies the contract,
  with obvious seams for growth.

Live implementation status: [README Progress](README.md#progress--where-we-are)
(Phases 1–3 done; Phase 4 next).

## Reference material

| Concern               | Legacy reference                                            | Rewrite anchor                                                                      |
| --------------------- | ----------------------------------------------------------- | ----------------------------------------------------------------------------------- |
| argv loop & dispatch  | `dev/gta3sc/src/main.cpp`                                   | —                                                                                   |
| option primitives     | `dev/gta3sc/src/cpp/argv.hpp` (`optget`/`optflag`/`optint`) | —                                                                                   |
| config path discovery | `dev/gta3sc/src/system.cpp` (`find_config_path`)            | —                                                                                   |
| `commandline.txt`     | `dev/gta3sc/config/<game>/commandline.txt`                  | external config repo                                                                |
| compile pipeline      | —                                                           | `driver::Compilation` (`include/gta3sc/driver/compilation.hpp`)                     |
| config loading        | —                                                           | `config::load_config` (`include/gta3sc/config/config.hpp`)                          |
| model loading         | —                                                           | `config::load_models_from_level` (`include/gta3sc/config/models.hpp`)               |
| source files / scan   | —                                                           | `SourceManager` (`include/gta3sc/source-manager.hpp`)                               |
| diagnostics           | —                                                           | `DiagnosticHandler`, `CallbackDiagnosticHandler` (`include/gta3sc/diagnostics.hpp`) |
| path resolvers        | —                                                           | `filesystem::RelativeInsensitivePathResolver`                                       |

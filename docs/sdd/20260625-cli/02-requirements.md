---
inherits: docs/sdd
spec_id: 20260625-cli
title: CLI — Requirements
status: in_progress
doc_role: stage-requirements
---

# 2. Requirements

Parent: [`README.md`](README.md).

## What "right now" means

Phases 1–3 are **implemented** and deliver a working, drop-in **compile** path:

```
gta3sc compile --config=gta3 main.sc      # explicit subcommand
gta3sc --config=gta3 main.sc              # no subcommand → defaults to compile
gta3sc --config=gta3 main.sc -o out.scm
gta3sc --help
gta3sc --version
```

Everything else (full flag surface, `commandline.txt`, model loading, flag
plumbing, decompile, query subcommands, integration tests) remains Phases 4–6.
See [README Progress](README.md#progress--where-we-are) for live status and
[subtasks/](subtasks/) for the split.

## Honour-or-error

A flag is either **honoured** or it is a **hard error** — there is no
"parse-only stub" limbo. Grow the surface incrementally, fully wiring each
flag (parse → map → honour) before the parser accepts it.

Error wording and help text may differ from legacy; behaviour for the flags we
actually honour must not.

## Legacy flag inventory (contract reference)

Honour-or-error: a flag is **honoured** in its phase, or a **hard error**
("unrecognized argument") until then. No parse-only state. **stub** = routed but
not functional.

| Flag(s)                                                                             | Honoured in                        | Status today                            |
| ----------------------------------------------------------------------------------- | ---------------------------------- | --------------------------------------- |
| positional input, `-o`                                                              | 3                                  | **done**                                |
| `--help`/`-h`, `--version`                                                          | 2                                  | **done**                                |
| no subcommand ⇒ `compile`; explicit `compile`                                       | 2–3                                | **done**                                |
| explicit `decompile`/`query-*`                                                      | 6                                  | routed; body stub / “not yet implemented” |
| `--config=<name>` (path discovery + config.xml)                                     | 3                                  | **done**                                |
| `-flocal-var-limit`, `-fmission-var-*`, `-ftimer-index` (→ `StorageTable::Options`) | 4                                  | hard error until Phase 4                |
| `--add-config`, `--datadir`, `--levelfile`                                          | 4                                  | hard error until Phase 4                |
| `-D`/`--define`, `-U`/`--undefine`                                                  | 4 (when preprocessor defines wire) | error (blocked)                         |
| `commandline.txt` recursive merge                                                   | 5                                  | not read (config.xml only)              |
| `-m*` machine/header, `-O`, `--cs`/`--cm`                                           | when codegen gains the option      | error (blocked)                         |
| `-f*` language flags, `-fsyntax-only`, `--guesser`, `-pedantic[-errors]`            | when sema gains the option         | error (blocked)                         |
| `-W*`, `--error-format`, `--expect-var`                                             | 6                                  | error                                   |
| `query-config-path`, `query-models`                                                 | 6                                  | not implemented (see subtasks 05/07)    |
| `decompile`, `-emit-ir2`, `--recursive-traversal`                                   | 6                                  | stub / blocked (needs decompiler)       |

> **Dropped: extension-based action inference.** Legacy picks compile/decompile
> from the input extension. In a subcommand-first design the runner — not a
> pre-parse step — owns the input, so inference would mean partially parsing
> before choosing a runner. Per review, we drop it: **no subcommand ⇒ compile**;
> decompile is explicit (`gta3sc decompile …`). Simpler router, negligible loss
> (decompile is rarely the implicit case).

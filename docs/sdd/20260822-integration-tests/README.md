---
inherits: docs/sdd
spec_id: 20260822-integration-tests
title: Migrate legacy lit integration tests
status: approved
doc_role: single-file
---

# Spec: Migrate legacy lit integration tests

**Source code is the source of truth.** This file is disposable scaffolding. Prefer merged code/tests on conflict; humans may delete this directory anytime.

**Status:** approved

**Branch:** `gta3sc-rewrite-branches/integration-tests`

Related: CLI Phase 6 subtask [`../20260625-cli/subtasks/08-integration-tests.md`](../20260625-cli/subtasks/08-integration-tests.md) was blocked on this infrastructure. That subtask remains the place for a few CLI smoke `RUN:` lines once flags stabilize; this spec is the **harness + suite port**.

---

## 1. Context

### Problem

Legacy gta3sc (`~/dev/gta3sc/test`) has ~155 lit-driven integration tests covering lexer through codegen plus two full-game smoke tests. The rewrite had unit tests (doctest/CTest) but **no** integration/lit tree. BACKLOG tracked “Migrate lit tests from legacy” behind a CLI blocker that is now lifted (`gta3sc-cli-exe` / `gta3sc`).

The legacy harness is outdated: Python 2 shebangs, unmaintained OutputCheck, no CMake/CTest wiring, Travis-only `lit test` after a manual `GTA3SC=` export.

### Goals

- Port the legacy integration-test **suite and RUN-line model** into this repo under `test/`.
- Run those tests from CMake/CTest against the freshly built `gta3sc` binary.
- Modernize the harness: Python 3, PyPI `lit` (not LLVM `llvm-lit`), maintained FileCheck via PyPI `filecheck`.
- Keep Windows workable while **favoring Unix tooling** (`sh`, `md5sum`, Git Bash / MSYS2), matching legacy.
- Assume unimplemented rewrite features (decompiler, many CLI flags, full VC/SA) are present for the purpose of writing tests. Cases will fail until the compiler catches up; that is expected.

### Non-goals

- Implementing missing compiler features, diagnostic formatting, or CLI flags so tests go green.
- Providing or converting a V2 `config/` tree. The compiler discovers configs itself (see Requirements); the harness only forwards `GTA3SC_CONFIG_ROOT` if the user set it.
- Decompiler tests (legacy had none).
- Refactoring the **directory taxonomy** of the suite (pipeline-stage folders). Deferred follow-up — see Plan.
- Inventing a custom test runner instead of lit.
- XFAIL / known-fail buckets — leave cases failing until the compiler implements the needed behaviour.

### Constraints

- Python 3 only. Python 2 is dead.
- Test runner is the **PyPI `lit` package** (`lit` console script on `PATH`). Not LLVM’s `llvm-lit` binary. Install method is unspecified (dnf, pipx, uv, …). Do not assume `python -m lit` (PyPI lit has no `__main__`).
- `%FileCheck` is LLVM FileCheck **syntax** via the PyPI `filecheck` package’s console script (same caveat: no `python -m filecheck`).
- Unix-first helpers (`sh` scripts). On Windows, run from Git Bash / MSYS2 / Cygwin.
- CMake already has `include(CTest)` and doctest discovery; integration tests must not steal the default fast path. Option default **OFF**.

### Brownfield

- Legacy source of tests: `~/dev/gta3sc/test` (suites `codegen/`, `frontend/`, `lexer/`, `main/`, `misc/`, `parser/`, `preprocessor/`, `semantics/`).
- Rewrite binary: CMake target `gta3sc-cli-exe`, `OUTPUT_NAME gta3sc`.
- CLI subtask 08 can add a few CLI smoke `RUN:` lines after flags exist.

### Glossary

| Term | Meaning |
|------|---------|
| lit | PyPI test runner that executes `RUN:` lines. |
| FileCheck | Pattern matcher for compiler stdout. Here: PyPI `filecheck`, LLVM FileCheck syntax. |
| OutputCheck | Abandoned tool used by legacy (`CHECK-L:` = literal, `CHECK:` = regex). Replaced. |
| `%verify` | Clang `-verify` clone: match `// expected-error {{…}}` against compiler stderr. |

---

## 2. Requirements

### How tests are defined

- A test is a `.sc` or `.test` file with one or more `RUN:` lines. Any failing `RUN` fails the test.
- Only files **directly** under `test/<suite>/` are discovered. Deeper files are support scripts (multifile includes, etc.).
- Directories named `Inputs/` are excluded from discovery (fixtures, not tests). Present today: `test/frontend/Inputs/` (`test.xml`, `override.xml`) and `test/semantics/Inputs/data/` (`.dat` / `.ide`).
- Substitutions in `RUN` lines:

| Name | Meaning |
|------|---------|
| `%s`, `%t`, `%S`, `%T`, … | Standard lit substitutions. |
| `%gta3sc-filecheck` | `gta3sc-filecheck.sh`: `$1` filecheck, `$2` compiler; compile `%s` and FileCheck stdout. Extra args after the source go to the compiler (no baked `-emit-ir2 -o -`). |
| `%gta3sc-verify` | `gta3sc-verify.sh`: `$1` verifier, `$2` compiler; compile `%s` and check diagnostics. Exit 0/1 discarded; crash still fails. |
| `%gta3sc` | Binary under test, invoked with `-Wno-expect-var` (legacy default). |
| `%FileCheck` | PyPI `filecheck` (LLVM FileCheck syntax). |
| `%verify` | `verify-diagnostics.py` (Python 3) on stdin vs annotations in `$1`. |
| `%checksum` | `sh checksum.sh`: md5 of `$1` equals `$2`. |
| `%not` | Invert exit code; crash (`>1`) stays a failure. |

Keep the POSIX `sh` helpers (`not.sh`, `checksum.sh`, plus the filecheck/verify wrappers) with `#!/bin/sh`. Lit’s builtin `not` / `not --crash` is not a substitute (see Design).

### FileCheck syntax (migration from OutputCheck)

- Plain `CHECK:` / `CHECK-NEXT:` / `CHECK-NOT:` match **literally**.
- Regex fragments must be wrapped in `{{…}}`.
- There is no `CHECK-L:`. Legacy `CHECK-L:` / `CHECK-NEXT-L:` / `CHECK-NOT-L:` translate by dropping `-L`. Genuine regex `CHECK:` lines wrap the regex parts.

### How tests are run

- **Standalone:** `--param gta3sc=<path>` or `$GTA3SC` (no PATH lookup). `lit test -v` from the repo (or `lit test/codegen` for one suite). Works with an **out-of-tree** binary. Lit is pointed at the **source** `test/` directory, which has `lit.cfg` only (no generated site cfg).
- **CMake/CTest:** option `GTA3SC_INTEGRATION_TESTS` (default OFF). When ON, Python 3 + `lit` + `filecheck` on PATH are **required** (`FATAL_ERROR` if missing). Registers `add_test(NAME gta3sc_integration …)` with `LABELS integration`, passing `--param gta3sc=$<TARGET_FILE:gta3sc-cli-exe>` and `lit -v` (not `-s`) so CI logs keep per-test names and failing `RUN:` output. A plain `ctest` with the option OFF never sees ITs (fast path for agents). With the option ON, `ctest -L unit` is units; `ctest -L integration` / `-R gta3sc_integration` is ITs.
- Other CMake options already in the project: `GTA3SC_COVERAGE`, `GTA3SC_ANALYSIS`, plus `BUILD_TESTING` from `include(CTest)`.
- **Config data:** not managed by the harness. If `GTA3SC_CONFIG_ROOT` is set, it is forwarded into the test environment. If it is unset, the compiler’s own `find_config_root()` searches exe-adjacent `config/` (then `~/.local/share/gta3sc/config` and `/usr/share/gta3sc/config` on Unix). Setting `GTA3SC` to an out-of-tree binary therefore picks up that binary’s sibling `config/` automatically, with no extra env var.

### Suite contents

- Port the legacy suites **as-is** (copy, do not rewrite sources except FileCheck syntax).
- Keep `main/` network tests (Dropbox tarball + `%checksum`). Revisit tracked in `BACKLOG.md`.
- Empty `parser/` (legacy TODO placeholder) is kept.

### Expected-red

Cases that need unimplemented rewrite behaviour (`-emit-ir2`, `--guesser`, `-f*` / `-D` / `-W*`, VC/SA specifics, clang-style diagnostic formatting for `%verify`) **fail until those land**. Do not XFAIL the suite.

### Windows

Favor Unix tooling. Document Git Bash / MSYS2 / Cygwin. `execute_external` is off on `win32` (lit internal shell), matching legacy.

---

## 3. Design

### Approach — two configs, one suite

Checked-in **`test/lit.cfg`** holds discovery, substitutions, and helper paths. It is enough for standalone `lit test`.

CMake generates **`build/test/lit.site.cfg`** from `test/lit.site.cfg.in`. That file only sets `config.test_exec_root` (so lit temp/`Output/` land in the build tree) and then `load_config`s the source `lit.cfg`. The binary path is **not** baked into the site cfg: it is a generator expression, so CTest passes it at run time as `--param gta3sc=…`. `lit.cfg` reads `--param gta3sc` first, then `$GTA3SC`. There is no PATH fallback.

Standalone does not see `lit.site.cfg` because that file is generated in the **build** directory. `lit test` is aimed at the **source** `test/` folder, which contains `lit.cfg` only. CTest runs `lit <build>/test`, where the site cfg exists.

`configure_file(...)` is CMake copying the `.in` template and substituting `@GTA3SC_LIT_EXEC_ROOT@` / `@GTA3SC_LIT_SOURCE_CFG@`. The generated file lives under `build/` (already gitignored).

```text
Source                              Build
──────                              ─────
test/lit.cfg          <──load_config──  build/test/lit.site.cfg
test/lit.site.cfg.in  ──configure_file──►     test_exec_root = build/test
test/*.sc

Standalone:  lit --param gta3sc=<exe> test   (or $GTA3SC)
CTest:       lit -v --param gta3sc=<exe> build/test
```

CTest test name is `gta3sc_integration` (not a generic `integration`) so a superproject that `add_subdirectory`s this repo is unlikely to clash. Unit tests get `LABELS unit`.

### Alternatives rejected

| Alternative | Why not |
|-------------|---------|
| Custom Python runner | Reimplements lit’s `RUN:` / substitutions; more to maintain. |
| LLVM `llvm-lit` / `FileCheck` binaries | Extra LLVM toolchain; version skew vs PyPI lit. CMake looks up `lit` only. |
| Keep OutputCheck | Unmaintained, Python-2-era; CHECK-line translation is mechanical. |
| Commit V2 config fixtures in `test/` | Config data is a separate problem; compiler search path is enough. |
| Drop `main/` | User chose to keep them; revisit later (BACKLOG). |
| `python -c lit.main` CMake fallback | pipx/uv/dnf already put `lit` on PATH; the fallback bloated CMakeLists. |
| Lit builtin `not` / `not --crash` instead of `not.sh` | See below. |

### `not --crash` (not used)

PyPI lit 18.1.8 **parses** `not --crash` in its internal shell. Semantics:

- Plain `not cmd`: success if `cmd` is non-zero (any non-zero).
- `not --crash cmd`: expects `cmd` to **crash** (typically abort / signal / high status); a clean exit 1 is a failure of `not --crash`.

That is the **opposite emphasis** of `not.sh`, which treats **exit 1 as the expected compiler error** and treats crash (`>1`) as a test failure. Also, with `execute_external` on Unix, `not --crash` is re-pushed as an external `not` binary (LLVM’s `not` tool), which we do not depend on. Keep `%not` → `not.sh`.

### Invariants

- Console scripts only (`lit`, `filecheck`). No `python -m`.
- One-level discovery stays in `GTA3ScriptTest(ShTest)` so multifile support scripts are not separate tests.
- `%verify` still expects clang-style `file:line:col: error: msg` on stderr. Rewrite currently prints `diag: <title>` — tests stay red until diagnostic formatting lands (BACKLOG).

### Open questions (follow-ups, not this spec)

- Where the V2 `config/` tree lives for CI (compiler search path vs `GTA3SC_CONFIG_ROOT`).
- New directory taxonomy (see Plan).
- Rename `GTA3ScriptTest.py` to snake_case on next touch (not a dedicated cleanup).

---

## 4. Plan

### What already landed (this worktree)

1. **Harness rewrite** (not a byte-copy of legacy helpers):
   - `test/lit.cfg`, `test/lit.site.cfg.in`, `test/GTA3ScriptTest.py`
   - Python 3 `test/verify-diagnostics.py`
   - `test/{not,checksum}.sh`
   - `test/README.md`, `test/CMakeLists.txt`
   - Root `CMakeLists.txt`: `GTA3SC_INTEGRATION_TESTS` + `add_subdirectory(test)`
2. **Suite copy:** `cp -r` of `codegen frontend lexer main misc parser preprocessor semantics` from `~/dev/gta3sc/test`. No suite files invented; none omitted. After copy, 124/171 files are byte-identical to legacy; the other 47 differ **only** by FileCheck directive translation (`CHECK-*-L:` → `CHECK-*:` plus a handful of `{{regex}}` wraps).
3. **Ignore lit junk:** `test/.lit_test_times.txt` and `test/**/Output/` in `.gitignore`.

### Implementation order (historical)

1. Scaffold harness + CMake wiring.
2. Copy suites; translate CHECK lines.
3. Keep `main/`; document expected-red.

### Out of scope follow-ups

- **Refactor `test/` directory structure** — the pipeline-stage layout (`lexer/`, `semantics/`, `codegen/`, …) was copied 1:1 from legacy and is **not** a gold standard. Tracked in `BACKLOG.md`.
- Revisit `main/` Dropbox downloads — tracked in `BACKLOG.md` (leading idea: submodule of the unpacked scripts).
- Ship/discover V2 config data for CI.
- A few CLI smoke `RUN:` lines (`20260625-cli` subtask 08) once flags stabilize.
- Snake_case rename of the two Python helper modules.

### Touch points

- `test/**` (harness + suites)
- `CMakeLists.txt`, `test/CMakeLists.txt`, `unittest/CMakeLists.txt`
- `.gitignore`
- `BACKLOG.md` Integration Tests section

---

## 5. Acceptance

Harness (independent of compiler completeness):

- [x] `lit` discovers only top-level suite files (102 tests = 100 non-`main` + 2 `main`; nested scripts excluded).
- [x] `cmake -DGTA3SC_INTEGRATION_TESTS=ON` registers `gta3sc_integration` (configure **errors** if Python/`lit`/`filecheck` missing).
- [x] Option default OFF: plain `ctest` is units only.
- [x] Human review of this retroactive spec (`status: approved`).

Not required for this spec (compiler evolution):

- Green codegen/semantics suites.
- Working `main/` Dropbox downloads in CI.
- `%verify` matching rewrite diagnostics (needs clang-style formatting).

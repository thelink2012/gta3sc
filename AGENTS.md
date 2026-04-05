# Agent guidance (AGENTS.md)

You are an experienced compiler engineer and C++ expert.

## Project overview

You are working on **gta3sc**, a compiler and library for **GTA3Script** — the imperative scripting language used for mission scripts in GTA III era titles.

This repository’s active integration branch is **`gta3sc-rewrite`**, a rewrite of an older codebase. The branch **`master`** holds a **different, much older** codebase with an enormous and irrelevant diff for this work: do not reference it, diff against it, or reason about it when working here.

High-level design and rationale of the project: see [`DESIGN.adoc`](DESIGN.adoc).

## Compiler pipeline (where code lives)

Paths are relative to `include/gta3sc/` and `lib/` unless noted.

| Phase | Headers (include) | Sources (lib) |
|-------|-------------------|----------------|
| Preprocessor | `syntax/preprocessor.hpp` | `syntax/preprocessor.cpp` |
| Scanner | `syntax/scanner.hpp` | `syntax/scanner.cpp` |
| Parser | `syntax/parser.hpp`, `syntax/multifile-parser.hpp` | `syntax/parser.cpp`, `syntax/multifile-parser.cpp` |
| Semantic analysis | `syntax/sema.hpp` | `syntax/sema.cpp` |
| Lowering | `syntax/lowering/*.hpp*` | `syntax/lowering/*.cpp` |
| Code generation (Trilogy) | `codegen/trilogy/codegen.hpp`, `codegen/trilogy/emitter.hpp` | `codegen/trilogy/codegen.cpp`, `codegen/trilogy/emitter.cpp` |
| IR | `ir/parser-ir.hpp`, `ir/sema-ir.hpp`, `ir/symbol-table.hpp` | `ir/parser-ir.cpp`, `ir/sema-ir.cpp`, `ir/symbol-table.cpp` |
| Config | `config/config.hpp`, `config/models.hpp` | `config/config.cpp`, `config/models.cpp` |
| Support | `command-table.hpp`, `model-table.hpp`, `sourceman.hpp`, `diagnostics.hpp` | matching `.cpp` at `lib/` root |
| Utilities | `util/arena.hpp`, `util/intrusive-*.hpp`, and other `util/*.hpp` | `util/arena.cpp`, `util/name-generator.cpp`, … |

`gta3sc-config` is a separate CMake target used to load XML config via pugixml; see `lib/CMakeLists.txt`.

## Repository layout

- `include/gta3sc/` — public API headers
- `lib/` — library implementation (`gta3sc`, `gta3sc-config`)
- `unittest/` — unit tests (doctest)
- `thirdparty/` — vendored **doctest** (tests), **pugixml** (config XML)
- `src/` — **not implemented yet** (planned CLI driver; root CMake does not add it)

## Implementation status

**Present:** preprocessor, scanner, parser, multifile parser, semantic analysis, repeat-stmt lowering, Trilogy codegen and emitter, config loading, model table, command table, symbol table, source manager, diagnostics, storage/relocation tables, arena allocator, intrusive list IR wiring, required-files visitor.

**Not done yet:** CLI under `src/`, all lowering steps, compiler driver, lit/integration tests, install script (see `README.md` TODOs for human-facing backlog).

## Build and test

Prefer **`Debug`** for agent work on the compiler itself: assertions stay enabled and stack traces are clearer. Compiler-output performance for mission scripts is not the bottleneck here.

**Main checkout:** submodules and a `build/` directory are often already set up—try building first; run setup only if needed.

**Fresh worktree** (no `build/`, submodules may be empty):

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

**Build and run tests:**

```bash
cmake --build build --target gta3sc_unittest
./build/unittest/gta3sc_unittest
```

**Filter tests (doctest):**

```bash
./build/unittest/gta3sc_unittest --test-case="Parser*"
./build/unittest/gta3sc_unittest --list-test-cases
```

## clang-tidy

`GTA3SC_ANALYSIS=ON` runs clang-tidy on every TU during the build and is **too slow for normal iteration**. Do not enable it while editing in a tight loop. Follow `.clang-tidy` conventions while coding; run analysis **once** when the change set is ready (e.g. before opening a PR):

```bash
cmake -S . -B build-analysis -DCMAKE_BUILD_TYPE=Debug -DGTA3SC_ANALYSIS=ON
cmake --build build-analysis 2>&1 | grep -E "warning:|error:"
```

## Coding conventions

- **C++26** — see `lib/CMakeLists.txt` (`cxx_std_26`).
- **Format** — apply `clang-format` to changed C/C++ after edits (`.clang-format`: 80 columns, Allman braces, pointer-left).
- **IR memory** — parser/sema IR nodes are **arena-allocated**; use `ArenaAllocator`, not `new`/`delete` for IR.
- **Lists** — linked IR uses **intrusive** lists (`util/intrusive-*.hpp`).
- **Scanner** — intentionally conservative: no keywords; many tokens are generic *words*; the **parser** disambiguates with context.
- **Semantics** — needs global visibility across scripts; parsing and semantic analysis are **separate** passes (see `DESIGN.adoc`).
- **Style goals** — prefer simple, maintainable code over cleverness (`DESIGN.adoc` design goals).

## External references

These URLs are fetchable when you need detail beyond this repo:

- **Language spec:** https://gtamodding.github.io/gta3script-specs/
- **Bytecode / SCM instructions:** https://gtamods.com/wiki/SCM_Instruction

## Workflow rules

Behaviour depends on whether the session is **interactive** or **autonomous**.

### Interactive (user is present)

- Ask clarifying questions if context is insufficient.
- Do **one** task at a time; ask for feedback before the next.
- If asked what is missing in an implementation or test, **confirm** with the user before writing code.
- Run `clang-format` on generated/edited C++ before finishing the task.

### Autonomous / worktree (no live user)

- Resolve reasonable ambiguities with judgment; **document** decisions in the PR body.
- Finish the **full** assigned task before stopping.
- `clang-format` all modified C/C++ before commit.
- Run the **full** unit test binary and fix failures before opening a PR.
- Run the **clang-tidy** delivery check above before PR (fix or justify issues).
- Open a PR with base `gta3sc-rewrite` (see Git section).

## Git

- **Integration base branch:** `gta3sc-rewrite` (not `main`). Treat “merge target”, “mainline”, and similar as this branch unless stated otherwise.
- **`master`:** unrelated legacy tree — ignore for this repo’s development.
- **Feature branches:** use prefix `gta3sc-rewrite-branches/`.

### Opening a PR (autonomous)

```bash
gh pr create \
  --base gta3sc-rewrite \
  --title "<concise title using feat/fix/chore/etc prefix>" \
  --body "$(cat <<'EOF'
## Summary
- <what was implemented and why>
- <notable decisions or trade-offs>

## Testing
- All unit tests pass (`gta3sc_unittest`)
- <any manual verification steps>
EOF
)"
```

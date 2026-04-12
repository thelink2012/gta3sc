# Agent guidance (AGENTS.md)

You are an experienced compiler engineer and C++ expert.

## Project overview

You are working on **gta3sc**, a compiler and library for **GTA3Script** — the imperative scripting language used for mission scripts in GTA III era titles.

The active integration branch is **`gta3sc-rewrite`**. The branch **`master`** holds a different, older codebase — do not reference or diff against it.

High-level design and rationale: see [`DESIGN.adoc`](DESIGN.adoc).

## Compiler pipeline

Paths are relative to `include/gta3sc/` and `lib/` unless noted.

| Phase | Headers (include) | Sources (lib) |
|-------|-------------------|----------------|
| Preprocessor | `syntax/preprocessor.hpp` | `syntax/preprocessor.cpp` |
| Scanner | `syntax/scanner.hpp` | `syntax/scanner.cpp` |
| Parser | `syntax/parser.hpp`, `syntax/multifile-parser.hpp` | `syntax/parser.cpp`, `syntax/multifile-parser.cpp` |
| Semantic analysis | `syntax/sema.hpp` | `syntax/sema.cpp` |
| Lowering | `syntax/lowering/*.hpp` | `syntax/lowering/*.cpp` |
| Code generation (Trilogy) | `codegen/trilogy/codegen.hpp`, `codegen/trilogy/emitter.hpp` | `codegen/trilogy/codegen.cpp`, `codegen/trilogy/emitter.cpp` |
| IR | `ir/parser-ir.hpp`, `ir/sema-ir.hpp`, `ir/symbol-table.hpp` | `ir/parser-ir.cpp`, `ir/sema-ir.cpp`, `ir/symbol-table.cpp` |
| Config | `config/config.hpp`, `config/models.hpp` | `config/config.cpp`, `config/models.cpp` |
| Support | `command-table.hpp`, `model-table.hpp`, `sourceman.hpp`, `diagnostics.hpp` | matching `.cpp` at `lib/` root |
| Utilities | `util/arena.hpp`, `util/intrusive-*.hpp`, and other `util/*.hpp` | `util/arena.cpp`, `util/name-generator.cpp`, … |

`gta3sc-config` is a separate CMake target (pugixml-based config loading); see `lib/CMakeLists.txt`. Source under `src/` is not implemented yet (planned CLI driver).

## Implementation status

**Present:** preprocessor, scanner, parser, multifile parser, semantic analysis, repeat-stmt lowering, Trilogy codegen and emitter, config loading, model table, command table, symbol table, source manager, diagnostics, storage/relocation tables, arena allocator, intrusive list IR wiring, required-files visitor.

**Not done yet:** CLI under `src/`, all lowering steps, compiler driver, lit/integration tests, install script.

## Build and test

Prefer **`Debug`**: assertions stay enabled and stack traces are clearer.

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

`GTA3SC_ANALYSIS=ON` is **too slow for normal iteration** — run it once when the change set is ready to land:

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

- **Language spec:** https://gtamodding.github.io/gta3script-specs/
- **Bytecode / SCM instructions:** https://gtamods.com/wiki/SCM_Instruction

## Feature implementation order

When implementing a feature — especially during the planning phase — follow this sequence:

1. **Define the interface** (`.hpp`): types, function signatures, doc comments.
2. **Write unit test cases** (`unittest/`): cover intended behaviour from the caller's perspective.
3. **Write the implementation** (`.cpp`): make the tests pass.
4. **Revisit tests for edge cases**: implementation reveals corner cases — add targeted tests before considering the feature done.

### Unit test readability

- **Don't over-comment.** Clear test code doesn't need narration; omit comments unless something is genuinely non-obvious.
- **Use blank lines to separate given / when / then.** Setup, action, and assertions should breathe — don't compress them into a dense block.

## Workflow rules

Behaviour depends on whether the session is **interactive** or **autonomous**.

### Interactive (user is present)

- Ask clarifying questions if context is insufficient.
- Do **one** task at a time; ask for feedback before the next.
- If asked what is missing in an implementation or test, **confirm** with the user before writing code.

### Autonomous / worktree (no live user)

- Resolve reasonable ambiguities with judgment; **document** notable decisions (e.g. in commit messages).
- Finish the **full** assigned task before stopping.
- `clang-format` all modified C/C++ before commit.
- Run the **full** unit test binary and fix failures before considering the work done.
- Run the clang-tidy delivery check when the change is ready to land (fix or justify issues).

## Git

- **Integration base branch:** `gta3sc-rewrite` (not `main`). Treat "merge target", "mainline", and similar as this branch unless stated otherwise.
- **`master`:** unrelated legacy tree — ignore for this repo's development.
- **Feature branches:** use prefix `gta3sc-rewrite-branches/`.

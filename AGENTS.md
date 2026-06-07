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

### Documentation comments

Use `///` (not `/** … */`). Match the style already in the file you are editing.

**Type-level** (class, struct, enum, alias): one-line summary, then optional further paragraphs separated by a blank `///` line.

**Method-level**: brief imperative sentence (`/// Allocates storage from the arena.`). Add Doxygen tags only when they carry non-obvious information:

- `\param` / `\returns` — non-obvious parameters or return semantics
- `\ref OtherType` — cross-link to related types or methods
- `\note` — lifetime, threading, or semantic caveats
- `\example` with a fenced ` ```cpp ` block for non-obvious usage:

```cpp
/// Reports a diagnostic to this handler.
///
/// \example
/// ```cpp
/// handler.report(loc, diag::undefined_label).args("FOO");
/// // builder destructs at `;`, automatically emitting the diagnostic
/// ```
auto report(SourceLocation loc, const DiagnosticDescriptor& descriptor) noexcept
        -> Diagnostic::Builder;
```

If the comment is short, struct fields and private members can use shorter trailing Doxygen comments instead of a full `///` block:

```cpp
struct Token
{
    Category category{Category::end_of_line}; ///< Category of this token.
    SourceRange source;                       ///< Origin of this token.
};

// Private class members
private:
    char* region_ptr{};   ///< The pointer to the current region.
    size_t region_size{}; ///< The size of the current region.
```

Do not use `@brief`. Do not narrate the identifier back at the reader — `/// Returns the value.` above a getter named `value()` adds nothing.

### Naming

**Types:** `CamelCase` for classes, enums, and type aliases; `lower_case` for namespaces and free functions; `lower_case` for constants (including `static constexpr`).

**Members:** two styles coexist and need to be standardized into one; **prefer bare `lower_case` for new code** until a codebase-wide cleanup is done:

| Style | Used in |
|-------|---------|
| bare `lower_case` | pipeline / service classes: `Scanner`, `Parser`, `MultifileParser`, `ArenaMemoryResource` |
| `m_` + `lower_case` | arena-backed domain objects: IR nodes, `SymbolTable`, `CommandTable`, `ModelTable` |

`m_` is a **scope prefix only** — it does not encode type information. Never write `m_iCount`, `m_pBuffer`, or `m_strName`; those are Hungarian notation and do not appear anywhere in this codebase.

### `class` vs `struct` and access specifiers

**`struct`** — plain data, tag types, thin wrappers, helper bases (`ArenaObj`, `Token`, `SourceRange`, `CommandTable::ParamDef`). All members public; no access-specifier sections.

**`class`** — encapsulation, services, builders. The canonical section order:

```
public:
    constructors, API methods, nested-type forward declarations

private:
    private helper methods          ← first private

private:
    member variables                ← second private: is intentional
```

`readability-redundant-access-specifiers` is disabled specifically for this pattern. Do not collapse private methods and member variables into a single `private:` block.

Factory-controlled types (`ParserIR`, `ModelTable`, `SymbolTable`, `CommandTable`) open with an extra `private:` block for `PrivateTag` before any `public:`. These types also split `public:` into two blocks when nested types must be forward-declared before they appear in method signatures:

```
private:
    struct PrivateTag {};
    static constexpr PrivateTag private_tag{};

public:
    class NestedTypeA;              ← forward declarations only
    class NestedTypeB;

public:
    constructors, API methods       ← main API

private:
    private helper methods

private:
    member variables
```

Nested types defined out-of-line (e.g. `struct MultifileParser::ParseQueueItem`) follow struct rules.

### Other C++ patterns

- **Return types** — trailing syntax: `auto foo() -> T` (including `-> void`).
- **Special members** — copy deleted, move defaulted is the usual shape for non-value types.
- **Queries** — `[[nodiscard]]` on getters and predicates that must not be silently dropped.
- **Controlled construction** — `PrivateTag` + `static create` / table `insert_*` factories for arena IR and symbol entries.
- **Arena-owned types** — inherit `ArenaObj` when copy/move must be suppressed.
- **Builders** — fluent chaining returning `-> Builder&&`; terminal `build() &&`.
- **Diagnostics** — descriptors as `extern const DiagnosticDescriptor` in a `diag` sub-namespace; message placeholders like `// %0 => string (filepath)` on the declaration line.

## External references

- **Language spec:** https://gtamodding.github.io/gta3script-specs/
- **Bytecode / SCM instructions:** https://gtamods.com/wiki/SCM_Instruction

## Feature implementation order

When implementing a feature — especially during the planning phase — follow this sequence:

1. **Define the interface** (`.hpp`): types, function signatures, doc comments.
2. **Write unit test cases** (`unittest/`): cover intended behaviour from the caller's perspective.
3. **Write the implementation** (`.cpp`): make the tests pass.
4. **Revisit tests for edge cases**: implementation reveals corner cases — add targeted tests before considering the feature done.

### Unit tests

**Test case names** — describe the behaviour under test, not the fixture or class name.

- Good: `"load_models_from_ide with objs_only unset"`, `"scanner with empty stream"`, `"emit opcode"`
- Bad: `"ParserFixture - parsing a label"`, `"MyClass - description"`

Name the API, scenario, or outcome. Lowercase phrasing; no `ClassName -` prefix.

**Structure** — tests do not label Given/When/Then explicitly, but follow that rhythm with **blank lines** between them. Do not pack setup, calls, and `CHECK`/`REQUIRE` into one dense block.

```cpp
create_test_file("level.dat", R"(IDE data/vehicles.ide)");

auto table = gta3sc::config::load_models_from_level(
        root_test_dir, root_test_dir / "level.dat", false, sourceman,
        diagman, gta3sc::ModelTable::Builder(&arena))
        .build();

CHECK(diags.empty());
CHECK(table.size() == 1);
expect_model(table, "LANDSTAL", 400);
```

Use `SUBCASE` for variants of the same scenario. Use `REQUIRE` for preconditions that must hold before later assertions; `CHECK` for the rest.

```cpp
TEST_CASE_FIXTURE(CodeEmitterFixture, "emit opcode from command_id")
{
    CodeEmitter emitter;
    emitter.emit_opcode(4660, false);

    SUBCASE("offset increases by 2")
    {
        REQUIRE(emitter.offset() == 2);
    }

    SUBCASE("output is 16-bit little-endian with high bit clear")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x34}, std::byte{0x12}});
    }
}
```

**Don't over-comment.** Clear test code doesn't need narration.

**File size** — keep unit test `.cpp` files small enough to fit comfortably in context (~500 lines is a practical ceiling; split earlier if a file is growing broad). If one source file would produce a ~1000-line test file, split into two or more unit test `.cpp` files and register each in `unittest/CMakeLists.txt`.

- Good split: `unittest/config/models-ide.cpp` + `unittest/config/models-level.cpp` (both exercise `lib/config/models.cpp`)
- Do **not** use `unittest/syntax/parser.cpp` or `unittest/syntax/sema.cpp` as style templates — they are oversized and need splitting

**Fixtures** — the fixture hierarchy grew organically and will need reorganization, but a composable three-level pattern is established:

```cpp
// Level 1 — base building blocks (unittest/)
class WithTempDirFixture    { /* isolated temp dir; create_test_file() */ };
class WithDiagnosticFixture { /* captures diagnostics; consume_diag() */ };
class WithSourceFixture     { /* owns a SourceManager; make_source() */ };

// Level 2 — domain fixture: composes bases via multiple inheritance
class ModelsTestFixture
    : public WithTempDirFixture
    , public WithDiagnosticFixture
    , public WithSourceFixture
{
protected:
    ArenaMemoryResource arena; // NOLINT
};

// Level 3 — feature fixture: adds scenario-specific helpers
class ModelsIdeTestFixture : public ModelsTestFixture
{
public:
    auto load_models(std::string_view content, bool objs_only) -> ModelTable;
};
```

`protected:` data in fixture classes triggers `misc-non-private-member-variables-in-classes`; suppress with `// NOLINT`. This is intentional — sharing state with derived fixtures is the design.

Prefer a shared `*-fixture.hpp` header for fixtures reused across multiple test files. A file-local fixture in an anonymous namespace is fine when only one `.cpp` uses it.

Test code lives in `gta3sc::test` and sub-namespaces (`gta3sc::test::syntax`, `gta3sc::test::config`, …). `using namespace` for the test sub-namespace is fine; fully qualifying the fixture type is also fine.

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

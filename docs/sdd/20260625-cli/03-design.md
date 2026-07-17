---
inherits: docs/sdd
spec_id: 20260625-cli
title: CLI — Design
status: in_progress
doc_role: stage-design
---

# 3. Design

Parent: [`README.md`](README.md).

## Status notes (progress overlay — do not treat as new design)

- Phases 1–3 of this design are **landed**. Remaining work is Phase 4+ (see
  [README Progress](README.md#progress--where-we-are) and [subtasks/](subtasks/)).
- Config-path APIs shipped as `gta3sc::config::{config_search_paths,find_config_root}`
  rather than under `cli::` (section below still describes the intended seam).
- `run_help` / `run_version` are separate TUs; `cli::run` dispatches first-token
  help/version/subcommand (not a full argv pre-scan for `-h` anywhere).

## Architecture

### What lives in `lib/` vs `src/` (and what is unit-testable)

**All CLI logic lives in a library; `src/` is just `main()`.**

| Path                              | Target                                                                                  | Contents                                                                                                                               |
| --------------------------------- | --------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `lib/cli/`, `include/gta3sc/cli/` | static lib **`gta3sc-cli`** (namespace `gta3sc::cli`), links `gta3sc` + `gta3sc-config` | option parser, subcommand runners, `cli::run`, shared option groups + parse helpers, config-path discovery, help/version text, mappers |
| `src/gta3sc/main.cpp`             | executable **`gta3sc-cli`** (`OUTPUT_NAME gta3sc`), links `gta3sc-cli`                  | only `int main(...) { return gta3sc::cli::run(argc, argv); }`                                                                          |

`gta3sc_unittest` links `gta3sc-cli`, so **every CLI behaviour is reachable
from the unit tests.**

> Keep side effects at the **edges**. The thin `cli::run` shell does the two
> impure things — platform config-root discovery and choosing the output
> streams — then passes their *results* into the subcommand runners as plain
> parameters. The runners are therefore unit-testable against a temp directory
> and `std::ostringstream`s.

**Will I be able to unit-test `cli::run`?** Yes, in the sense that matters: all
of its logic is reachable and tested through the pieces it calls — the runners
(with an injected config root and streams), the subcommand split helper, and
config-path discovery. `run` itself stays a ~15-line shell, so there is little
*in it* to test beyond what the seams already cover; you *can* call it from a
test, but then you exercise real platform discovery, which we avoid by testing
the runners directly.

> **Library/target naming.** The executable target changes the name from `gta3sc-cli`
> to `gta3sc-cli-exe` it is the target users pass to `--target`, and building it
> transitively builds the library). CMake forbids two targets sharing a name,
> so the library is `gta3sc-cli`. The slight `lib name ≠ namespace`
> mismatch is the cost of keeping the user-facing target name stable.

### Control flow — subcommand owns its parsing

The router dispatches to a subcommand runner, and **the runner owns option
parsing.** Each subcommand decides which flags it accepts; shared flag groups
are parsed by shared helper functions. This mirrors `git <subcommand> <flags>`
and is the change requested in review.

```
main(argc, argv)
  → cli::run(argc, argv [, out, err])
      → views = span<string_view> over argv (zero-copy, §4.1)
      → if --help / --version anywhere: print generic message; return success  (§4.3)
      → (subcommand, rest) = split_subcommand(views)   // no subcommand ⇒ compile
      → config_root = find_config_root()               // impure edge; std::optional
      → dispatch:
          compile           → run_compile(rest, config_root, out, err)
          decompile         → run_decompile(rest, config_root, out, err)   // Phase 6 (stub)
          query-config-path → run_query_config_path(config_root, out, err)  // Phase 6
          query-models      → run_query_models(rest, config_root, out, err) // Phase 6

run_compile(args, config_root, out, err):
  CompileOptions opts;
  if(!parse_compile_options(OptionParser(args), opts, err)) return EXIT_FAILURE;
  // validate: input present, config_name present, config_root present
  // bootstrap: load_config → scan input-stem dir → driver::Compilation
  // write output (-o or <stem>.scm); return exit code
```

There is **no monolithic parse step** producing one big struct. Each subcommand
builds its own options object (§4.2) from shared groups.

### `Workspace` / `CompileJob` — still rejected (with a nuance)

The two-layer long-lived/short-lived session is **still not built.** The
subcommand-first design does *not* force it back.

What the design *does* introduce is **shared bootstrap**: `compile` and
`query-models` both need to turn a config name into a loaded `CommandTable`
(and optionally a `ModelTable`). The right expression of that is a **free
function**, not a long-lived stateful service:

```cpp
// Phase 6, when the second consumer (query-models) appears.
auto load_command_table(const ConfigOptions& config,
                        const std::filesystem::path& config_root,
                        SourceManager& sources, DiagnosticHandler& diag,
                        ArenaAllocator<> arena) -> std::optional<CommandTable>;
// TODO probably we don't pass config_root + source but rather a path resolver?
```

Because the tables are arena-backed, the *caller* owns the arenas (locals in the
runner) and passes an allocator in — no owning "environment" object is needed
yet. In Phase 3 only `compile` exists, so even this helper is deferred: the
compile runner keeps the arenas/tables as locals. A long-lived holder (call it
`CompilerEnvironment` when it arrives) is justified only once something reuses
loaded tables across *multiple* compilations (LSP, watch-mode). Recorded as a
future seam, not a deliverable.

### Names (challenged and chosen)

| Concept                     | Chosen                                                                                                                                    | Rejected / alternatives                       | Reasoning                                                                            |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------------ |
| argv cursor / primitives    | `cli::OptionParser`                                                                                                                       | `ArgCursor`, port `optget`/`optflag`          | matches the user's framing; a stateful forward cursor over args                      |
| ~~parsed result aggregate~~ | **dropped**                                                                                                                               | `CommandLine`, `DriverSettings`, `Invocation` | no monolithic aggregate in a subcommand-first design (§3.2, §10)                     |
| per-subcommand options      | `cli::CompileOptions` (+ shared `ConfigOptions`, later `LanguageOptions`, `MachineOptions`)                                               | one global `Options`                          | each subcommand composes the shared groups it accepts                                |
| shared flag-group parser    | `cli::parse_language_option(OptionParser&, LanguageOptions&) -> bool`                                                                     | —                                             | free function reused by `compile`/`decompile`; returns whether it consumed an option |
| subcommand runner           | `cli::run_compile`, `cli::run_decompile`, …                                                                                               | —                                             | one `run_<subcommand>.cpp` file each (§4.2)                                          |
| entry point                 | `cli::run(int argc, char** argv, std::ostream& out, std::ostream& err) -> int` (+ a 2-arg overload defaulting to `std::cout`/`std::cerr`) | `run_cli`, `main`                             | returns a process exit code; streams injectable for tests                            |
| CLI error printer           | `cli::report_error(std::ostream& err, fmt, args…)`                                                                                        | routing through `DiagnosticDescriptor`        | argv mistakes have no source location (§4.5)                                         |

We use namespace **`gta3sc::cli`** (not `gta3sc::driver`): `driver` is the
embeddable, argv-free pipeline (`driver::Compilation`); `cli` is the argv
frontend (tokenising, routing, exit codes). Nothing in `cli` is useful to an
embedder without a `char** argv`.

### Thin `main.cpp`

```cpp
#include <gta3sc/cli/run.hpp>

int main(int argc, char** argv)
{
    return gta3sc::cli::run(argc, argv);
}
```

## Component design

### `cli::OptionParser` (Phase 1)

A throw-free forward cursor over `std::span<const std::string_view>`. It is the
clean re-expression of legacy `optget`/`optflag`/`optint`.

**Why `span<string_view>` and not raw `argc`/`argv`?** The conversion from argv
is **zero-copy** — each `string_view` points into the existing argv buffer
(which lives for the whole program), so we only build a small vector of views,
never copy strings. We choose views because:

1. **`commandline.txt` (Phase 5) is not argv.** Tokenising the file yields
   `string_view`s into the file buffer. A single `span<string_view>` interface
   lets the *same* parser drive both argv and `commandline.txt` tokens.
2. **Testability.** A test builds `std::vector<std::string_view>{ "-o", "x" }`
   directly; it never has to fabricate a null-terminated `char**`.
3. **Safety/ergonomics.** Length-aware views make `--opt=value` splitting and
   prefix checks trivial versus `char*` + `strncmp`.

```cpp
#pragma once
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace gta3sc::cli
{
/// Result of attempting to match a value-bearing option.
///
/// Separates "did the option name match?" from "was a value present?".
template<typename T>
struct OptionMatch
{
    bool matched{false};      ///< The option name matched at the cursor.
    std::optional<T> value;   ///< The value; empty when the match errored.

    explicit operator bool() const noexcept { return matched; }
};

/// Forward cursor over command-line arguments with GCC-style option matching.
///
/// Re-expresses the legacy `optget`/`optflag`/`optint` helpers as a single
/// stateful cursor. Matchers consume from the front on a successful match and
/// leave the cursor untouched on no-match. Errors are recorded in a fail-fast
/// failure state (§4.5), never thrown.
class OptionParser
{
public:
    explicit OptionParser(std::span<const std::string_view> args) noexcept;

    /// Whether there are no more options to parse.
    [[nodiscard]] auto eof() const noexcept -> bool;

    /// Peeks the next token in the options stream.
    ///
    /// Returns `std::nullopt` if the stream has no more options.
    [[nodiscard]] auto peek() const noexcept -> std::optional<std::string_view>;

    /// Consumes the next token in the options stream.
    ///
    /// Returns `std::nullopt` if the stream has no more options.
    auto next() noexcept -> std::optional<std::string_view>;

    /// Consumes the next token if the cursor sits on a positional (i.e. does
    /// not start with "-").
    [[nodiscard]] auto positional() const noexcept ->
    std::optional<std::string_view>;

    /// Matches a valueless option, e.g. `option("-h", "--help")`.
    auto option(std::string_view short_opt, std::string_view long_opt) noexcept
            -> std::optional<bool>;

    /// Matches a `-fname` / `-fno-name` flag toggle.
    ///
    /// `-f` can be exchanged with any other letter (e.g. `-m`).
    ///
    /// Returns the value of the matched toggle flag or `std::nullopt` if no match.
    auto toggle(std::string_view name) noexcept -> std::optional<bool>;

    /// Matches a set-only `-fname`-style flag (no `-fno-` form).
    auto flag(std::string_view name) noexcept -> std::optional<bool>;

    /// Matches an option taking exactly one value (`--opt=v`, `--opt v`, `-ov`,
    /// `-o v`).
    ///
    /// On a name match with a missing value, records a failure and
    /// the returned `OptionMatch.value` is not set.
    auto value(std::string_view short_opt, std::string_view long_opt) noexcept
            -> OptionMatch<std::string_view>;

    /// Matches a value option whose value is an integer.
    ///
    /// Records a failure when the value is absent, not an integer, or out of
    /// range for \p IntegralType.
    template<typename IntegralType>
    auto value_int(std::string_view long_opt) noexcept -> OptionMatch<IntegralType>;

    /// Whether a matcher hit a recognised-but-invalid argument (§4.5).
    [[nodiscard]] auto failed() const noexcept -> bool;

    /// The single failure message (valid when \ref failed).
    [[nodiscard]] auto error() const noexcept -> std::string_view;

private:
    std::size_t pos{0};
    std::span<const std::string_view> args;
    std::string error_message; ///< Single fail-fast message; see error().
    bool has_failed{false};
};
} // namespace gta3sc::cli
```

A subcommand's parse loop (sketch):

```cpp
// TODO use if...elseif rather than continue
while(!parser.empty() && !parser.failed())
{
    if(auto m = parser.positional()) { /* input file */ continue; }
    if(auto m = parser.value("-o", ""))       { if(m.value) opts.output = *m.value; continue; }
    if(auto m = parser.value("", "--config")) { if(m.value) opts.config.name = *m.value; continue; }
    // Phase 4: if(parse_language_option(parser, opts.language)) continue; ...
    report_error(err, "unrecognized argument '{}'", parser.peek());
    return false;
}
if(parser.failed()) { report_error(err, "{}", parser.error()); return false; }
```

Because `OptionMatch::operator bool` means "recognised", a match with a missing
value still takes the `if` branch (so we `continue`), the `while`-condition then
sees `failed()` and exits, and the loop reports `parser.error()` once. A genuine
unknown token reaches the final `report_error` and stops.

### Subcommand runners and shared option groups (Phases 3+)

Each subcommand lives in its own file (`lib/cli/run-compile.cpp`,
`run-decompile.cpp`, `run-query-config-path.cpp`, `run-query-models.cpp`)
and exposes a `run_<name>` function plus a testable `parse_<name>_options`:

```cpp
namespace gta3sc::cli
{
/// Game-config selection, shared by every subcommand that loads a config.
struct ConfigOptions
{
    std::string name;                                    ///< From `--config`.
    std::vector<std::filesystem::path> add_config_files; ///< Phase 4.
};

/// Everything the `compile` subcommand accepts. Grows field-by-field as flags
/// are honoured (Phase 4); no fields exist for unwired flags.
struct CompileOptions
{
    std::filesystem::path input;
    std::filesystem::path output;   ///< Empty → `<input-stem>.scm`.
    ConfigOptions config;
    // Phase 4+: LanguageOptions language; MachineOptions machine; datadir; …
};

/// Parses the `compile` argument list into \p result.
///
/// Returns false and reports via \p parser on the first error (fail-fast).
auto parse_compile_options(OptionParser& parser, CompileOptions& result) -> bool;

/// Runs the `compile` subcommand end-to-end.
auto run_compile(std::span<const std::string_view> args,
                 const std::optional<std::filesystem::path>& config_root,
                 std::ostream& out, std::ostream& err) -> int;
} // namespace gta3sc::cli
```

Shared flag groups (Phase 4) are parsed by free helpers used across
subcommands, exactly as requested:

```cpp
/// Consumes one language option at the cursor if present.
/// \returns true if it consumed an option (check `parser.failed()` for errors).
auto parse_language_option(OptionParser& parser, LanguageOptions& result) -> bool;
auto parse_machine_option(OptionParser& parser, MachineOptions& result) -> bool;
auto parse_config_option(OptionParser& parser, ConfigOptions& result) -> bool;
```

`parse_compile_options` calls these in its loop; a future `parse_decompile_options`
reuses `parse_config_option` (and whichever groups decompile shares). Each
subcommand thus controls precisely which groups it accepts.

> **Dropped: `cli::CommandLine`.** A single parsed aggregate made sense only for
> a monolithic parser. With per-subcommand parsing, the aggregate is the
> subcommand's own options object (`CompileOptions`), composed of shared groups.

### `--help` / `--version` across subcommands (Phase 2)

Legacy prints **one generic message** regardless of context, and we keep that.
The simplest faithful implementation: `cli::run` does a **pre-scan** of the
arguments for `-h`/`--help` (and `--version`) before routing, and short-circuits
with the single generic message. Consequences, all acceptable:

- `gta3sc --help`, `gta3sc compile --help`, `gta3sc query-models --help` all
  print the **same** generic help and exit success. This matches legacy and the
  reviewer's "same generic message for all" instruction.
- Help/version win over "missing input/config" (they short-circuit first), as
  the discussion already allowed.
- A pathological value that is literally `--help` (e.g. `-o --help`) would be
  treated as help. Vanishingly unlikely for these two flags; should be documented
  and accepted to avoid threading help/version state through every runner.
- `--help` and `--version` act as if they were a subcommand of their own.

The generic text is a single constant (`cli::help_text`, `cli::version_text`)
printed to the `out` stream. Per-subcommand help and a declarative flag/help
model are **explicitly out of scope** for now; the constant is a fine starting
point and we revisit only if a real need appears.

### Config path discovery (Phase 3)

Port legacy `find_config_path` with a testable seam: split *candidate
generation* (pure) from *selection* (first existing directory).

```cpp
/// Ordered config-root candidates for the host platform.
auto config_search_paths(const std::filesystem::path& exe_dir,
                         const std::filesystem::path& home)
        -> std::vector<std::filesystem::path>;

/// First candidate from \ref config_search_paths that is a directory.
auto find_config_root() -> std::optional<std::filesystem::path>;
```

Order matches legacy: exe-adjacent `config/`, then
`$HOME/.local/share/gta3sc/config`, then `/usr/share/gta3sc/config`; on Windows
the module directory's `config`. Executable-path lookup stays in platform
`#ifdef`s in the `.cpp`; `config_search_paths` is pure and unit-tested with
synthetic `exe_dir`/`home`.

`run` resolves the root once (impure edge) and passes the `std::optional` into
runners, so runners are tested by passing a temp-directory root — no env-var or
global state.

### Errors, diagnostics, exit codes

Two distinct channels:

- **CLI-level errors** (unknown flag, missing value, no input, missing
  `--config`, no config root) have **no source location** and are *not* routed
  through `DiagnosticDescriptor`. A tiny helper prints to the injected `err`:

  ```cpp
  template<typename... Args>
  void report_error(std::ostream& err, std::format_string<Args...> fmt,
                    Args&&... args); // → "gta3sc: error: <formatted>\n"
  ```

  **Where it connects:** the subcommand runners
  call it — when `OptionParser::failed()` (printing `parser.error()`), on an
  unknown token, and during validation (`no input file`, `no game config
  specified [--config=<name>]`, `could not find config directory`). It is the
  one place CLI errors reach stderr.

- **`OptionParser` failure state** is **single-message, fail-fast**: one
  `std::string error_message` returned by `error()` as a `string_view`. We do
  **not** support multiple errors or multi-line messages, and we do not need to:
  argv parsing is fail-fast (the first bad token is the actionable one), and
  collecting more would require continue-after-error cursor semantics for no
  real UX gain. (Contrast the compiler's *source* parser, which `BACKLOG.md`
  wants to accumulate errors — different problem, different component.) If
  multi-error CLI reporting is ever wanted, it becomes a diagnostic sink later.

- **Pipeline diagnostics** (parse/sema/codegen) flow through a
  `CallbackDiagnosticHandler` whose callback renders to `err`. Full
  diagnostic-to-string formatting is a separate `BACKLOG.md` item; Phase 3 ships
  a **minimal renderer** (descriptor title for now) with a clear TODO. Success
  is decided by `Compilation::compile`'s return value, not by the renderer.

Exit codes: `int` using `EXIT_SUCCESS` / `EXIT_FAILURE`. No enum.

### Output writing (Phase 3)

`run_compile` writes the `std::vector<std::byte>` from `Compilation::compile` to
the `-o` path, or `<input-stem>.scm`. CLEO `.cs`/`.cm` and headerless output
depend on codegen flags that are not wired yet (Phase 4+); Phase 3 emits the
standard header+relocation `main.scm` that `Compilation` produces today.

### Stream injection and testability

`cli::run` and every `run_<subcommand>` take `std::ostream& out, std::ostream&
err`. A 2-argument `cli::run(argc, argv)` overload defaults them to
`std::cout`/`std::cerr`; `main` uses that. Tests pass `std::ostringstream`s to
assert on help text, query output, and error messages, and use
`WithTempDirFixture` for config trees and inputs — making the runners
hermetically testable.

## Decisions (resolved here; revisit only with cause)

- **Subcommand owns parsing.** Router → `run_<subcommand>` → `OptionParser`;
  shared `parse_*_option` helpers parse shared groups. No monolithic parser.
- **No `cli::CommandLine`.** Each subcommand has its own options object composed
  of shared groups (`ConfigOptions`, later `LanguageOptions`, …).
- **No extension-based action inference.** No subcommand ⇒ compile; decompile is
  explicit.
- **`--help`/`--version`** are global short-circuits via a pre-scan in `run`,
  printing one generic message for every subcommand (legacy behaviour).
  Per-subcommand help is out of scope.
- **Honour-or-error.** Any flag not fully wired is a hard "unrecognized
  argument" error. No parse-only stubs, no accept-and-ignore. Consequence:
  `commandline.txt` (Phase 5) is meaningful only once its flags are honoured, and
  any shipped `commandline.txt` must reference only honoured flags.
- **`commandline.txt` optional** until the config submodule ships one;
  `--config` works from `config.xml` alone (no legacy "missing commandline.txt"
  error).
- **`OptionParser` errors** are single-message, fail-fast (§4.5).
- **All CLI logic in `gta3sc-cli`**, `src/` is just `main`; streams injected
  for hermetic tests.
- **`Workspace`/`CompileJob` rejected**; shared bootstrap is a free function
  (Phase 6), not a session.
- **Exit codes** are plain `int`/`EXIT_*`.

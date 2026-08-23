# Integration Tests

End-to-end tests that run `gta3sc` against real script sources and check its output.
## Requirements

- Python 3
- [lit](https://pypi.org/project/lit/)¹
- [filecheck](https://pypi.org/project/filecheck/)¹
- Unix utilities: `sh`, `md5sum`, `curl`, `tar`, `gzip`².

¹: `lit` and `filecheck` must be on `PATH`. Install it anyway you like, you can generally install them using `pipx install lit filecheck`, or `uv tool install lit && uv tool install filecheck`, or even through distro package managers e.g. `dnf install python3-lit python3-filecheck`. 

²: The best way to acquire these utilities on Windows is by installing Git For Windows, MSYS2 or Cygwin. Then open their respective terminal.

## Running

### Via CMake / CTest

Integration tests are **off by default** so a plain `ctest` only runs unit
tests. Enable integration at configure time to run them.

Build the compiler, then run the`gta3sc_integration` (or `ctest`, or `ctest -L integration`):

    cmake -S . -B build -DGTA3SC_INTEGRATION_TESTS=ON
    cmake --build build --target gta3sc-cli-exe
    ctest --test-dir build -R gta3sc_integration --output-on-failure

CTest passes the freshly built binary to lit via `--param gta3sc=<path>`.

### Standalone

The binary under test must be set explicitly (`--param gta3sc=` or `GTA3SC`).

    lit --param gta3sc=/path/to/gta3sc test -v
    # or
    export GTA3SC=/path/to/gta3sc
    lit test --verbose
    # or a single suite
    lit test/codegen --verbose

## Writing tests

A test is a `.sc` (or `.test`) file with one or more `RUN:` lines. If any `RUN`
command fails, the test fails. Only files directly under a suite directory
(e.g. `codegen/`) are discovered; deeper files are support scripts referenced by
a test. Directories named `Inputs/` are not discovered as tests (they hold
fixtures such as `frontend/Inputs/*.xml` and `semantics/Inputs/data/`).

Substitutions available in `RUN` lines:

| Name        | Meaning |
|-------------|---------|
| `%s`, `%t`, `%S`, `%T`, … | Standard [lit substitutions](https://llvm.org/docs/CommandGuide/lit.html#substitutions). |
| `%gta3sc`   | The `gta3sc` binary under test (with `-Wno-expect-var`). |
| `%FileCheck`| [FileCheck](https://llvm.org/docs/CommandGuide/FileCheck.html). |
| `%verify`   | Clang-style `-verify`: checks stdin against `// expected-error`/`// expected-warning` annotations (`verify-diagnostics.py`). |
| `%gta3sc-filecheck` | Compile `%s` and FileCheck the stdout against that same file. |
| `%gta3sc-verify` | Compile `%s` and check diagnostics. |
| `%checksum` | Asserts the md5sum of `$1` equals `$2`. |
| `%not`      | Inverts the exit code (unless the program crashed). |

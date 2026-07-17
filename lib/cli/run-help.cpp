#include <cstdlib>
#include <gta3sc/cli/run-help.hpp>
#include <string_view>

namespace
{
// TODO replace by gta3sc-legacy help text?
constexpr std::string_view help_text
        = R"(Usage: gta3sc [<subcommand>] [<options>] <input>

Subcommands:
  compile          Compile a GTA3Script source file (default)
  decompile        Decompile a compiled script

Options:
  --config=<name>  Select a game configuration (required for compile)
  -o <file>        Output file (default: <input-stem>.scm)
  -h, --help       Print this help text
  -v, --version    Print the version
)";
} // namespace

namespace gta3sc::cli
{
auto run_help(std::ostream& out) -> int
{
    out << help_text;
    return EXIT_SUCCESS;
}
} // namespace gta3sc::cli

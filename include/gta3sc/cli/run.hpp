#pragma once
#include <gta3sc/cli/run-help.hpp>
#include <gta3sc/cli/run-version.hpp>
#include <ostream>

namespace gta3sc::cli
{
/// Entry point for the gta3sc CLI.
///
/// Parses \p argv, dispatches to the appropriate subcommand, and returns an
/// exit code suitable for passing to `std::exit`.
///
/// \param argc Argument count (as received by `main`).
/// \param argv Argument vector (as received by `main`).
/// \param out  Standard output stream.
/// \param err  Standard error stream.
auto run(int argc, char** argv, std::ostream& out, std::ostream& err) -> int;

} // namespace gta3sc::cli

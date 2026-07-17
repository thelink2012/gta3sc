#pragma once
#include <ostream>

namespace gta3sc::cli
{
/// Prints generic help text to \p out.
auto run_help(std::ostream& out) -> int;

} // namespace gta3sc::cli

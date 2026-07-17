#pragma once
#include <ostream>

namespace gta3sc::cli
{
/// Prints the version string to \p out.
auto run_version(std::ostream& out) -> int;

} // namespace gta3sc::cli

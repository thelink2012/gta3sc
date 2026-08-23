#include <cstdlib>
#include <gta3sc/cli/run-version.hpp>
#include <gta3sc/cli/version.hpp>

namespace gta3sc::cli
{
auto run_version(std::ostream& out) -> int
{
    out << "gta3sc " << version() << '\n';
    return EXIT_SUCCESS;
}
} // namespace gta3sc::cli

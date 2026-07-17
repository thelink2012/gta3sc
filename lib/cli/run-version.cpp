#include <cstdlib>
#include <gta3sc/cli/run-version.hpp>
#include <string_view>

namespace
{
// TODO: wire `project(gta3sc VERSION …)` + git-describe when available.
constexpr std::string_view version_text = "gta3sc (rewrite) 0.0.0\n"
                                          "TODO: wire project VERSION + "
                                          "git-describe\n";
} // namespace

namespace gta3sc::cli
{
auto run_version(std::ostream& out) -> int
{
    out << version_text;
    return EXIT_SUCCESS;
}
} // namespace gta3sc::cli

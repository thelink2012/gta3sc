#pragma once
#include <format>
#include <ostream>

namespace gta3sc::cli
{
/// Prints a CLI-level error to \p err in the form `"gta3sc: error: <msg>\n"`.
template<typename... Args>
void report_error(std::ostream& err, std::format_string<Args...> fmt,
                  Args&&... args)
{
    err << "gta3sc: error: "
        << std::vformat(fmt.get(), std::make_format_args(args...)) << '\n';
}
} // namespace gta3sc::cli

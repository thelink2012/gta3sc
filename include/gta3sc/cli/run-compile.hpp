#pragma once
#include <filesystem>
#include <gta3sc/cli/option-parser.hpp>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace gta3sc::cli
{
/// Game-config selection options, shared by every subcommand that loads a
/// config.
struct ConfigOptions
{
    std::string name; ///< Config name from `--config`.
};

/// Options for the `compile` subcommand.
struct CompileOptions
{
    std::filesystem::path input; ///< Positional input file.
    std::filesystem::path
            output;       ///< From `-o`; empty means `<input-stem>.scm`.
    ConfigOptions config; ///< From `--config`.
};

/// Tries to consume ONE recognised option at the current cursor position
/// and accumulates the result in \p result.
///
/// \returns `(matched=false, _)` when no option was identified and consumed,
// `(matched=true, std::nullopt)` when an option was consumed but there's a
// parse error.
// `(matched=true, std::monostate)` when an option was successfully consumed and
// parsed.
auto parse_compile_options(OptionParser& parser, CompileOptions& result)
        -> OptionMatch<std::monostate>;

/// Runs the `compile` subcommand end-to-end.
///
/// \param args        Arguments after the `compile` subcommand word .
/// \param config_root Root config directory as returned by
///                    `config::find_config_root()`.
/// \param out         Standard output.
/// \param err         Standard error.
auto run_compile(std::span<const std::string_view> args,
                 const std::filesystem::path& config_root, std::ostream& out,
                 std::ostream& err) -> int;

} // namespace gta3sc::cli

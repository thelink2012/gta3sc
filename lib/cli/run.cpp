#include "error.hpp"
#include <cstdlib>
#include <gta3sc/cli/run-compile.hpp>
#include <gta3sc/cli/run-help.hpp>
#include <gta3sc/cli/run-version.hpp>
#include <gta3sc/config/config-path.hpp>
#include <string_view>
#include <vector>

namespace
{
enum class Action
{
    compile,
    decompile,
    query_config_path,
    query_models,
    help,
    version
};

auto split_action(std::span<const std::string_view> argv)
        -> std::pair<Action, std::span<const std::string_view>>
{
    if(argv.empty())
        return {Action::compile, argv};

    auto first = argv[0];
    auto rest = argv.subspan(1);

    if(first == "--help" || first == "-h")
        return {Action::help, rest};
    else if(first == "--version" || first == "-v")
        return {Action::version, rest};
    else if(first == "compile")
        return {Action::compile, rest};
    else if(first == "decompile")
        return {Action::decompile, rest};
    else if(first == "query-config-path")
        return {Action::query_config_path, rest};
    else if(first == "query-models")
        return {Action::query_models, rest};
    else
        return {Action::compile, argv};
}
} // namespace

namespace gta3sc::cli
{
auto run(int argc, char** argv, std::ostream& out, std::ostream& err) -> int
{
    if(argc < 1)
        return run_help(out);

    std::vector<std::string_view> view(argv + 1, argv + argc);
    auto [action, rest] = split_action(view);

    auto config_root = gta3sc::config::find_config_root();

    switch(action)
    {
        case Action::help:
            return run_help(out);
        case Action::version:
            return run_version(out);
        case Action::compile:
            if(!config_root)
            {
                report_error(err, "could not find config/ directory");
                return EXIT_FAILURE;
            }
            return run_compile(rest, *config_root, out, err);
        default:
            report_error(err, "subcommand not yet implemented");
            return EXIT_FAILURE;
    }
}
} // namespace gta3sc::cli

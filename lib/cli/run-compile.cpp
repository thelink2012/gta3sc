#include "error.hpp"
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <gta3sc/cli/run-compile.hpp>
#include <gta3sc/cli/run.hpp>
#include <gta3sc/command-table.hpp>
#include <gta3sc/config/config.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/util/arena.hpp>
#include <vector>

namespace
{
using ExitCode = int;

using gta3sc::ArenaAllocator;
using gta3sc::ArenaMemoryResource;
using gta3sc::CommandTable;
using gta3sc::FilePool;
using gta3sc::ModelTable;
using namespace gta3sc::cli;

auto monostate(std::string_view) -> std::monostate;

auto parse_compile_args(std::span<const std::string_view> args,
                        std::ostream& out, std::ostream& err)
        -> std::expected<CompileOptions, ExitCode>;

auto validate_compile_options(const CompileOptions& opts,
                              std::ostream& err) -> bool;

auto load_command_table(const ConfigOptions& opts,
                        const std::filesystem::path& config_root,
                        std::ostream& err,
                        ArenaAllocator<> arena) -> std::optional<CommandTable>;

auto load_model_table(ArenaAllocator<> arena) -> std::optional<ModelTable>;

auto scan_subscripts(const std::filesystem::path& input,
                     gta3sc::SourceManager& source_manager,
                     std::ostream& err) -> bool;

auto resolve_output_path(const std::filesystem::path& input,
                         const std::filesystem::path& output)
        -> std::filesystem::path;

auto write_output(const std::filesystem::path& output_path,
                  std::span<const std::byte> output_bytes,
                  std::ostream& err) -> bool;
} // namespace

namespace gta3sc::cli
{
auto parse_compile_options(OptionParser& parser, CompileOptions& result)
        -> OptionMatch<std::monostate>
{
    if(auto m = parser.positional())
    {
        result.input = *m;
        return {true, std::monostate{}};
    }
    else if(auto m = parser.value("-o", ""))
    {
        if(m.value)
            result.output = *m.value;
        return {true, m.value.transform(monostate)};
    }
    else if(auto m = parser.value("", "--config"))
    {
        if(m.value)
            result.config.name = *m.value;
        return {true, m.value.transform(monostate)};
    }
    return {};
}

auto run_compile(std::span<const std::string_view> args,
                 const std::filesystem::path& config_root, std::ostream& out,
                 std::ostream& err) -> int
{
    auto opts = parse_compile_args(args, out, err);
    if(!opts)
        return opts.error();

    if(!validate_compile_options(*opts, err))
        return EXIT_FAILURE;

    ArenaMemoryResource command_table_arena;
    auto command_table = load_command_table(opts->config, config_root, err,
                                            &command_table_arena);
    if(!command_table)
        return EXIT_FAILURE;

    gta3sc::ArenaMemoryResource model_table_arena;
    auto model_table = load_model_table(&model_table_arena);
    if(!model_table)
        return EXIT_FAILURE;

    gta3sc::SourceManager source_manager;

    bool had_error = false;
    gta3sc::CallbackDiagnosticHandler diag_handler(
            [&err, &had_error](const gta3sc::Diagnostic& diag) {
                // TODO: full diagnostic formatting (see BACKLOG.md)
                err << "diag: " << diag.descriptor->title() << '\n';
                had_error = true;
            });

    if(!scan_subscripts(opts->input, source_manager, err))
        return EXIT_FAILURE;

    std::vector<std::byte> output_bytes;
    gta3sc::driver::Compilation compilation(opts->input, *command_table,
                                            *model_table, source_manager,
                                            diag_handler);
    if(!compilation.compile({&output_bytes}))
        return EXIT_FAILURE;

    const auto output_path = resolve_output_path(opts->input, opts->output);
    if(!write_output(output_path, output_bytes, err))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

} // namespace gta3sc::cli

namespace
{
auto monostate(std::string_view) -> std::monostate
{
    return {};
}

auto parse_compile_args(std::span<const std::string_view> args,
                        std::ostream& out, std::ostream& err)
        -> std::expected<CompileOptions, ExitCode>
{
    OptionParser parser(args);
    CompileOptions opts;

    while(!parser.eof() && !parser.failed())
    {
        if(parser.option("-h", "--help"))
            return std::unexpected(run_help(out));
        else if(parser.option("-v", "--version"))
            return std::unexpected(run_version(out));
        else if(!parse_compile_options(parser, opts))
            parser.fail("unrecognized argument '{}'", *parser.peek());
    }

    if(parser.failed())
    {
        report_error(err, "{}", parser.error());
        return std::unexpected(EXIT_FAILURE);
    }

    return opts;
}

auto validate_compile_options(const CompileOptions& opts,
                              std::ostream& err) -> bool
{
    if(opts.input.empty())
    {
        report_error(err, "no input file");
        return false;
    }
    else if(opts.config.name.empty())
    {
        report_error(err, "no game config specified [--config=<name>]");
        return false;
    }
    return true;
}

auto load_command_table(const ConfigOptions& opts,
                        const std::filesystem::path& config_root,
                        std::ostream& err,
                        ArenaAllocator<> arena) -> std::optional<CommandTable>
{
    FilePool file_pool;

    // TODO find a way to share diagnistics across this, model loading and
    // compilation.
    bool had_error = false;
    gta3sc::CallbackDiagnosticHandler diag_handler(
            [&err, &had_error](const gta3sc::Diagnostic& diag) {
                // TODO: full diagnostic formatting (see BACKLOG.md)
                err << "diag: " << diag.descriptor->title() << '\n';
                had_error = true;
            });

    // TODO consider passing RelativePathResolver to load_config
    auto builder = gta3sc::config::load_config(
            config_root, config_root / opts.name / "config.xml", file_pool,
            diag_handler, gta3sc::CommandTable::Builder(arena));

    if(had_error)
        return {};

    return std::move(builder).build();
}

auto load_model_table(ArenaAllocator<> arena) -> std::optional<ModelTable>
{
    // TODO return a default and a level model table, or have two functions.
    // TODO empty model table (model loading is Phase 4+)
    gta3sc::ModelTable model_table = gta3sc::ModelTable::Builder(arena).build();
    return model_table;
}

auto scan_subscripts(const std::filesystem::path& input,
                     gta3sc::SourceManager& source_manager,
                     std::ostream& err) -> bool
{
    auto subscripts_dir = input;
    subscripts_dir.replace_extension();

    if(!std::filesystem::is_directory(subscripts_dir))
        return true;

    if(!source_manager.scan_directory(subscripts_dir))
    {
        report_error(err, "failed to scan directory '{}'",
                     subscripts_dir.string());
        return false;
    }

    return true;
}

auto resolve_output_path(const std::filesystem::path& input,
                         const std::filesystem::path& output)
        -> std::filesystem::path
{
    if(!output.empty())
        return output;

    auto output_path = input;
    output_path.replace_extension(".scm");
    return output_path;
}

auto write_output(const std::filesystem::path& output_path,
                  std::span<const std::byte> output_bytes,
                  std::ostream& err) -> bool
{
    std::ofstream ofs(output_path, std::ios::binary);
    if(!ofs)
    {
        report_error(err, "cannot open output file '{}'", output_path.string());
        return false;
    }

    ofs.write(reinterpret_cast<const char*>(output_bytes.data()),
              static_cast<std::streamsize>(output_bytes.size()));
    if(!ofs)
    {
        report_error(err, "error writing output file '{}'",
                     output_path.string());
        return false;
    }

    return true;
}
} // namespace

// TODO is it a problem if we have MAIN.sc as the main file and then
// MAIN/MAIN.sc as another file?

// TODO rename Multifile classes to Multiscript?
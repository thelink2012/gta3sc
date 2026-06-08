#include <filesystem>
#include <gta3sc/command-table.hpp>
#include <gta3sc/config/config.hpp>
#include <gta3sc/config/models.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/filesystem/file-location.hpp>
#include <gta3sc/filesystem/path-resolver.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/util/arena.hpp>
#include <print>
#include <vector>

using namespace gta3sc;

// TODO
//   - lower MISSION_START/END
//   - sema handle filename

// TODO is it a problem if we have MAIN.sc as the main file and then
// MAIN/MAIN.sc as another file?

// TODO rename Multifile classes to Multiscript?

int main()
{
    // TEMP: gtavc_main integration test (see gta3sc test/main/gtavc.test)
    const std::filesystem::path input_file = "build/gtavc-test/main.sc";

    const std::filesystem::path config_root_path
            = "/home/denimorim/dev/gta3script-config/config";
    const std::filesystem::path config_path = config_root_path
                                              / "gtavc/config.xml";

    const std::filesystem::path level_root_path
            = "/home/denimorim/Downloads/OriginalData/ViceCity";
    const std::filesystem::path level_path = level_root_path
                                             / "data/gta_vc.dat";

    gta3sc::SourceManager file_manager;
    std::vector<gta3sc::Diagnostic> diagnostics;
    gta3sc::CallbackDiagnosticHandler diag_manager(
            [&](const gta3sc::Diagnostic& diag) {
                diagnostics.push_back(diag);
                std::println(stderr, "error: {} (loc {})",
                             diag.descriptor->title(),
                             static_cast<uint32_t>(diag.location));
                if(diag.location != gta3sc::no_file_loc)
                {
                    const gta3sc::FileRange snippet_range{diag.location, 80};
                    if(const auto snippet = file_manager.string_copy_of(
                               snippet_range))
                        std::println(stderr, "  context: {}", *snippet);
                }
            });

    gta3sc::ArenaMemoryResource models_arena;
    gta3sc::ArenaMemoryResource command_table_arena;

    // TODO use a different file pool for load_config
    gta3sc::CommandTable::Builder commands_builder(&command_table_arena);
    commands_builder = gta3sc::config::load_config(
            config_root_path, config_path, file_manager, diag_manager,
            std::move(commands_builder));
    // TEMP (brute-force): default.xml carries the full DEFAULTMODEL table
    // (MAFIA, BFINJECT, …). constants.xml only has a partial set — see
    // brute-force-summary.md issue 5.
    commands_builder = gta3sc::config::load_config(
            config_root_path, config_root_path / "gtavc/default.xml",
            file_manager, diag_manager, std::move(commands_builder));
    gta3sc::CommandTable command_table = std::move(commands_builder).build();

    gta3sc::ModelTable model_table;
    {
        gta3sc::ModelTable::Builder model_builder(&models_arena);
        gta3sc::filesystem::RelativeInsensitivePathResolver level_path_resolver(
                level_root_path);
        model_builder = gta3sc::config::load_models_from_level(
                level_path_resolver, level_path, true, file_manager,
                diag_manager, std::move(model_builder));
        model_table = std::move(model_builder).build();
    }

    auto input_scripts_dir = input_file;
    input_scripts_dir.replace_extension();
    if(std::filesystem::is_directory(input_scripts_dir))
    {
        if(!file_manager.scan_directory(input_scripts_dir))
        {
            // TODO diag
            std::println(stderr, "error: compilation failed");
            return 1;
        }
    }

    ////////////// COMPILATION START //////////////

    std::vector<std::byte> output;
    gta3sc::driver::Compilation compilation(
            input_file, command_table, model_table, file_manager, diag_manager);
    if(!compilation.compile({&output}))
    {
        std::println(stderr, "error: compilation failed ({} diagnostics)",
                     diagnostics.size());
        return 1;
    }

    ////////////// COMPILATION END //////////////

    auto output_file = input_file;
    output_file.replace_extension(".scm");

    FILE* fout = std::fopen(output_file.c_str(), "wb");
    if(!fout)
    {
        // TODO diag
        std::println(stderr, "error: compilation failed");
        return 1;
    }
    if(std::fwrite(output.data(), 1, output.size(), fout) != output.size())
    {
        // TODO diag
        std::println(stderr, "error: compilation failed");
        return 1;
    }
    std::fclose(fout);

    std::println("SUCCESS!");
    return 0;
}

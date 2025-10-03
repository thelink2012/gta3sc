#include <filesystem>
#include <gta3sc/command-table.hpp>
#include <gta3sc/config/config.hpp>
#include <gta3sc/config/models.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena.hpp>
#include <print>

using namespace gta3sc;

// TODO
//   - lower MISSION_START/END
//   - sema handle filename
//   - parser handle gosub_file etc in parse stmt list

// TODO is it a problem if we have MAIN.sc as the main file and then
// MAIN/MAIN.sc as another file?

// TODO rename Multifile classes to Multiscript?

int main()
{
    const std::filesystem::path input_file = "testscript/test.sc";

    const std::filesystem::path config_root_path
            = "/home/denimorim/dev/gta3script-config/config";
    const std::filesystem::path config_path = config_root_path
                                              / "gta3/config.xml";

    const std::filesystem::path level_root_path
            = "/home/denimorim/Downloads/OriginalData/GTA3";
    const std::filesystem::path default_level_path = level_root_path
                                                     / "data/default.dat";
    const std::filesystem::path level_path = level_root_path / "data/gta3.dat";

    gta3sc::CallbackDiagnosticHandler diag_manager(
            [](const gta3sc::Diagnostic& diag) {
                std::println(stderr, "error: {}", diag.descriptor->title());
            });
    gta3sc::SourceManager file_manager;

    gta3sc::ArenaMemoryResource models_arena;
    gta3sc::ArenaMemoryResource command_table_arena;

    gta3sc::CommandTable::Builder commands_builder(&command_table_arena);
    commands_builder = gta3sc::config::load_config(
            config_root_path, config_path, file_manager, diag_manager,
            std::move(commands_builder));
    gta3sc::CommandTable command_table = std::move(commands_builder).build();

    gta3sc::ModelTable::Builder model_builder(&models_arena);
    // TODO case insensitive DAT loading
    /*model_builder = gta3sc::config::load_models_from_level(
        level_root_path,
        level_path,
        true,
        file_manager,
        diag_manager,
        std::move(model_builder)
    );*/
    gta3sc::ModelTable model_table = std::move(model_builder).build();

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
        std::println(stderr, "error: compilation failed");
        return 1;
    }

    ////////////// COMPILATION END //////////////

    auto output_file = input_file;
    output_file.replace_extension(".cs");

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

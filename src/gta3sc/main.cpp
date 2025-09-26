#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <gta3sc/codegen/relocator.hpp>
#include <gta3sc/command-table.hpp>
#include <gta3sc/config/config.hpp>
#include <gta3sc/config/models.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/preprocessor.hpp>
#include <gta3sc/syntax/scanner.hpp>
#include <gta3sc/syntax/sema.hpp>
#include <gta3sc/util/arena.hpp>
#include <print>

using namespace gta3sc;

namespace gta3sc::driver
{
class Compilation
{
public:
    // TODO improve ctor definition
    Compilation(const std::filesystem::path& input_file,
                CommandTable& command_table, ModelTable& model_table,
                SourceManager& source_manager, DiagnosticHandler& diag_manager);

    Compilation(Compilation&&) noexcept = default;
    auto operator=(Compilation&&) noexcept -> Compilation& = default;

    Compilation(const Compilation&) = delete;
    auto operator=(const Compilation&) -> Compilation& = delete;

    // TODO virtual
    ~Compilation() noexcept = default;

    bool compile();

    // TODO output interface
    std::vector<std::byte> output;

protected:
    auto parse() -> std::optional<LinkedIR<ParserIR>>;
    auto lower(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<ParserIR>>;
    auto sema(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<SemaIR>>;
    auto codegen(const LinkedIR<SemaIR>& ir) -> bool; // TODO output interface

protected:
    std::unique_ptr<ArenaMemoryResource> symbol_arena;
    std::unique_ptr<ArenaMemoryResource> parser_ir_arena;
    std::unique_ptr<ArenaMemoryResource> sema_ir_arena;
    SymbolTable symbol_table;

private: // TODO move some to protected storage maybe?
    std::filesystem::path input_file;
    CommandTable* command_table;
    ModelTable* model_table;
    SourceManager* source_manager;
    DiagnosticHandler* diag_manager;
};
} // namespace gta3sc::driver

namespace gta3sc::driver
{
Compilation::Compilation(const std::filesystem::path& input_file,
                         CommandTable& command_table, ModelTable& model_table,
                         SourceManager& source_manager,
                         DiagnosticHandler& diag_manager) :
    symbol_arena(std::make_unique<ArenaMemoryResource>()),
    parser_ir_arena(std::make_unique<ArenaMemoryResource>()),
    sema_ir_arena(std::make_unique<ArenaMemoryResource>()),
    symbol_table(symbol_arena.get()),
    input_file(input_file),
    command_table(&command_table),
    model_table(&model_table),
    source_manager(&source_manager),
    diag_manager(&diag_manager)
{}

auto Compilation::parse() -> std::optional<LinkedIR<ParserIR>>
{
    syntax::MultifileParser parser(input_file, symbol_table, *source_manager,
                                   *diag_manager, parser_ir_arena.get());
    return parser.parse();
}

auto Compilation::lower(LinkedIR<ParserIR> ir)
        -> std::optional<LinkedIR<ParserIR>>
{
    // TODO lowering pass
    return std::optional<LinkedIR<ParserIR>>(std::move(ir));
}

auto Compilation::sema(LinkedIR<ParserIR> input_ir)
        -> std::optional<LinkedIR<SemaIR>>
{
    syntax::Sema sema(std::move(input_ir), symbol_table, *command_table,
                      *model_table, *diag_manager, sema_ir_arena.get());

    auto sema_ir = sema.validate();
    if(!sema_ir)
        return std::nullopt;

    return sema_ir;
}

auto Compilation::codegen(const LinkedIR<SemaIR>& input_ir) -> bool
{
    const auto storage_options = codegen::StorageTable::Options{};
    auto storage_table = codegen::StorageTable::from_symbols(symbol_table,
                                                             storage_options);
    if(!storage_table)
    {
        // TODO diagman emit not enough storage for variables!?
        return false;
    }

    codegen::RelocationTable reloc_table(symbol_table);

    codegen::trilogy::MultifileCodeGen codegen(symbol_table, *storage_table,
                                               *diag_manager);

    if(!codegen.generate_header_stubs(output))
        return false;

    auto next_file = codegen.generate_first_file(input_ir, reloc_table, output);
    while(next_file && next_file->file)
    {
        next_file = codegen.generate_next_file(
                *next_file->file, next_file->next_ir, input_ir.end(),
                reloc_table, output);
    }

    if(!next_file)
        return false;

    assert(next_file->file == nullptr);

    if(!codegen.generate_headers(reloc_table, output))
        return false;

    // TODO improve relocation so its done in steps i.e.
    //   first relocate main segment
    //   then relocate each mission
    //   ...
    //   on each step discard the registered fixups in the reloc table.
    //   this will save memory.
    //   needs to improve the Relocator interface for this.
    gta3sc::codegen::Relocator relocator(output);
    if(!relocator.relocate(reloc_table, *diag_manager))
        return false;

    return true;
}

bool Compilation::compile()
{
    auto parser_ir = parse();
    if(!parser_ir)
        return false;

    parser_ir = lower(std::move(*parser_ir));
    if(!parser_ir)
        return false;

    auto sema_ir = sema(std::move(*parser_ir));
    if(!sema_ir)
        return false;

    // No longer need the parser IR allocated data since we dropped the
    // parser IR into the sema scope in the previous step.
    parser_ir_arena.reset();

    if(!codegen(*sema_ir))
        return false;

    return true;
}
} // namespace gta3sc::driver

// TODO
//   - lower MISSION_START/END
//   - sema handle filename
//   - parser handle gosub_file etc in parse stmt list

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
            std::fprintf(stderr, "error: compilation failed 1\n");
            return 1;
        }
    }

    ////////////// COMPILATION START //////////////

    gta3sc::driver::Compilation compilation(
            input_file, command_table, model_table, file_manager, diag_manager);
    if(!compilation.compile())
    {
        std::fprintf(stderr, "error: compilation failed 2\n");
        return 1;
    }
    auto output = std::move(compilation.output); // TODO output inteface

    ////////////// COMPILATION END //////////////

    auto output_file = input_file;
    output_file.replace_extension(".cs");

    FILE* fout = std::fopen(output_file.c_str(), "wb");
    if(!fout)
    {
        // TODO diag
        std::fprintf(stderr, "error: compilation failed 3\n");
        return 1;
    }
    if(std::fwrite(output.data(), 1, output.size(), fout) != output.size())
    {
        // TODO diag
        std::fprintf(stderr, "error: compilation failed 4\n");
        return 1;
    }
    std::fclose(fout);

    std::printf("SUCCESS!\n");
    return 0;
}

// TODO improve diag to not depend on modifying a single header that ends up
// recompiling everything (or something like that)

// TODO is it a problem if we have MAIN.sc as the main file and then
// MAIN/MAIN.sc as another file?

// TODO rename Multifile classes to Multiscript?

#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/relocator.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <gta3sc/command-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/sema.hpp>
#include <gta3sc/util/arena.hpp>

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
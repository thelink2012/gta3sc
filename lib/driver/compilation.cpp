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
    return sema.validate();
}

auto Compilation::codegen(LinkedIR<SemaIR> input_ir, Result result) -> bool
{
    assert(result.target_main_scm != nullptr);
    auto& output_main_scm = *result.target_main_scm;

    const auto storage_options = codegen::StorageTable::Options{};
    auto storage_table = codegen::StorageTable::from_symbols(
            symbol_table, storage_options, *diag_manager);
    if(!storage_table)
        return false;

    codegen::RelocationTable reloc_table(symbol_table);

    codegen::trilogy::MultifileCodeGen codegen(symbol_table, *storage_table,
                                               *diag_manager);
    if(!codegen.generate(input_ir, reloc_table, output_main_scm))
        return false;

    codegen::Relocator relocator(output_main_scm);
    if(!relocator.relocate(reloc_table, *diag_manager))
        return false;

    return true;
}

bool Compilation::compile(Result result)
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
    parser_ir_arena->release();

    if(!codegen(std::move(*sema_ir), std::move(result)))
        return false;

    // No longer need the sema IR allocated data since we code generated it.
    sema_ir_arena->release();
    symbol_arena->release();
    symbol_table = SymbolTable(symbol_arena.get());

    return true;
}
} // namespace gta3sc::driver

// TODO unit test

// TODO improve relocation so its done in steps i.e.
//   first gen + relocate main segment
//   then gen + relocate each mission individually
//   ...
//   on each step discard the registered fixups in the reloc table.
//   this will save memory.
//   needs to improve the Relocator interface for this.

// TODO add AbstractCompilation -> Compilation
//                              -> DecoratedCompilation -> ...
//    maybe I'll need to think about a pipeline and subinterfaces
//    like analyzer (where I only need the IR as output, not codegen)
//    think about it further

// TODO the output interface could be improved to allow for the output
//      to be written in steps instead of a entire std::vector<std::byte>
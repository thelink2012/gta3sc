#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/relocator.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <gta3sc/command-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/syntax/lowering/if-stmt-rewriter.hpp>
#include <gta3sc/syntax/lowering/load-and-launch-mission-rewriter.hpp>
#include <gta3sc/syntax/lowering/mission-stmt-rewriter.hpp>
#include <gta3sc/syntax/lowering/repeat-stmt-rewriter.hpp>
#include <gta3sc/syntax/lowering/scope-remover.hpp>
#include <gta3sc/syntax/lowering/stats-rewriter.hpp>
#include <gta3sc/syntax/lowering/var-decl-remover.hpp>
#include <gta3sc/syntax/lowering/while-stmt-rewriter.hpp>
#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/sema.hpp>
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/name-generator.hpp>

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

auto Compilation::lower_parser(LinkedIR<ParserIR> ir) -> LinkedIR<ParserIR>
{
    using namespace gta3sc::syntax;
    auto* arena = parser_ir_arena.get();

    // Synthetic labels use a "@" prefix to avoid collisions with source
    // identifiers, which can never start with "@".
    util::NameGenerator repeat_namegen("@REPEAT_");

    CompositeInstructionRewriter rewriter(
            MissionStmtRewriter(arena),
            RepeatStmtRewriter(repeat_namegen, arena));
    rewriter.rewrite_each(ir);

    return ir;
}

auto Compilation::sema(LinkedIR<ParserIR> input_ir)
        -> std::optional<LinkedIR<SemaIR>>
{
    syntax::Sema sema(std::move(input_ir), symbol_table, *command_table,
                      *model_table, *diag_manager, sema_ir_arena.get());
    return sema.validate();
}

auto Compilation::lower_sema(LinkedIR<SemaIR> ir) -> LinkedIR<SemaIR>
{
    using namespace gta3sc::syntax;
    auto* arena = sema_ir_arena.get();
    auto& commands = *command_table;
    auto& symbols = symbol_table;

    // Synthetic labels use a "@" prefix to avoid collisions with source
    // identifiers, which can never start with "@".
    util::NameGenerator if_namegen("@IF_");
    util::NameGenerator while_namegen("@WHILE_");

    CompositeInstructionRewriter rewriter(
            IfStmtRewriter(commands, symbols, if_namegen, arena),
            WhileStmtRewriter(commands, symbols, while_namegen, arena),
            ScopeRemover(commands, arena), VarDeclRemover(commands, arena),
            LoadAndLaunchMissionRewriter(commands, arena),
            StatsRewriter::for_collectable1_total(commands, symbols, arena),
            StatsRewriter::for_progress_total(commands, symbols, arena),
            StatsRewriter::for_mission_total(commands, symbols, arena),
            StatsRewriter::for_mission_respect_total(commands, symbols, arena));
    rewriter.rewrite_each(ir);

    return ir;
}

auto Compilation::codegen(LinkedIR<SemaIR> input_ir, Result result) -> bool
{
    // TODO is it possible to run all these steps without stopping on error, and
    //      only stopping at the very end?

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

    *parser_ir = lower_parser(std::move(*parser_ir));

    auto sema_ir = sema(std::move(*parser_ir));
    if(!sema_ir)
        return false;

    // No longer need the parser IR allocated data since we dropped the
    // parser IR into the sema scope in the previous step.
    parser_ir_arena->release();

    *sema_ir = lower_sema(std::move(*sema_ir));

    if(!codegen(std::move(*sema_ir), std::move(result)))
        return false;

    // No longer need the sema IR allocated data since we code generated it.
    sema_ir_arena->release();
    symbol_arena->release();
    symbol_table = SymbolTable(symbol_arena.get());

    return true;
}
} // namespace gta3sc::driver

// TODO maybe in namegen we could use the same names as miss2?

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
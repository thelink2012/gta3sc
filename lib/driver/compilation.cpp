#include <array>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/relocator.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <gta3sc/command-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
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
namespace
{
template<typename IR, typename Rewriter>
auto apply_rewriter(LinkedIR<IR>&& ir, Rewriter& rewriter) -> LinkedIR<IR>
{
    LinkedIR<IR> result;
    auto it = ir.begin();
    while(it != ir.end())
    {
        if(auto rewrite = rewriter.visit(*it))
        {
            it = ir.erase(it);
            if(!rewrite->empty())
                result.splice_back(std::move(*rewrite));
        }
        else
        {
            auto& node = *it;
            it = ir.erase(it);
            result.push_back(node);
        }
    }
    return result;
}
} // namespace

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
    syntax::MissionStmtRewriter mission_rewriter(parser_ir_arena.get());
    ir = apply_rewriter(std::move(ir), mission_rewriter);

    util::NameGenerator namegen("LOWER_");
    syntax::RepeatStmtRewriter repeat_rewriter(namegen, parser_ir_arena.get());
    return apply_rewriter(std::move(ir), repeat_rewriter);
}

auto Compilation::sema(LinkedIR<ParserIR> input_ir)
        -> std::optional<LinkedIR<SemaIR>>
{
    syntax::Sema sema(std::move(input_ir), symbol_table, *command_table,
                      *model_table, *diag_manager, sema_ir_arena.get());
    return sema.validate();
}

auto Compilation::lower(LinkedIR<SemaIR> ir) -> std::optional<LinkedIR<SemaIR>>
{
    util::NameGenerator namegen("LOWER_");

    syntax::IfStmtRewriter if_rewriter(*command_table, symbol_table, namegen,
                                       sema_ir_arena.get());
    ir = apply_rewriter(std::move(ir), if_rewriter);

    syntax::WhileStmtRewriter while_rewriter(*command_table, symbol_table,
                                             namegen, sema_ir_arena.get());
    ir = apply_rewriter(std::move(ir), while_rewriter);

    syntax::ScopeRemover scope_rewriter(*command_table, sema_ir_arena.get());
    ir = apply_rewriter(std::move(ir), scope_rewriter);

    syntax::VarDeclRemover var_decl_rewriter(*command_table,
                                             sema_ir_arena.get());
    ir = apply_rewriter(std::move(ir), var_decl_rewriter);

    syntax::LoadAndLaunchMissionRewriter load_and_launch_rewriter(
            *command_table, sema_ir_arena.get());
    ir = apply_rewriter(std::move(ir), load_and_launch_rewriter);

    auto stats_rewriters = std::array{
            syntax::StatsRewriter::for_collectable1_total(
                    *command_table, symbol_table, sema_ir_arena.get()),
            syntax::StatsRewriter::for_progress_total(
                    *command_table, symbol_table, sema_ir_arena.get()),
            syntax::StatsRewriter::for_mission_total(
                    *command_table, symbol_table, sema_ir_arena.get()),
            syntax::StatsRewriter::for_mission_respect_total(
                    *command_table, symbol_table, sema_ir_arena.get()),
    };

    for(auto& stats_rewriter : stats_rewriters)
        ir = apply_rewriter(std::move(ir), stats_rewriter);

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

    parser_ir = lower(std::move(*parser_ir));
    if(!parser_ir)
        return false;

    auto sema_ir = sema(std::move(*parser_ir));
    if(!sema_ir)
        return false;

    // No longer need the parser IR allocated data since we dropped the
    // parser IR into the sema scope in the previous step.
    parser_ir_arena->release();

    sema_ir = lower(std::move(*sema_ir));
    if(!sema_ir)
        return false;

    if(!codegen(std::move(*sema_ir), std::move(result)))
        return false;

    // No longer need the sema IR allocated data since we code generated it.
    sema_ir_arena->release();
    symbol_arena->release();
    symbol_table = SymbolTable(symbol_arena.get());

    return true;
}
} // namespace gta3sc::driver

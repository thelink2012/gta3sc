#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/lowering/stats-rewriter.hpp>
using namespace std::literals::string_view_literals;

namespace gta3sc::syntax
{
constexpr auto command_set_collectable1_total = "SET_COLLECTABLE1_TOTAL"sv;
constexpr auto command_set_progress_total = "SET_PROGRESS_TOTAL"sv;
constexpr auto command_set_total_number_of_missions
        = "SET_TOTAL_NUMBER_OF_MISSIONS"sv;
constexpr auto command_set_mission_respect_total
        = "SET_MISSION_RESPECT_TOTAL"sv;

StatsRewriter::StatsRewriter(const CommandTable::CommandDef* command_def,
                             uint32_t total,
                             ArenaAllocator<> allocator) noexcept :
    m_command_def(command_def), m_total(total), m_allocator(allocator)
{}

StatsRewriter::StatsRewriter(const CommandTable& cmdtable,
                             std::string_view command_name, uint32_t total,
                             ArenaAllocator<> allocator) noexcept :
    StatsRewriter(cmdtable.find_command(command_name), total, allocator)
{}

auto StatsRewriter::for_collectable1_total(
        const CommandTable& cmdtable, const SymbolTable& symtable,
        ArenaAllocator<> allocator) noexcept -> StatsRewriter
{
    return StatsRewriter(cmdtable, command_set_collectable1_total,
                         symtable.collectable1_total(), allocator);
}

auto StatsRewriter::for_progress_total(
        const CommandTable& cmdtable, const SymbolTable& symtable,
        ArenaAllocator<> allocator) noexcept -> StatsRewriter
{
    return StatsRewriter(cmdtable, command_set_progress_total,
                         symtable.progress_total(), allocator);
}

auto StatsRewriter::for_mission_total(
        const CommandTable& cmdtable, const SymbolTable& symtable,
        ArenaAllocator<> allocator) noexcept -> StatsRewriter
{
    return StatsRewriter(cmdtable, command_set_total_number_of_missions,
                         symtable.mission_total(), allocator);
}

auto StatsRewriter::for_mission_respect_total(
        const CommandTable& cmdtable, const SymbolTable& symtable,
        ArenaAllocator<> allocator) noexcept -> StatsRewriter
{
    return StatsRewriter(cmdtable, command_set_mission_respect_total,
                         symtable.mission_respect_total(), allocator);
}

auto StatsRewriter::visit(const SemaIR& line) -> Result
{
    // Rewrite
    //   <initializer> N ...
    // into
    //   <initializer> total ...
    //
    // where `total` is the count accumulated during semantic analysis for the
    // dependency commands that feed this initializer.

    if(m_command_def == nullptr)
        return {};

    if(!line.has_command() || &line.command().def() != m_command_def)
        return {};

    const auto& cmd = line.command();
    if(cmd.num_args() < 1 || cmd.arg(0).as_int().value_or(-1) != 0)
        return {};

    auto builder = std::move(SemaIR::Builder(m_allocator)
                                     .label(line.label_or_null())
                                     .command(cmd.def(), cmd.source())
                                     .not_flag(cmd.not_flag())
                                     .with_num_args(cmd.num_args())
                                     .arg_int(m_total, cmd.arg(0).source()));
    for(size_t i = 1; i < cmd.num_args(); ++i)
        builder = std::move(builder).arg(&cmd.arg(i));

    return LinkedIR<SemaIR>{std::move(builder).build()};
}
} // namespace gta3sc::syntax

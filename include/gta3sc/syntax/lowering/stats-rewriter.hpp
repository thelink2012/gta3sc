#pragma once
#include <gta3sc/command-table.hpp>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/util/arena.hpp>
#include <string_view>

namespace gta3sc::syntax
{
/// Rewrites a stats initializer command: if the line matches the command,
/// replaces its first argument with `total`.
class StatsRewriter : public InstructionRewriter<SemaIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    /// Creates a rewriter that identifies the command `command_def` and
    /// replaces its first argument with `total`.
    ///
    /// If `command_def` is `nullptr`, the rewriter does nothing.
    explicit StatsRewriter(const CommandTable::CommandDef* command_def,
                           uint32_t total, ArenaAllocator<> allocator) noexcept;

    /// Creates a rewriter that identifies the command named `command_name`
    /// and replaces its first argument with `total`.
    ///
    /// If the command table does not have the command, the rewriter does
    /// nothing.
    explicit StatsRewriter(const CommandTable& cmdtable,
                           std::string_view command_name, uint32_t total,
                           ArenaAllocator<> allocator) noexcept;

    StatsRewriter(const StatsRewriter&) = delete;
    auto operator=(const StatsRewriter&) -> StatsRewriter& = delete;

    StatsRewriter(StatsRewriter&&) noexcept = default;
    auto operator=(StatsRewriter&&) noexcept -> StatsRewriter& = default;

    ~StatsRewriter() noexcept override = default;

    auto visit(const SemaIR& line) -> Result final;

    /// Rewriter for `SET_COLLECTABLE1_TOTAL` using
    /// `symtable.collectable1_total()`.
    static auto for_collectable1_total(
            const CommandTable& cmdtable, const SymbolTable& symtable,
            ArenaAllocator<> allocator) noexcept -> StatsRewriter;

    /// Rewriter for `SET_PROGRESS_TOTAL` using `symtable.progress_total()`.
    static auto
    for_progress_total(const CommandTable& cmdtable,
                       const SymbolTable& symtable,
                       ArenaAllocator<> allocator) noexcept -> StatsRewriter;

    /// Rewriter for `SET_TOTAL_NUMBER_OF_MISSIONS` using
    /// `symtable.mission_total()`.
    static auto
    for_mission_total(const CommandTable& cmdtable, const SymbolTable& symtable,
                      ArenaAllocator<> allocator) noexcept -> StatsRewriter;

    /// Rewriter for `SET_MISSION_RESPECT_TOTAL` using
    /// `symtable.mission_respect_total()`.
    static auto for_mission_respect_total(
            const CommandTable& cmdtable, const SymbolTable& symtable,
            ArenaAllocator<> allocator) noexcept -> StatsRewriter;

private:
    const CommandTable::CommandDef* m_command_def;
    uint32_t m_total;
    ArenaAllocator<> m_allocator;
};
} // namespace gta3sc::syntax

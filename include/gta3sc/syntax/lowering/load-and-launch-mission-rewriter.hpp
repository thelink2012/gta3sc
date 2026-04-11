#pragma once
#include <gta3sc/command-table.hpp>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/util/arena.hpp>

namespace gta3sc::syntax
{
/// Lowers LOAD_AND_LAUNCH_MISSION filename.sc to
/// LOAD_AND_LAUNCH_MISSION_INTERNAL mission_id.
///
/// The command table must define both `LOAD_AND_LAUNCH_MISSION` and
/// `LOAD_AND_LAUNCH_MISSION_INTERNAL` asserted at construction time.
class LoadAndLaunchMissionRewriter final : public InstructionRewriter<SemaIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    explicit LoadAndLaunchMissionRewriter(const CommandTable& cmdtable,
                                          ArenaAllocator<> allocator) noexcept;

    LoadAndLaunchMissionRewriter(const LoadAndLaunchMissionRewriter&) = delete;
    auto operator=(const LoadAndLaunchMissionRewriter&)
            -> LoadAndLaunchMissionRewriter& = delete;

    LoadAndLaunchMissionRewriter(LoadAndLaunchMissionRewriter&&) noexcept
            = default;
    auto operator=(LoadAndLaunchMissionRewriter&&) noexcept
            -> LoadAndLaunchMissionRewriter& = default;

    ~LoadAndLaunchMissionRewriter() noexcept final = default;

    auto visit(const SemaIR& line) -> Result final;

private:
    const CommandTable::CommandDef* load_and_launch_def{};
    const CommandTable::CommandDef* load_and_launch_internal_def{};
    ArenaAllocator<> allocator;
};
} // namespace gta3sc::syntax

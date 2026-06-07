#pragma once
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/util/arena.hpp>

namespace gta3sc::syntax
{
/// A rewriter that transforms MISSION_START and MISSION_END.
///
/// MISSION_START is removed.
/// MISSION_END is replaced with TERMINATE_THIS_SCRIPT.
class MissionStmtRewriter final : public InstructionRewriter<ParserIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    explicit MissionStmtRewriter(ArenaAllocator<> allocator) noexcept;

    MissionStmtRewriter(const MissionStmtRewriter&) = delete;
    auto operator=(const MissionStmtRewriter&) -> MissionStmtRewriter& = delete;

    MissionStmtRewriter(MissionStmtRewriter&&) noexcept = default;
    auto
    operator=(MissionStmtRewriter&&) noexcept -> MissionStmtRewriter& = default;

    ~MissionStmtRewriter() noexcept final = default;

    auto visit(const ParserIR& line) -> Result final;

private:
    auto visit_mission_start(const IRType& line) -> Result;
    auto visit_mission_end(const IRType& line) -> Result;

    ArenaAllocator<> allocator;
};
} // namespace gta3sc::syntax

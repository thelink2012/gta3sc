#include <gta3sc/syntax/lowering/mission-stmt-rewriter.hpp>
using namespace std::literals::string_view_literals;

namespace gta3sc::syntax
{
constexpr auto command_mission_start = "MISSION_START"sv;
constexpr auto command_mission_end = "MISSION_END"sv;
constexpr auto command_terminate_this_script = "TERMINATE_THIS_SCRIPT"sv;

MissionStmtRewriter::MissionStmtRewriter(ArenaAllocator<> allocator) noexcept :
    allocator(allocator)
{}

auto MissionStmtRewriter::visit(const ParserIR& line) -> Result
{
    if(line.has_command())
    {
        const auto name = line.command().name();
        if(name == command_mission_start)
            return visit_mission_start(line);
        if(name == command_mission_end)
            return visit_mission_end(line);
    }
    return Result{};
}

auto MissionStmtRewriter::visit_mission_start(const IRType& line) -> Result
{
    // Rewrite
    //   [label:] MISSION_START
    // into
    //   [label:]
    // or remove the line entirely when there is no label.

    if(const auto label = line.label_or_null())
    {
        return LinkedIR<ParserIR>(
                {ParserIR::create(label, nullptr, allocator)});
    }

    return LinkedIR<ParserIR>{};
}

auto MissionStmtRewriter::visit_mission_end(const IRType& line) -> Result
{
    // Rewrite
    //   [label:] MISSION_END
    // into
    //   [label:] TERMINATE_THIS_SCRIPT

    const ParserIR::Command& mission_end = line.command();

    return LinkedIR<ParserIR>({ParserIR::Builder(allocator)
                                       .label(line.label_or_null())
                                       .command(command_terminate_this_script,
                                                mission_end.source())
                                       .build()});
}
} // namespace gta3sc::syntax

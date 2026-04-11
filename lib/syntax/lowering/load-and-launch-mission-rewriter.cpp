#include <cassert>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/lowering/load-and-launch-mission-rewriter.hpp>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto command_load_and_launch_mission = "LOAD_AND_LAUNCH_MISSION"sv;
constexpr auto command_load_and_launch_mission_internal
        = "LOAD_AND_LAUNCH_MISSION_INTERNAL"sv;
} // namespace

namespace gta3sc::syntax
{
LoadAndLaunchMissionRewriter::LoadAndLaunchMissionRewriter(
        const CommandTable& cmdtable, ArenaAllocator<> allocator) noexcept :
    allocator(allocator)
{
    load_and_launch_def = cmdtable.find_command(
            command_load_and_launch_mission);
    load_and_launch_internal_def = cmdtable.find_command(
            command_load_and_launch_mission_internal);
    assert(load_and_launch_def != nullptr
           && load_and_launch_internal_def != nullptr);
}

auto LoadAndLaunchMissionRewriter::visit(const SemaIR& line) -> Result
{
    if(!line.has_command() || &line.command().def() != load_and_launch_def)
        return {};

    // Rewrite
    //   LOAD_AND_LAUNCH_MISSION <filename>
    // into
    //   LOAD_AND_LAUNCH_MISSION_INTERNAL <mission_id>

    const auto& cmd = line.command();
    const auto file = cmd.num_args() < 1 ? nullptr : cmd.arg(0).as_filename();
    if(file == nullptr || file->type() != SymbolTable::FileType::mission)
        return {};

    const auto mission_id = static_cast<int32_t>(file->type_id());

    return LinkedIR<SemaIR>{
            SemaIR::Builder(allocator)
                    .label(line.label_or_null())
                    .command(*load_and_launch_internal_def, cmd.source())
                    .arg_int(mission_id, cmd.arg(0).source())
                    .build()};
}
} // namespace gta3sc::syntax

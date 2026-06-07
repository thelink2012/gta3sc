#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/syntax/lowering/instruction-remover.hpp>
#include <ranges>

namespace gta3sc::syntax
{
InstructionRemover::InstructionRemover(ArenaAllocator<> allocator) noexcept :
    allocator(allocator)
{}

void InstructionRemover::add_command(const CommandTable::CommandDef* def)
{
    if(def != nullptr)
        removal_set.push_back(def);
}

auto InstructionRemover::visit(const SemaIR& line) -> Result
{
    // Rewrite
    //   COMMAND_IN_REMOVAL_SET ...
    // into
    //   (nothing, or label-only node if the line had a user label)
    if(!line.has_command())
        return {};

    if(!std::ranges::contains(removal_set, &line.command().def()))
        return {};

    if(line.has_label())
    {
        return LinkedIR<SemaIR>({SemaIR::create(line.label_or_null(),
                                                nullptr, allocator)});
    }

    return LinkedIR<SemaIR>{};
}
} // namespace gta3sc::syntax
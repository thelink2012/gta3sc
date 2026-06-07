#include <gta3sc/syntax/lowering/scope-remover.hpp>

namespace gta3sc::syntax
{
ScopeRemover::ScopeRemover(const CommandTable& cmdtable,
                           ArenaAllocator<> allocator) noexcept :
    InstructionRemover(allocator)
{
    add_command(cmdtable.find_command("{"));
    add_command(cmdtable.find_command("}"));
}
} // namespace gta3sc::syntax

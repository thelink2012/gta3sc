#include <gta3sc/syntax/lowering/var-decl-remover.hpp>

namespace gta3sc::syntax
{
VarDeclRemover::VarDeclRemover(const CommandTable& cmdtable,
                               ArenaAllocator<> allocator) noexcept :
    InstructionRemover(allocator)
{
    add_command(cmdtable.find_command("VAR_INT"));
    add_command(cmdtable.find_command("LVAR_INT"));
    add_command(cmdtable.find_command("VAR_FLOAT"));
    add_command(cmdtable.find_command("LVAR_FLOAT"));
    add_command(cmdtable.find_command("VAR_TEXT_LABEL"));
    add_command(cmdtable.find_command("LVAR_TEXT_LABEL"));
}
} // namespace gta3sc::syntax

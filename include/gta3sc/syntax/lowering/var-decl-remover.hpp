#pragma once
#include <gta3sc/syntax/lowering/instruction-remover.hpp>

namespace gta3sc::syntax
{
/// Rewriter that removes variable declarations (e.g. VAR_INT) from the IR.
class VarDeclRemover final : public InstructionRemover
{
public:
    explicit VarDeclRemover(const CommandTable& cmdtable,
                            ArenaAllocator<> allocator) noexcept;

    VarDeclRemover(const VarDeclRemover&) = delete;
    auto operator=(const VarDeclRemover&) -> VarDeclRemover& = delete;

    VarDeclRemover(VarDeclRemover&&) noexcept = default;
    auto operator=(VarDeclRemover&&) noexcept -> VarDeclRemover& = default;

    ~VarDeclRemover() noexcept final = default;
};
} // namespace gta3sc::syntax

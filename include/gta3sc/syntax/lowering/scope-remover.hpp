#pragma once
#include <gta3sc/syntax/lowering/instruction-remover.hpp>

namespace gta3sc::syntax
{
/// Rewriter that removes scope blocks (i.e. `{` and `}`) from the IR.
class ScopeRemover final : public InstructionRemover
{
public:
    explicit ScopeRemover(const CommandTable& cmdtable,
                          ArenaAllocator<> allocator) noexcept;

    ScopeRemover(const ScopeRemover&) = delete;
    auto operator=(const ScopeRemover&) -> ScopeRemover& = delete;

    ScopeRemover(ScopeRemover&&) noexcept = default;
    auto operator=(ScopeRemover&&) noexcept -> ScopeRemover& = default;

    ~ScopeRemover() noexcept final = default;
};
} // namespace gta3sc::syntax

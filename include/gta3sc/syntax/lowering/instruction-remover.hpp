#pragma once
#include <gta3sc/command-table.hpp>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/util/arena.hpp>
#include <vector>

namespace gta3sc::syntax
{
/// Base for rewriters that unconditionally remove a fixed set of commands.
///
/// Subclasses call `add_command(def)` in their constructor for each command
/// to remove. `visit()` is final and handles all logic.
///
/// \example
/// ```cpp
/// class ScopeRemover final : public InstructionRemover
/// {
/// public:
///     explicit ScopeRemover(const CommandTable& cmdtable,
///                           ArenaAllocator<> allocator) noexcept :
///         InstructionRemover(allocator)
///     {
///         add_command(cmdtable.find_command("{"));
///         add_command(cmdtable.find_command("}"));
///     }
/// };
/// ```
class InstructionRemover : public InstructionRewriter<SemaIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    InstructionRemover(const InstructionRemover&) = delete;
    auto operator=(const InstructionRemover&) -> InstructionRemover& = delete;

    InstructionRemover(InstructionRemover&&) noexcept = default;
    auto
    operator=(InstructionRemover&&) noexcept -> InstructionRemover& = default;

    ~InstructionRemover() noexcept override = default;

    auto visit(const SemaIR& line) -> Result final;

protected:
    explicit InstructionRemover(ArenaAllocator<> allocator) noexcept;

    /// Adds a command to the removal set unless it is `nullptr`.
    void add_command(const CommandTable::CommandDef* def);

private:
    ArenaAllocator<> allocator;
    std::vector<const CommandTable::CommandDef*> removal_set;
};
} // namespace gta3sc::syntax

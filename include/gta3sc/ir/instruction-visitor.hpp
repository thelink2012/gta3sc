#pragma once
#include <gta3sc/ir/linked-ir.hpp>

namespace gta3sc
{
/// Base class for instruction visitors.
///
/// An instruction visitor is a class that visits IR instructions
/// for a particular purpose such as finding required files.
template<typename IR, typename TResult = void>
class InstructionVisitor
{
public:
    using IRType = IR;
    using Result = TResult;

public:
    InstructionVisitor() noexcept = default;
    virtual ~InstructionVisitor() noexcept = default;

    InstructionVisitor(const InstructionVisitor&) noexcept = default;
    auto operator=(const InstructionVisitor&) noexcept
            -> InstructionVisitor& = default;

    InstructionVisitor(InstructionVisitor&&) noexcept = default;
    auto
    operator=(InstructionVisitor&&) noexcept -> InstructionVisitor& = default;

    virtual auto visit(const IRType&) -> Result = 0;

    /// Calls \ref visit for each instruction in the given IR.
    void visit_each(const LinkedIR<IRType>&);
};

template<typename IR, typename TResult>
inline void InstructionVisitor<IR, TResult>::visit_each(const LinkedIR<IR>& ir)
{
    for(const auto& line : ir)
        visit(line);
}
} // namespace gta3sc

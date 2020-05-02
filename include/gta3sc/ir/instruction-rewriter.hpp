#pragma once
#include <gta3sc/ir/instruction-visitor.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <optional>

namespace gta3sc
{
// Base class for instruction rewriters.
//
// An instruction rewriter visits instructions and possibly produces a rewrite
// of said instruction as another chain of instructions.
template<typename IRType>
class InstructionRewriter
    : public InstructionVisitor<IRType, std::optional<LinkedIR<IRType>>>
{
};
} // namespace gta3sc

#pragma once
#include <gta3sc/ir/instruction-visitor.hpp>
#include <gta3sc/ir/linked-ir.hpp>

namespace gta3sc
{
/// The rewrite specification of an instruction rewriter.
///
/// See `InstructionRewriter` for details.
template<typename IR>
struct InstructionRewriterResult
{
    /// Whether the input instruction should be replaced by `ir`.
    bool should_rewrite = false;
    /// Chain of instructions to replace the input instruction.
    LinkedIR<IR> ir;

    InstructionRewriterResult() = default;

    InstructionRewriterResult(LinkedIR<IR> ir) :
        should_rewrite(true), ir(std::move(ir))
    {}

    explicit operator bool() const { return should_rewrite; }
};

// Base class for instruction rewriters.
//
// An instruction rewriter visits instructions and possibly produces a rewrite
// specification of said instruction as another chain of instructions.
template<typename IR>
class InstructionRewriter
    : public InstructionVisitor<IR, InstructionRewriterResult<IR>>
{
};
}

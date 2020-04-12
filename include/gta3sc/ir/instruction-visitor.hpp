#pragma once

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
    virtual ~InstructionVisitor() {}
    virtual auto visit(const IRType&) -> Result = 0;
};
}

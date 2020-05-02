#pragma once
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/util/arena-allocator.hpp>
#include <gta3sc/util/name-generator.hpp>
#include <string>
#include <vector>

namespace gta3sc::syntax
{
/// A rewriter that transforms REPEAT statements.
///
/// This rewriter transforms `REPEAT...ENDREPEAT` commands into commands of
/// lower level (i.e. GOTOs and comparisions).
class RepeatStmtRewriter final : public InstructionRewriter<ParserIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    /// Constructs the rewriter.
    ///
    /// The rewriter will use the given name generator to produce
    /// names for any necessary labels.
    ///
    /// The given arena will be used to allocate the IR of replacement
    /// commands.
    explicit RepeatStmtRewriter(gta3sc::util::NameGenerator& namegen,
                                ArenaMemoryResource& arena) noexcept;

    RepeatStmtRewriter(const RepeatStmtRewriter&) = delete;
    RepeatStmtRewriter& operator=(const RepeatStmtRewriter&) = delete;

    RepeatStmtRewriter(RepeatStmtRewriter&&) noexcept = default;
    RepeatStmtRewriter& operator=(RepeatStmtRewriter&&) noexcept = default;

    ~RepeatStmtRewriter() noexcept final = default;

    auto visit(const ParserIR& line) -> Result final;

private:
    struct RepeatStmt
    {
        const ParserIR::Argument* num_times;
        const ParserIR::Argument* iter_var;
        const ParserIR::LabelDef* loop_label;
    };

    auto visit_repeat(const IRType& line) -> Result;
    auto visit_endrepeat(const IRType& line) -> Result;
    auto generate_loop_label(SourceRange source) -> const ParserIR::LabelDef*;

private:
    ArenaMemoryResource* arena;
    gta3sc::util::NameGenerator* namegen;
    std::string namegen_buffer;
    std::vector<RepeatStmt> repeat_stack;
};
} // namespace gta3sc::syntax

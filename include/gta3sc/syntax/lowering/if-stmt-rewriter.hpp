#pragma once
#include <gta3sc/command-table.hpp>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/name-generator.hpp>
#include <string>
#include <utility>
#include <vector>

namespace gta3sc::syntax
{
/// A rewriter that transforms IF/IFNOT structured statements.
///
/// Lowers IF/IFNOT...ELSE...ENDIF blocks into flat ANDOR + GOTOs.
///
/// Expects the IR to be semantically correct, otherwise behavior is undefined.
class IfStmtRewriter final : public InstructionRewriter<SemaIR>
{
public:
    using typename InstructionRewriter::IRType;
    using typename InstructionRewriter::Result;

    /// Constructs the rewriter.
    ///
    /// The rewriter will use the given name generator and symbol table
    /// to produce synthetic names for any necessary labels.
    ///
    /// The given arena will be used to allocate the IR of replacement
    /// commands.
    explicit IfStmtRewriter(const CommandTable& cmdtable, SymbolTable& symtable,
                            util::NameGenerator& namegen,
                            ArenaAllocator<> allocator) noexcept;

    IfStmtRewriter(const IfStmtRewriter&) = delete;
    auto operator=(const IfStmtRewriter&) -> IfStmtRewriter& = delete;

    IfStmtRewriter(IfStmtRewriter&&) noexcept = default;
    auto operator=(IfStmtRewriter&&) noexcept -> IfStmtRewriter& = default;

    ~IfStmtRewriter() noexcept final = default;

    auto visit(const SemaIR& line) -> Result final;

private:
    /// State machine for iterating on the IF..ELSE..ENDIF blocks.
    enum class IfStmtState : uint8_t
    {
        /// Iterating on the conditions between IF...GOTO_IF_FALSE.
        CONDS,
        /// Finished conditions and must generate a GOTO_IF_FALSE.
        BRANCH,
        /// Iterating on the statements of the THEN block.
        THEN,
        //// Iterating on the statements of the ELSE block.
        ELSE,
    };

    /// Per-block state stored on the nesting stack.
    struct IfStmt
    {
        /// Lazily generated label for the ELSE target.
        const SymbolTable::Label* else_label;
        /// Target of control-flow exits of this IF block.
        const SymbolTable::Label* endif_label;
        /// True when this block was introduced by IFNOT.
        bool is_ifnot;
        /// Current rewrite step for this IF block.
        IfStmtState state;
        /// Remaining condition lines to consume before branching.
        int8_t conds_remaining;
    };

    auto visit(const IRType& line, IfStmt& stmt) -> Result;
    auto visit_if(const IRType& line, bool is_ifnot) -> Result;
    auto visit_conds(const IRType& line, IfStmt& stmt) -> Result;
    auto visit_branch(const IRType& line, IfStmt& stmt) -> Result;
    auto visit_then(const IRType& line, IfStmt& stmt) -> Result;
    auto visit_else(const IRType& line, IfStmt& stmt) -> Result;

    auto emit_then_else_split(const IRType& else_line,
                              const IfStmt& stmt) -> LinkedIR<SemaIR>;
    auto emit_branch_to_else(FileRange source,
                             const IfStmt& stmt) -> LinkedIR<SemaIR>;

    static auto
    andor_from_if(const SemaIR::Command& cmd) -> std::pair<int8_t, FileRange>;
    static auto andor_num_conds(int8_t andor_count) -> int8_t;

    auto generate_label(FileRange source) -> const SymbolTable::Label*;

    // Cached command definitions.
    const CommandTable::CommandDef* if_def{};
    const CommandTable::CommandDef* ifnot_def{};
    const CommandTable::CommandDef* else_def{};
    const CommandTable::CommandDef* endif_def{};
    const CommandTable::CommandDef* andor_def{};
    const CommandTable::CommandDef* goto_def{};
    const CommandTable::CommandDef* goto_if_false_def{};
    const CommandTable::CommandDef* goto_if_true_def{};

private:
    ArenaAllocator<> allocator;
    SymbolTable* symtable;
    util::NameGenerator* namegen;
    std::string namegen_buffer;
    std::vector<IfStmt> if_stack;
};
} // namespace gta3sc::syntax

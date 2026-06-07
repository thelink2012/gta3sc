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
/// A rewriter that transforms WHILE/WHILENOT structured statements.
///
/// Lowers WHILE/WHILENOT...ENDWHILE blocks into flat ANDOR + GOTOs.
///
/// Expects the IR to be semantically correct, otherwise behavior is undefined.
class WhileStmtRewriter final : public InstructionRewriter<SemaIR>
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
    explicit WhileStmtRewriter(const CommandTable& cmdtable,
                               SymbolTable& symtable,
                               util::NameGenerator& namegen,
                               ArenaAllocator<> allocator) noexcept;

    WhileStmtRewriter(const WhileStmtRewriter&) = delete;
    auto operator=(const WhileStmtRewriter&) -> WhileStmtRewriter& = delete;

    WhileStmtRewriter(WhileStmtRewriter&&) noexcept = default;
    auto
    operator=(WhileStmtRewriter&&) noexcept -> WhileStmtRewriter& = default;

    ~WhileStmtRewriter() noexcept final = default;

    auto visit(const SemaIR& line) -> Result final;

private:
    /// State machine for iterating on the WHILE..ENDWHILE blocks.
    enum class WhileStmtState : uint8_t
    {
        /// Iterating on the conditions between WHILE...GOTO_IF_FALSE.
        CONDS,
        /// Finished conditions and must generate a GOTO_IF_FALSE.
        BRANCH,
        /// Iterating on the statements of the loop body.
        BODY,
    };

    /// Per-block state stored on the nesting stack.
    struct WhileStmt
    {
        /// Target of the loop head (GOTO back target).
        const SymbolTable::Label* while_label;
        /// Target of control-flow exits of this WHILE block.
        const SymbolTable::Label* endwhile_label;
        /// True when this block was introduced by WHILENOT.
        bool is_whilenot;
        /// Current rewrite step for this WHILE block.
        WhileStmtState state;
        /// Remaining condition lines to consume before branching.
        int8_t conds_remaining;
    };

    auto visit(const IRType& line, WhileStmt& stmt) -> Result;
    auto visit_while(const IRType& line, bool is_whilenot) -> Result;
    auto visit_conds(const IRType& line, WhileStmt& stmt) -> Result;
    auto visit_branch(const IRType& line, WhileStmt& stmt) -> Result;
    auto visit_body(const IRType& line, WhileStmt& stmt) -> Result;

    auto emit_loop_conditional_exit(FileRange source,
                                    const WhileStmt& stmt) -> LinkedIR<SemaIR>;
    auto emit_loop_tail(const IRType& endwhile_line,
                        const WhileStmt& stmt) -> LinkedIR<SemaIR>;

    static auto andor_from_while(const SemaIR::Command& cmd)
            -> std::pair<int8_t, FileRange>;
    static auto andor_num_conds(int8_t andor_count) -> int8_t;

    auto generate_label(FileRange source) -> const SymbolTable::Label*;

    // Cached command definitions.
    const CommandTable::CommandDef* while_def{};
    const CommandTable::CommandDef* whilenot_def{};
    const CommandTable::CommandDef* endwhile_def{};
    const CommandTable::CommandDef* andor_def{};
    const CommandTable::CommandDef* goto_def{};
    const CommandTable::CommandDef* goto_if_false_def{};
    const CommandTable::CommandDef* goto_if_true_def{};

private:
    ArenaAllocator<> allocator;
    SymbolTable* symtable;
    util::NameGenerator* namegen;
    std::string namegen_buffer;
    std::vector<WhileStmt> while_stack;
};
} // namespace gta3sc::syntax

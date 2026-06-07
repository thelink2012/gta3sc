#include <cassert>
#include <gta3sc/syntax/lowering/while-stmt-rewriter.hpp>
#include <limits>

namespace gta3sc::syntax
{
namespace
{
constexpr size_t default_stack_capacity = 8;
}

WhileStmtRewriter::WhileStmtRewriter(const CommandTable& cmdtable,
                                     SymbolTable& symtable,
                                     util::NameGenerator& namegen,
                                     ArenaAllocator<> allocator) noexcept :
    allocator(allocator), symtable(&symtable), namegen(&namegen)
{
    while_def = cmdtable.find_command("WHILE");
    whilenot_def = cmdtable.find_command("WHILENOT");
    endwhile_def = cmdtable.find_command("ENDWHILE");
    andor_def = cmdtable.find_command("ANDOR");
    goto_def = cmdtable.find_command("GOTO");
    goto_if_false_def = cmdtable.find_command("GOTO_IF_FALSE");
    goto_if_true_def = cmdtable.find_command("GOTO_IF_TRUE");

    // TODO how to make this visitor work gracefully when some command is
    // missing?
    assert(while_def != nullptr);
    assert(whilenot_def != nullptr);
    assert(endwhile_def != nullptr);
    assert(andor_def != nullptr);
    assert(goto_def != nullptr);
    assert(goto_if_false_def != nullptr);
    assert(goto_if_true_def != nullptr);
}

auto WhileStmtRewriter::visit(const SemaIR& line) -> Result
{
    if(line.has_command())
    {
        if(&line.command().def() == while_def)
            return visit_while(line, false);
        else if(&line.command().def() == whilenot_def)
            return visit_while(line, true);
    }

    if(!while_stack.empty())
    {
        auto& stmt = while_stack.back();
        auto result = visit(line, stmt);

        if(stmt.state != WhileStmtState::CONDS && line.has_command()
           && &line.command().def() == endwhile_def)
        {
            while_stack.pop_back();
        }

        return result;
    }

    return {};
}

auto WhileStmtRewriter::visit(const IRType& line, WhileStmt& stmt) -> Result
{
    switch(stmt.state)
    {
        case WhileStmtState::CONDS:
            return visit_conds(line, stmt);
        case WhileStmtState::BRANCH:
            return visit_branch(line, stmt);
        case WhileStmtState::BODY:
            return visit_body(line, stmt);
    }
    assert(false);
}

auto WhileStmtRewriter::visit_while(const IRType& line,
                                    bool is_whilenot) -> Result
{
    // Rewrite
    //   WHILE andor_count
    // into
    //   while_label:
    //   ANDOR andor_count
    //
    // WHILENOT is the same, but subsequent branches use GOTO_IF_TRUE.

    const auto& cmd = line.command();
    const auto [andor_count, andor_src] = andor_from_while(cmd);
    const int8_t num_conds = andor_num_conds(andor_count);

    const auto while_label = generate_label(no_file_range);
    const auto endwhile_label = generate_label(no_file_range);

    while_stack.reserve(default_stack_capacity);
    while_stack.push_back(WhileStmt{.while_label = while_label,
                                    .endwhile_label = endwhile_label,
                                    .is_whilenot = is_whilenot,
                                    .state = WhileStmtState::CONDS,
                                    .conds_remaining = num_conds});

    return LinkedIR<SemaIR>(
            {SemaIR::Builder(allocator).label(while_label).build(),
             SemaIR::Builder(allocator)
                     .label(line.label_or_null())
                     .command(*andor_def, cmd.source())
                     .arg_int(andor_count, andor_src)
                     .build()});
}

auto WhileStmtRewriter::visit_conds(const IRType& line,
                                    WhileStmt& stmt) -> Result
{
    // Count down condition lines; the last condition leaves the
    // state machine in BRANCH without rewriting the line itself.

    if(!line.has_command())
        return {};

    assert(&line.command().def() != endwhile_def);

    stmt.conds_remaining -= 1;
    if(stmt.conds_remaining == 0)
        stmt.state = WhileStmtState::BRANCH;

    return {};
}

auto WhileStmtRewriter::visit_branch(const IRType& line,
                                     WhileStmt& stmt) -> Result
{
    // After the last condition, emit the branch for ENDWHILE.
    // WHILE uses GOTO_IF_FALSE; WHILENOT uses GOTO_IF_TRUE.

    if(line.has_command() && &line.command().def() == endwhile_def)
    {
        // Empty body: conditions are directly followed by ENDWHILE.
        //
        // Rewrite
        //   ENDWHILE
        // into
        //   GOTO_IF_FALSE endwhile_label
        //   GOTO while_label
        //   endwhile_label:

        auto result = emit_loop_conditional_exit(line.command().source(), stmt);
        result.splice_back(emit_loop_tail(line, stmt));
        return result;
    }

    // First body statement:
    //   GOTO_IF_FALSE endwhile_label
    //   <copy of original line>

    stmt.state = WhileStmtState::BODY;

    const auto src = line.has_command() ? line.command().source()
                                        : SemaIR::Builder::no_range;
    auto result = emit_loop_conditional_exit(src, stmt);
    result.splice_back(LinkedIR<SemaIR>({SemaIR::create(
            line.label_or_null(), line.command_or_null(), allocator)}));
    return result;
}

auto WhileStmtRewriter::visit_body(const IRType& line,
                                   WhileStmt& stmt) -> Result
{
    if(!line.has_command() || &line.command().def() != endwhile_def)
        return {};

    // Rewrite
    //   ENDWHILE
    // into
    //   GOTO while_label
    //   endwhile_label:

    return emit_loop_tail(line, stmt);
}

auto WhileStmtRewriter::generate_label(FileRange source)
        -> const SymbolTable::Label*
{
    namegen->generate(namegen_buffer);
    const auto [label, inserted] = symtable->insert_label(
            namegen_buffer, SymbolTable::global_scope, source);
    assert(inserted);
    return label;
}

auto WhileStmtRewriter::andor_from_while(const SemaIR::Command& cmd)
        -> std::pair<int8_t, FileRange>
{
    assert(cmd.num_args() >= 1);

    const auto source = cmd.arg(0).source();
    const auto count = cmd.arg(0).as_int();
    assert(count.has_value());
    assert(*count >= std::numeric_limits<int8_t>::min());
    assert(*count <= std::numeric_limits<int8_t>::max());

    return {static_cast<int8_t>(*count), source};
}

auto WhileStmtRewriter::andor_num_conds(int8_t andor_count) -> int8_t
{
    return static_cast<int8_t>(andor_count < 20 ? andor_count + 1
                                                : andor_count - 19);
}

auto WhileStmtRewriter::emit_loop_tail(
        const IRType& endwhile_line, const WhileStmt& stmt) -> LinkedIR<SemaIR>
{
    return LinkedIR<SemaIR>(
            {SemaIR::Builder(allocator)
                     .label(endwhile_line.label_or_null())
                     .command(*goto_def, endwhile_line.command().source())
                     .arg_label(*stmt.while_label,
                                endwhile_line.command().source())
                     .build(),
             SemaIR::Builder(allocator).label(stmt.endwhile_label).build()});
}

auto WhileStmtRewriter::emit_loop_conditional_exit(
        FileRange source, const WhileStmt& stmt) -> LinkedIR<SemaIR>
{
    const auto branch_def = stmt.is_whilenot ? goto_if_true_def
                                             : goto_if_false_def;
    return LinkedIR<SemaIR>({SemaIR::Builder(allocator)
                                     .command(*branch_def, source)
                                     .arg_label(*stmt.endwhile_label, source)
                                     .build()});
}
} // namespace gta3sc::syntax

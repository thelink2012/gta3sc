#include <cassert>
#include <gta3sc/syntax/lowering/if-stmt-rewriter.hpp>
#include <limits>

namespace gta3sc::syntax
{
namespace
{
constexpr size_t default_stack_capacity = 8;
}

IfStmtRewriter::IfStmtRewriter(const CommandTable& cmdtable,
                               SymbolTable& symtable,
                               util::NameGenerator& namegen,
                               ArenaAllocator<> allocator) noexcept :
    allocator(allocator), symtable(&symtable), namegen(&namegen)
{
    if_def = cmdtable.find_command("IF");
    ifnot_def = cmdtable.find_command("IFNOT");
    else_def = cmdtable.find_command("ELSE");
    endif_def = cmdtable.find_command("ENDIF");
    andor_def = cmdtable.find_command("ANDOR");
    goto_def = cmdtable.find_command("GOTO");
    goto_if_false_def = cmdtable.find_command("GOTO_IF_FALSE");
    goto_if_true_def = cmdtable.find_command("GOTO_IF_TRUE");

    // TODO how to make this visitor work gracefully when some command is missing?
    assert(if_def != nullptr);
    assert(ifnot_def != nullptr);
    assert(else_def != nullptr);
    assert(endif_def != nullptr);
    assert(andor_def != nullptr);
    assert(goto_def != nullptr);
    assert(goto_if_false_def != nullptr);
    assert(goto_if_true_def != nullptr);
}

auto IfStmtRewriter::visit(const SemaIR& line) -> Result
{
    if(line.has_command())
    {
        if(&line.command().def() == if_def)
            return visit_if(line, false);
        else if(&line.command().def() == ifnot_def)
            return visit_if(line, true);
    }

    if(!if_stack.empty())
    {
        auto& stmt = if_stack.back();
        auto result = visit(line, stmt);

        if(stmt.state != IfStmtState::CONDS && line.has_command()
           && &line.command().def() == endif_def)
        {
            if_stack.pop_back();
        }

        return result;
    }

    return {};
}

auto IfStmtRewriter::visit(const IRType& line, IfStmt& stmt) -> Result
{
    switch(stmt.state)
    {
        case IfStmtState::CONDS:
            return visit_conds(line, stmt);
        case IfStmtState::BRANCH:
            return visit_branch(line, stmt);
        case IfStmtState::THEN:
            return visit_then(line, stmt);
        case IfStmtState::ELSE:
            return visit_else(line, stmt);
    }
    assert(false);
}

auto IfStmtRewriter::visit_if(const IRType& line, bool is_ifnot) -> Result
{
    // Rewrite
    //   IF/IFNOT andor_count
    // into
    //   ANDOR andor_count
    //
    // IFNOT is the same, but subsequent branches use GOTO_IF_TRUE.

    const auto& cmd = line.command();
    const auto [andor_count, andor_src] = andor_from_if(cmd);
    const int8_t num_conds = andor_num_conds(andor_count);

    const auto endif_label = generate_label(no_file_range);

    if_stack.reserve(default_stack_capacity);
    if_stack.push_back(IfStmt{.else_label = nullptr,
                              .endif_label = endif_label,
                              .is_ifnot = is_ifnot,
                              .state = IfStmtState::CONDS,
                              .conds_remaining = num_conds});

    return LinkedIR<SemaIR>({SemaIR::Builder(allocator)
                                     .label(line.label_or_null())
                                     .command(*andor_def, cmd.source())
                                     .arg_int(andor_count, andor_src)
                                     .build()});
}

auto IfStmtRewriter::visit_conds(const IRType& line, IfStmt& stmt) -> Result
{
    // Count down condition lines; the last condition leaves the
    // state machine in BRANCH without rewriting the line itself.

    if(!line.has_command())
        return {};

    assert(&line.command().def() != else_def
           && &line.command().def() != endif_def);

    stmt.conds_remaining -= 1;
    if(stmt.conds_remaining == 0)
        stmt.state = IfStmtState::BRANCH;

    return {};
}

auto IfStmtRewriter::visit_branch(const IRType& line, IfStmt& stmt) -> Result
{
    // After the last condition, emit the branch for ELSE/ENDIF/THEN.
    // IF uses GOTO_IF_FALSE; IFNOT uses GOTO_IF_TRUE.

    const auto branch_def = stmt.is_ifnot ? goto_if_true_def
                                          : goto_if_false_def;

    if(line.has_command() && &line.command().def() == else_def)
    {
        // This happens when there's no statements in the THEN block, i.e. the
        // conditions are directly followed by an ELSE.
        //
        // Rewrite
        //   ELSE
        // into
        //   GOTO_IF_FALSE else_label
        //   GOTO endif_label
        //   else_label:

        if(stmt.else_label == nullptr)
            stmt.else_label = generate_label(no_file_range);

        stmt.state = IfStmtState::ELSE;

        // GOTO_IF_FALSE else_label
        auto result = emit_branch_to_else(line.command().source(), stmt);
        // GOTO endif_label
        // else_label:
        result.splice_back(emit_then_else_split(line, stmt));
        return result;
    }
    else if(line.has_command() && &line.command().def() == endif_def)
    {
        // Empty THEN-block, but no ELSE.
        //
        // Rewrite
        //   ENDIF
        // into
        //   GOTO_IF_FALSE endif_label
        //   endif_label:

        return LinkedIR<SemaIR>(
                {SemaIR::Builder(allocator)
                         .command(*branch_def, line.command().source())
                         .arg_label(*stmt.endif_label, line.command().source())
                         .build(),
                 SemaIR::Builder(allocator).label(stmt.endif_label).build()});
    }
    else
    {
        // First then-statement:
        //   GOTO_IF_FALSE else_label
        //   <copy of original line>

        if(stmt.else_label == nullptr)
            stmt.else_label = generate_label(no_file_range);

        stmt.state = IfStmtState::THEN;

        const auto src = line.has_command() ? line.command().source()
                                            : SemaIR::Builder::no_range;
        return LinkedIR<SemaIR>(
                {SemaIR::Builder(allocator)
                         .command(*branch_def, src)
                         .arg_label(*stmt.else_label, src)
                         .build(),
                 SemaIR::create(line.label_or_null(), line.command_or_null(),
                                allocator)});
    }
}

auto IfStmtRewriter::visit_then(const IRType& line, IfStmt& stmt) -> Result
{
    if(line.has_command() && &line.command().def() == else_def)
    {
        // Rewrite
        //   ELSE
        // into
        //   GOTO endif_label
        //   else_label:

        assert(stmt.else_label != nullptr);

        stmt.state = IfStmtState::ELSE;

        return emit_then_else_split(line, stmt);
    }
    else if(line.has_command() && &line.command().def() == endif_def)
    {
        // Rewrite
        //   ENDIF
        // into
        //   else_label:
        //   endif_label:

        assert(stmt.else_label != nullptr);

        return LinkedIR<SemaIR>(
                {SemaIR::Builder(allocator).label(stmt.else_label).build(),
                 SemaIR::Builder(allocator).label(stmt.endif_label).build()});
    }
    else
    {
        return {};
    }
}

auto IfStmtRewriter::visit_else(const IRType& line, IfStmt& stmt) -> Result
{
    if(!line.has_command() || &line.command().def() != endif_def)
        return {};

    // Rewrite
    //   ENDIF
    // into
    //   endif_label:

    return LinkedIR<SemaIR>(
            {SemaIR::Builder(allocator).label(stmt.endif_label).build()});
}

auto IfStmtRewriter::generate_label(FileRange source)
        -> const SymbolTable::Label*
{
    namegen->generate(namegen_buffer);
    const auto [label, inserted] = symtable->insert_label(
            namegen_buffer, SymbolTable::global_scope, source);
    assert(inserted);
    return label;
}

auto IfStmtRewriter::andor_from_if(const SemaIR::Command& cmd)
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

auto IfStmtRewriter::andor_num_conds(int8_t andor_count) -> int8_t
{
    return static_cast<int8_t>(andor_count < 20 ? andor_count + 1
                                                : andor_count - 19);
}

auto IfStmtRewriter::emit_then_else_split(
        const IRType& else_line, const IfStmt& stmt) -> LinkedIR<SemaIR>
{
    return LinkedIR<SemaIR>(
            {SemaIR::Builder(allocator)
                     .label(else_line.label_or_null())
                     .command(*goto_def, else_line.command().source())
                     .arg_label(*stmt.endif_label, else_line.command().source())
                     .build(),
             SemaIR::Builder(allocator).label(stmt.else_label).build()});
}

auto IfStmtRewriter::emit_branch_to_else(FileRange source,
                                         const IfStmt& stmt) -> LinkedIR<SemaIR>
{
    const auto branch_def = stmt.is_ifnot ? goto_if_true_def
                                          : goto_if_false_def;
    return LinkedIR<SemaIR>({SemaIR::Builder(allocator)
                                     .command(*branch_def, source)
                                     .arg_label(*stmt.else_label, source)
                                     .build()});
}
} // namespace gta3sc::syntax

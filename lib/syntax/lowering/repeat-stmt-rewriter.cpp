#include <gta3sc/syntax/lowering/repeat-stmt-rewriter.hpp>
using namespace std::literals::string_view_literals;

namespace gta3sc::syntax
{
constexpr size_t default_stack_size = 8;

constexpr auto COMMAND_REPEAT = "REPEAT"sv;
constexpr auto COMMAND_ENDREPEAT = "ENDREPEAT"sv;
constexpr auto COMMAND_GOTO_IF_FALSE = "GOTO_IF_FALSE"sv;
constexpr auto COMMAND_SET = "SET"sv;
constexpr auto COMMAND_ADD_THING_TO_THING = "ADD_THING_TO_THING"sv;
constexpr auto COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING
        = "IS_THING_GREATER_OR_EQUAL_TO_THING"sv;

RepeatStmtRewriter::RepeatStmtRewriter(gta3sc::util::NameGenerator& namegen,
                                       ArenaMemoryResource& arena) noexcept :
    namegen(&namegen), arena(&arena)
{}

auto RepeatStmtRewriter::visit(const ParserIR& line) -> Result
{
    if(line.command)
    {
        if(line.command->name == COMMAND_REPEAT)
            return visit_repeat(line);
        else if(line.command->name == COMMAND_ENDREPEAT)
            return visit_endrepeat(line);
    }
    return Result{};
}

auto RepeatStmtRewriter::visit_repeat(const ParserIR& line) -> Result
{
    // Rewrite
    //   REPEAT num_times iter_var
    // into
    //   SET iter_var 0
    //   loop_label:

    const ParserIR::Command* repeat = line.command;
    assert(repeat != nullptr);

    if(repeat->args.size() != 2)
        return Result{};

    const auto* const num_times = repeat->args[0];
    const auto* const iter_var = repeat->args[1];
    const auto* const loop_label_def = generate_loop_label(repeat->source);

    if(repeat_stack.capacity() == 0)
        repeat_stack.reserve(default_stack_size);
    repeat_stack.push_back(RepeatStmt{num_times, iter_var, loop_label_def});

    return LinkedIR<ParserIR>(
            {ParserIR::Builder(*arena)
                     .label(line.label)
                     .command(COMMAND_SET, repeat->source)
                     .arg(iter_var)
                     .arg_int(0, repeat->source)
                     .build(),

             ParserIR::create(loop_label_def, nullptr, arena)});
}

auto RepeatStmtRewriter::visit_endrepeat(const ParserIR& line) -> Result
{
    // Rewrite
    //   ENDREPEAT
    // into
    //   ADD_THING_TO_THING iter_var 1
    //   IS_THING_GREATER_OR_EQUAL_TO_THING iter_var num_times
    //   GOTO_IF_FALSE loop_label

    const ParserIR::Command* endrepeat = line.command;
    assert(endrepeat != nullptr);

    if(repeat_stack.empty())
        return Result{};

    const auto [num_times, iter_var, loop_label] = repeat_stack.back();
    repeat_stack.pop_back();

    return LinkedIR<ParserIR>(
            {ParserIR::Builder(*arena)
                     .label(line.label)
                     .command(COMMAND_ADD_THING_TO_THING, endrepeat->source)
                     .arg(iter_var)
                     .arg_int(1, endrepeat->source)
                     .build(),

             ParserIR::Builder(*arena)
                     .command(COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING,
                              endrepeat->source)
                     .arg(iter_var)
                     .arg(num_times)
                     .build(),

             ParserIR::Builder(*arena)
                     .command(COMMAND_GOTO_IF_FALSE, endrepeat->source)
                     .arg_ident(loop_label->name, endrepeat->source)
                     .build()});
}

auto RepeatStmtRewriter::generate_loop_label(SourceRange source)
        -> const ParserIR::LabelDef*
{
    namegen->generate(namegen_buffer);
    return ParserIR::LabelDef::create(namegen_buffer, source, arena);
}
} // namespace gta3sc::syntax

#include <gta3sc/syntax/lowering/repeat-stmt-rewriter.hpp>
using namespace std::literals::string_view_literals;

namespace gta3sc::syntax
{
constexpr size_t default_stack_size = 8;

constexpr auto command_repeat = "REPEAT"sv;
constexpr auto command_endrepeat = "ENDREPEAT"sv;
constexpr auto command_goto_if_false = "GOTO_IF_FALSE"sv;
constexpr auto command_set = "SET"sv;
constexpr auto command_add_thing_to_thing = "ADD_THING_TO_THING"sv;
constexpr auto command_is_thing_greater_or_equal_to_thing
        = "IS_THING_GREATER_OR_EQUAL_TO_THING"sv;

RepeatStmtRewriter::RepeatStmtRewriter(gta3sc::util::NameGenerator& namegen,
                                       ArenaMemoryResource& arena) noexcept :
    namegen(&namegen), arena(&arena)
{}

auto RepeatStmtRewriter::visit(const ParserIR& line) -> Result
{
    if(line.command)
    {
        if(line.command->name == command_repeat)
            return visit_repeat(line);
        else if(line.command->name == command_endrepeat)
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
                     .command(command_set, repeat->source)
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
                     .command(command_add_thing_to_thing, endrepeat->source)
                     .arg(iter_var)
                     .arg_int(1, endrepeat->source)
                     .build(),

             ParserIR::Builder(*arena)
                     .command(command_is_thing_greater_or_equal_to_thing,
                              endrepeat->source)
                     .arg(iter_var)
                     .arg(num_times)
                     .build(),

             ParserIR::Builder(*arena)
                     .command(command_goto_if_false, endrepeat->source)
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

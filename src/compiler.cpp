#include <stdinc.h>
#include "compiler.hpp"
#include "symtable.hpp"
#include "commands.hpp"
#include "program.hpp"

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
static ArgVariant conv_int(T integral)
{
    int32_t i = static_cast<int32_t>(integral);

    if(i >= std::numeric_limits<int8_t>::min() && i <= std::numeric_limits<int8_t>::max())
        return int8_t(i);
    else if(i >= std::numeric_limits<int16_t>::min() && i <= std::numeric_limits<int16_t>::max())
        return int16_t(i);
    else
        return int32_t(i);
}

void CompilerContext::compile()
{
    Expects(compiled.empty());
    Expects(script->top_label->code_position == nullopt);
    Expects(script->start_label->code_position == nullopt);

    program.supported_or_fatal(nocontext, commands.andor, "ANDOR");
    program.supported_or_fatal(nocontext, commands.goto_, "GOTO");
    program.supported_or_fatal(nocontext, commands.goto_if_false, "GOTO_IF_FALSE");

    compile_label(script->top_label);
    compile_label(script->start_label);
    return compile_statements(*script->tree);
}

shared_ptr<Label> CompilerContext::make_internal_label()
{
    this->internal_labels.emplace_back(std::make_shared<Label>(this->current_scope, this->script));
    return this->internal_labels.back();
}

void CompilerContext::compile_label(const SyntaxTree& label_node)
{
    return compile_label(label_node.annotation<shared_ptr<Label>>());
}

void CompilerContext::compile_label(shared_ptr<Label> label_ptr)
{
    this->compiled.emplace_back(std::move(label_ptr));
}

void CompilerContext::compile_command(const Command& command, ArgList args, bool not_flag)
{
    if(command.extension && program.opt.pedantic)
        program.pedantic(this->script, "use of command {} which is a language extension [-pedantic]", command.name);

    if(command.has_optional())
    {
        assert(args.size() == 0 || !is<EOAL>(args.back()));
        args.emplace_back(EOAL{});
    }

    this->compiled.emplace_back(CompiledCommand{ not_flag, command, std::move(args) });
}

void CompilerContext::compile_command(const SyntaxTree& command_node, bool not_flag)
{
    if(command_node.maybe_annotation<DummyCommandAnnotation>())
    {
        // compile nothing
    }
    else if(auto opt_annot = command_node.maybe_annotation<const ReplacedCommandAnnotation&>())
    {
        const Command& command = opt_annot->command;

        if(commands.equal(command, commands.skip_cutscene_start_internal))
        {
            this->label_skip_cutscene_end = any_cast<shared_ptr<Label>>(opt_annot->params[0]);
        }

        compile_command(command, get_args(opt_annot->command, opt_annot->params));
    }
    else
    {
        const Command& command = command_node.annotation<std::reference_wrapper<const Command>>();

        if(commands.equal(command, commands.skip_cutscene_end) && this->label_skip_cutscene_end)
        {
            compile_label(this->label_skip_cutscene_end);
            this->label_skip_cutscene_end = nullptr;
        }

        compile_command(command, get_args(command, command_node), not_flag);
    }
}

void CompilerContext::compile_statements(const SyntaxTree& base)
{
    for(auto it = base.begin(); it != base.end(); ++it)
        compile_statement(*it->get());
}

void CompilerContext::compile_statements(const SyntaxTree& parent, size_t from_id, size_t to_id_including)
{
    for(size_t i = from_id; i <= to_id_including; ++i)
        compile_statement(parent.child(i));
}

void CompilerContext::compile_statement(const SyntaxTree& node, bool not_flag)
{
    switch(node.type())
    {
        case NodeType::Block:
            // group of anything, usually group of statements
            compile_statements(node);
            break;
        case NodeType::NOT:
            compile_statement(node.child(0), !not_flag);
            break;
        case NodeType::Command:
            compile_command(node, not_flag);
            break;
        case NodeType::MISSION_START:
        case NodeType::SCRIPT_START:
            break;
        case NodeType::MISSION_END:
        case NodeType::SCRIPT_END:
            compile_mission_end(node, not_flag);
            break;
        case NodeType::Greater:
        case NodeType::GreaterEqual:
        case NodeType::Lesser:
        case NodeType::LesserEqual:
            compile_condition(node, not_flag);
            break;
        case NodeType::Equal:
        case NodeType::Cast:
            compile_expr(node, not_flag);
            break;
        case NodeType::Increment:
        case NodeType::Decrement:
            compile_incdec(node, not_flag);
            break;
        case NodeType::Label:
            compile_label(node);
            break;
        case NodeType::Scope:
            compile_scope(node);
            break;
        case NodeType::IF:
            compile_if(node);
            break;
        case NodeType::WHILE:
            compile_while(node);
            break;
        case NodeType::REPEAT:
            compile_repeat(node);
            break;
        case NodeType::SWITCH:
            compile_switch(node);
            break;
        case NodeType::BREAK:
            compile_break(node);
            break;
        case NodeType::CONTINUE:
            compile_continue(node);
            break;
        case NodeType::VAR_INT:
        case NodeType::LVAR_INT:
        case NodeType::VAR_FLOAT:
        case NodeType::LVAR_FLOAT:
        case NodeType::VAR_TEXT_LABEL:
        case NodeType::LVAR_TEXT_LABEL:
        case NodeType::VAR_TEXT_LABEL16:
        case NodeType::LVAR_TEXT_LABEL16:
            break;
        case NodeType::CONST_INT:
        case NodeType::CONST_FLOAT:
            break;
        case NodeType::DUMP:
            compile_dump(node);
            break;
        default:
            Unreachable();
    }
}

void CompilerContext::compile_dump(const SyntaxTree& node)
{
    this->compiled.emplace_back(node.annotation<DumpAnnotation>().bytes);
}

void CompilerContext::compile_scope(const SyntaxTree& scope_node)
{
    auto guard = make_scope_guard([this] {
        this->current_scope = nullptr;
    });

    Expects(this->current_scope == nullptr);
    this->current_scope = scope_node.annotation<shared_ptr<Scope>>();
    compile_statements(scope_node.child(0));
}

void CompilerContext::compile_if(const SyntaxTree& if_node)
{
    if(if_node.child_count() == 3) // [conds, case_true, else]
    {
        auto else_ptr = make_internal_label();
        auto end_ptr  = make_internal_label();
        compile_conditions(if_node.child(0), else_ptr);
        compile_statements(if_node.child(1));
        compile_command(*this->commands.goto_, { end_ptr });
        compile_label(else_ptr);
        compile_statements(if_node.child(2));
        compile_label(end_ptr);
    }
    else // [conds, case_true]
    {
        auto end_ptr = make_internal_label();
        compile_conditions(if_node.child(0), end_ptr);
        compile_statements(if_node.child(1));
        compile_label(end_ptr);
    }
}

void CompilerContext::compile_while(const SyntaxTree& while_node)
{
    auto beg_ptr = make_internal_label();
    auto end_ptr = make_internal_label();

    loop_stack.emplace_back(LoopInfo { beg_ptr, end_ptr });

    compile_label(beg_ptr);
    compile_conditions(while_node.child(0), end_ptr);
    compile_statements(while_node.child(1));
    compile_command(*this->commands.goto_, { beg_ptr });
    compile_label(end_ptr);

    loop_stack.pop_back();
}

void CompilerContext::compile_repeat(const SyntaxTree& repeat_node)
{
    auto& annotation = repeat_node.annotation<const RepeatAnnotation&>();
    auto& times = repeat_node.child(0);
    auto& var = repeat_node.child(1);

    auto continue_ptr = make_internal_label();
    auto break_ptr    = make_internal_label();
    auto loop_ptr     = make_internal_label();

    loop_stack.emplace_back(LoopInfo { continue_ptr, break_ptr });

    compile_command(annotation.set_var_to_zero, { get_arg(var), get_arg(annotation.number_zero) });
    compile_label(loop_ptr);
    compile_statements(repeat_node.child(2));
    compile_label(continue_ptr);
    compile_command(annotation.add_var_with_one, { get_arg(var), get_arg(annotation.number_one) });
    compile_command(annotation.is_var_geq_times, { get_arg(var), get_arg(times) });
    compile_command(*this->commands.goto_if_false, { loop_ptr });
    compile_label(break_ptr);

    loop_stack.pop_back();
}

void CompilerContext::compile_switch(const SyntaxTree& switch_node)
{
    auto continue_ptr = nullptr;
    auto break_ptr = make_internal_label();

    loop_stack.emplace_back(LoopInfo{ continue_ptr, break_ptr });

    std::vector<Case> cases;
    cases.reserve(1 + switch_node.annotation<const SwitchAnnotation&>().num_cases);

    auto& switch_body = switch_node.child(1);
    for(size_t i = 0; i < switch_body.child_count(); ++i)
    {
        auto& node = switch_body.child(i);
        switch(node.type())
        {
            case NodeType::CASE:
            {
                auto value  = node.child(0).annotation<int32_t>();
                auto& isveq = node.annotation<const SwitchCaseAnnotation&>().is_var_eq_int;
                cases.emplace_back(value, isveq);
                break;
            }

            case NodeType::DEFAULT:
            {
                cases.emplace_back(nullopt, nullopt);
                break;
            }

            default: // statement
            {
                if(!cases.empty())
                {
                    auto& prev_case = cases.back();
                    if(prev_case.first_statement_id == SIZE_MAX)
                        prev_case.first_statement_id = i;
                    prev_case.last_statement_id = i;
                }
                break;
            }
        }
    }

    auto blank_cases_begin = cases.end();
    auto blank_cases_end   = cases.end();

    for(auto it = cases.begin(); it != cases.end(); ++it)
    {
        if(it->is_empty())
        {
            if(blank_cases_begin == cases.end())
            {
                blank_cases_begin = it;
                blank_cases_end   = std::next(it);
            }
            else
            {
                blank_cases_end = std::next(it);
            }
        }
        else
        {
            if(blank_cases_begin != cases.end())
            {
                for(auto e = blank_cases_begin; e != blank_cases_end; ++e)
                {
                    e->first_statement_id = it->first_statement_id;
                    e->last_statement_id  = it->last_statement_id;
                }

                blank_cases_begin = cases.end();
                blank_cases_end = cases.end();
            }
        }
    }

    auto opt_switch_start = commands.switch_start;
    auto opt_switch_continued = commands.switch_continued;

    if(opt_switch_start && opt_switch_start->supported
    && opt_switch_continued && opt_switch_continued->supported)
    {
        compile_switch_withop(switch_node, cases, break_ptr);
    }
    else
    {
        compile_switch_ifchain(switch_node, cases, break_ptr);
    }

    loop_stack.pop_back();
}

void CompilerContext::compile_switch_withop(const SyntaxTree& swnode, std::vector<Case>& cases, shared_ptr<Label> break_ptr)
{
    std::vector<Case*> sorted_cases;   // does not contain default, unlike `cases`
    sorted_cases.resize(cases.size());
    bool has_default = swnode.annotation<const SwitchAnnotation&>().has_default;
    const Case* case_default = nullptr;

    const Command& switch_start     = this->commands.switch_start.value();
    const Command& switch_continued = this->commands.switch_continued.value();

    std::transform(cases.begin(), cases.end(), sorted_cases.begin(), [] (auto& c) { return std::addressof<Case>(c); });

    std::sort(sorted_cases.begin(), sorted_cases.end(), [](const Case* a, const Case* b) {
        if(a->is_default()) return false; // default should be the last
        if(b->is_default()) return true;  // !a->is_default() == true
        return *a->value < *b->value;
    });

    if(has_default)
    {
        case_default = sorted_cases.back();
        sorted_cases.pop_back();
        assert(case_default->is_default());
    }

    for(auto& c : cases)
        c.target = make_internal_label();

    for(size_t i = 0; i < sorted_cases.size(); )
    {
        std::vector<ArgVariant> args;
        args.reserve(18);

        const Command& switch_op = (i == 0? switch_start : switch_continued);
        size_t max_cases_here    = (i == 0? 7 : 9);

        if(i == 0)
        {
            args.emplace_back(get_arg(swnode.child(0)));
            args.emplace_back(conv_int(sorted_cases.size()));
            args.emplace_back(conv_int(has_default));
            args.emplace_back(has_default? case_default->target : break_ptr);
        }

        for(size_t k = 0; k < max_cases_here; ++k, ++i)
        {
            if(i < sorted_cases.size())
            {
                args.emplace_back(conv_int(*sorted_cases[i]->value));
                args.emplace_back(sorted_cases[i]->target);
            }
            else
            {
                args.emplace_back(conv_int(-1));
                args.emplace_back(break_ptr);
            }
        }

        compile_command(switch_op, std::move(args));
    }

    for(auto it = cases.begin(); it != cases.end(); ++it)
    {
        compile_label(it->target);
        if(std::next(it) == cases.end() || !std::next(it)->same_body_as(*it))
        {
            compile_statements(swnode.child(1), it->first_statement_id, it->last_statement_id);
        }
    }

    compile_label(break_ptr);
}

void CompilerContext::compile_switch_ifchain(const SyntaxTree& swnode, std::vector<Case>& cases, shared_ptr<Label> break_ptr)
{
    Case* default_case = nullptr;

    for(auto it = cases.begin(), next_it = cases.end(); it != cases.end(); it = next_it)
    {
        auto next_ptr = make_internal_label();
        auto body_ptr = make_internal_label();

        next_it = std::find_if(std::next(it), cases.end(), [&](const Case& c){
            return !c.same_body_as(*it);
        });

        auto num_ifs = std::accumulate(it, next_it, size_t(0), [](size_t accum, const Case& c) {
            return accum + (c.is_default()? 0 : 1);
        });
        if(num_ifs > 8)
            program.error(swnode, "more than 8 cases with the same body not supported using if-chain");
        else if(num_ifs > 1)
            compile_command(*commands.andor, { conv_int(21 + num_ifs - 2) });
        else if(num_ifs == 0 && it->is_default())
        {
            default_case = std::addressof(*it);
            continue;
        }

        for(auto k = it; k != next_it; ++k)
        {
            if(!k->is_default())
            {
                if(k->is_var_eq_int == nullopt)
                    program.fatal_error(nocontext, "unexpected failure at {}", __func__);

                compile_command(**k->is_var_eq_int, { get_arg(swnode.child(0)), conv_int(*k->value) });
            }
            else
                default_case = std::addressof(*k);
        }
        if(num_ifs) compile_command(*commands.goto_if_false, { next_ptr });

        compile_label(body_ptr);
        std::for_each(it, next_it, [&](Case& c) { c.target = body_ptr; });
        compile_statements(swnode.child(1), it->first_statement_id, it->last_statement_id);
        compile_label(next_ptr);
    }

    if(default_case)
    {
        if(default_case->target)
        {
            compile_command(*commands.goto_, { default_case->target });
        }
        else
        {
            default_case->target = make_internal_label();
            compile_label(default_case->target);
            compile_statements(swnode.child(1), default_case->first_statement_id, default_case->last_statement_id);
        }
    }

    compile_label(break_ptr);
}

void CompilerContext::compile_break(const SyntaxTree& break_node)
{
    for(auto it = loop_stack.rbegin(); it != loop_stack.rend(); ++it)
    {
        if(it->break_label)
        {
            compile_command(*commands.goto_, { it->break_label });
            return;
        }
    }
    Unreachable();
}

void CompilerContext::compile_continue(const SyntaxTree& continue_node)
{
    for(auto it = loop_stack.rbegin(); it != loop_stack.rend(); ++it)
    {
        if(it->continue_label)
        {
            compile_command(*commands.goto_, { it->continue_label });
            return;
        }
    }
    Unreachable();
}

void CompilerContext::compile_expr(const SyntaxTree& eq_node, bool not_flag)
{
    if(eq_node.child(1).maybe_annotation<std::reference_wrapper<const Command>>())
    {
        // 'a = b OP c' or 'a OP= b'

        const SyntaxTree& op_node = eq_node.child(1);

        const Command& cmd_set = eq_node.annotation<std::reference_wrapper<const Command>>();
        const Command& cmd_op  = op_node.annotation<std::reference_wrapper<const Command>>();

        auto a = get_arg(eq_node.child(0));
        auto b = get_arg(op_node.child(0));
        auto c = get_arg(op_node.child(1));

        if(!is_same_var(a, b))
        {
            if(!is_same_var(a, c))
            {
                compile_command(cmd_set, { a, b }, not_flag);
                compile_command(cmd_op, { a, c }, not_flag);
            }
            else
            {
                // Safe. The annotate_tree step won't allow non-commutative
                // operations (substraction or division) to be compiled.
                compile_command(cmd_op, { a, b }, not_flag);
            }
        }
        else
        {
            compile_command(cmd_op, { a, c }, not_flag);
        }
    }
    else
    {
        // 'a = b' or 'a =# b' or 'a > b' (and such)

        bool invert = (eq_node.type() == NodeType::Lesser || eq_node.type() == NodeType::LesserEqual);

        auto& a_node = eq_node.child(!invert? 0 : 1);
        auto& b_node = eq_node.child(!invert? 1 : 0);

        const Command& cmd_set = eq_node.annotation<std::reference_wrapper<const Command>>();
        compile_command(cmd_set, { get_arg(a_node), get_arg(b_node) }, not_flag);
    }
}

void CompilerContext::compile_incdec(const SyntaxTree& op_node, bool not_flag)
{
    auto& annotation = op_node.annotation<const IncDecAnnotation&>();
    auto& var = op_node.child(0);
    compile_command(annotation.op_var_with_one, { get_arg(var), get_arg(annotation.number_one) });
}

void CompilerContext::compile_mission_end(const SyntaxTree& me_node, bool not_flag)
{
    const Command& command = me_node.annotation<std::reference_wrapper<const Command>>();
    return compile_command(command, {}, not_flag);
}

void CompilerContext::compile_condition(const SyntaxTree& node, bool not_flag)
{
    // also see compile_conditions
    switch(node.type())
    {
        case NodeType::NOT:
            return compile_condition(node.child(0), !not_flag);
        case NodeType::Command:
            return compile_command(node, not_flag);
        case NodeType::Equal:
        case NodeType::Greater:
        case NodeType::GreaterEqual:
        case NodeType::Lesser:
        case NodeType::LesserEqual:
            return compile_expr(node, not_flag);
        default:
            Unreachable();
    }
}

void CompilerContext::compile_conditions(const SyntaxTree& conds_node, const shared_ptr<Label>& else_ptr)
{
    auto compile_multi_andor = [this](const auto& conds_node, size_t op)
    {
        assert(conds_node.child_count() <= 8);
        compile_command(*this->commands.andor, { conv_int(op + conds_node.child_count() - 2) });
        for(auto& cond : conds_node) compile_condition(*cond);
    };

    switch(conds_node.type())
    {
        // single condition (also see compile_condition)
        case NodeType::NOT:
        case NodeType::Command:
        case NodeType::Equal:
        case NodeType::Greater:
        case NodeType::GreaterEqual:
        case NodeType::Lesser:
        case NodeType::LesserEqual:
            if (!this->program.opt.optimize_andor)
                compile_command(*this->commands.andor, { conv_int(0) });
            compile_condition(conds_node);
            break;
        case NodeType::AND: // 1-8
            compile_multi_andor(conds_node, 1);
            break;
        case NodeType::OR: // 21-28
            compile_multi_andor(conds_node, 21);
            break;
        default:
            Unreachable();
    }

    compile_command(*this->commands.goto_if_false, { else_ptr });
}

auto CompilerContext::get_args(const Command& command, const std::vector<any>& params) -> ArgList
{
    ArgList args;
    args.reserve(params.size());

    for(auto& p : params)
        args.emplace_back(get_arg(p));

    return args;
}

auto CompilerContext::get_args(const Command& command, const SyntaxTree& command_node) -> ArgList
{
    Expects(command_node.child_count() >= 1); // command_name + [args...]

    ArgList args;
    args.reserve(command_node.child_count() - 1);

    for(auto it = std::next(command_node.begin()); it != command_node.end(); ++it)
        args.emplace_back( get_arg(**it) );

    return args;
}

ArgVariant CompilerContext::get_arg(const Commands::MatchArgument& a)
{
    if(is<int32_t>(a))
        return conv_int(get<int32_t>(a));
    else if(is<float>(a))
        return get<float>(a);
    else
        return get_arg(*get<const SyntaxTree*>(a));
}

ArgVariant CompilerContext::get_arg(const any& param)
{
    if(auto opt = any_cast<int32_t>(&param))
        return conv_int(*opt);
    else if(auto opt = any_cast<float>(&param))
        return *opt;
    else if(auto opt = any_cast<shared_ptr<Label>>(&param))
        return std::move(*opt);
    else
        Unreachable(); // implement more on necessity
}

ArgVariant CompilerContext::get_arg(const SyntaxTree& arg_node)
{
    switch(arg_node.type())
    {
        case NodeType::Integer:
        {
            return conv_int(arg_node.annotation<int32_t>());
        }

        case NodeType::Float:
        {
            return arg_node.annotation<float>();
        }

        case NodeType::Text:
        {
            if(auto opt_int = arg_node.maybe_annotation<int32_t>())
            {
                return conv_int(*opt_int);
            }
            else if(auto opt_flt = arg_node.maybe_annotation<float>())
            {
                return *opt_flt;
            }
            else if(auto opt_var = arg_node.maybe_annotation<shared_ptr<Var>>())
            {
                return CompiledVar{ std::move(*opt_var), nullopt };
            }
            else if(auto opt_var = arg_node.maybe_annotation<const ArrayAnnotation&>())
            {
                if(is<shared_ptr<Var>>(opt_var->index))
                    return CompiledVar { opt_var->base, get<shared_ptr<Var>>(opt_var->index) };
                else
                    return CompiledVar { opt_var->base, get<int32_t>(opt_var->index) };
            }
            else if(auto opt_label = arg_node.maybe_annotation<shared_ptr<Label>>())
            {
                auto label = std::move(*opt_label);
                if(!label->may_branch_from(*this->script, program))
                {
                    auto sckind_ = to_string(label->script.lock()->type);
                    program.error(arg_node, "reference to local label outside of its {} script", sckind_);
                    program.note(*label->script.lock(), "label belongs to this script");
                }
                return label;
            }
            else if(auto opt_text = arg_node.maybe_annotation<const TextLabelAnnotation&>())
            {
                // TODO FIXME this is not the correct place to put this check, but I'm too lazy atm to put in the analyzes phase.
                if(program.opt.warn_conflict_text_label_var && symbols.find_var(opt_text->string, this->current_scope))
                    program.warning(arg_node, "text label collides with some variable name");

                auto type = opt_text->is_varlen? CompiledString::Type::StringVar : CompiledString::Type::TextLabel8;
                return CompiledString{ type, opt_text->preserve_case, opt_text->string };
            }
            else if(auto opt_umodel = arg_node.maybe_annotation<const ModelAnnotation&>())
            {
                assert(opt_umodel->where.expired() == false);
                int32_t i32 = opt_umodel->where.lock()->find_model_at(opt_umodel->id);
                return conv_int(i32);
            }
            else
            {
                Unreachable();
            }
            break;
        }

        case NodeType::String:
        {
            if(auto opt_text = arg_node.maybe_annotation<const TextLabelAnnotation&>())
            {
                auto type = opt_text->is_varlen? CompiledString::Type::StringVar : CompiledString::Type::TextLabel8;
                return CompiledString{ type, opt_text->preserve_case, opt_text->string };
            }
            else if(auto opt_buffer = arg_node.maybe_annotation<const String128Annotation&>())
            {
                return CompiledString { CompiledString::Type::String128, false, opt_buffer->string };
            }
            else
            {
                Unreachable();
            }
            break;
        }

        default:
            Unreachable();
    }
}

bool CompilerContext::is_same_var(const ArgVariant& lhs, const ArgVariant& rhs)
{
    if(is<CompiledVar>(lhs) && is<CompiledVar>(rhs))
    {
        return get<CompiledVar>(lhs) == get<CompiledVar>(rhs);
    }
    return false;
}

#include "charconv.hpp"
#include <gta3sc/sema.hpp>

// gta3script-specs 7fe565c767ee85fb8c99b594b3b3d280aa1b1c80

// TODO entity checking

namespace gta3sc
{
using ParamType = CommandManager::ParamType;

auto Sema::validate() -> std::optional<LinkedIR<SemaIR>>
{
    assert(!ran_analysis);
    this->ran_analysis = true;

    if(discover_declarations_pass())
        return check_semantics_pass();
    else
        return std::nullopt;
}

bool Sema::discover_declarations_pass()
{
    assert(report_count == 0);
    this->current_scope = -1;

    for(auto& line : parser_ir)
    {
        if(line.label)
        {
            declare_label(*line.label);
        }

        if(line.command)
        {
            if(line.command->name == "{")
            {
                assert(current_scope == -1);
                this->current_scope = symrepo->allocate_scope();
            }
            else if(line.command->name == "}")
            {
                assert(current_scope != -1);
                this->current_scope = -1;
            }
            else if(line.command->name == "VAR_INT")
            {
                declare_variable(*line.command, 0, SymVariable::Type::INT);
            }
            else if(line.command->name == "LVAR_INT")
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::INT);
            }
            else if(line.command->name == "VAR_FLOAT")
            {
                declare_variable(*line.command, 0, SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "LVAR_FLOAT")
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "VAR_TEXT_LABEL")
            {
                declare_variable(*line.command, 0,
                                 SymVariable::Type::TEXT_LABEL);
            }
            else if(line.command->name == "LVAR_TEXT_LABEL")
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::TEXT_LABEL);
            }
        }
    }

    // Ensure variables do not collide with other names in the same namespace.
    for(ScopeId i = 0; i < symrepo->var_tables.size(); ++i)
    {
        for(const auto& [name, var] : symrepo->var_tables[i])
        {
            if(i != 0 && symrepo->lookup_var(name, 0))
                report(var->source, Diag::duplicate_var_lvar);

            if(cmdman->find_constant_any_means(name))
                report(var->source, Diag::duplicate_var_string_constant);
        }
    }

    return (report_count == 0);
}

auto Sema::check_semantics_pass() -> std::optional<LinkedIR<SemaIR>>
{
    assert(report_count == 0);

    LinkedIR<SemaIR> linked(*arena);

    this->current_scope = -1;
    ScopeId scope_accum = 0;

    for(auto& line : parser_ir)
    {
        this->analyzing_var_decl = false;
        this->analyzing_alternative_command = false;

        linked.push_back(SemaIR::create(arena));

        if(line.command)
        {
            if(line.command->name == "{")
            {
                assert(this->current_scope == -1);
                this->current_scope = ++scope_accum;
            }
            else if(line.command->name == "}")
            {
                assert(this->current_scope != -1);
                this->current_scope = -1;
            }
            else if(line.command->name == "VAR_INT"
                    || line.command->name == "LVAR_INT"
                    || line.command->name == "VAR_FLOAT"
                    || line.command->name == "LVAR_FLOAT"
                    || line.command->name == "VAR_TEXT_LABEL"
                    || line.command->name == "LVAR_TEXT_LABEL")
            {
                this->analyzing_var_decl = true;
            }
        }

        if(line.label)
            linked.back().label = validate_label_def(*line.label);

        if(line.command)
            linked.back().command = validate_command(*line.command);
    }

    if(report_count != 0)
        return std::nullopt;

    return linked;
}

auto Sema::validate_label_def(const ParserIR::LabelDef& label_def)
        -> observer_ptr<SymLabel>
{
    auto sym_label = symrepo->lookup_label(label_def.name);
    if(!sym_label)
    {
        report(label_def.source, Diag::undefined_label);
        return nullptr;
    }
    return sym_label;
}

auto Sema::validate_command(const ParserIR::Command& command)
        -> arena_ptr<SemaIR::Command>
{
    bool failed = false;
    const CommandManager::CommandDef* command_def{};

    if(const auto alternator = cmdman->find_alternator(command.name))
    {
        const auto& alternatives = alternator->alternatives;
        auto it = std::find_if(alternatives.begin(), alternatives.end(),
                               [&](const auto* alternative) {
                                   return is_matching_alternative(command,
                                                                  *alternative);
                               });
        if(it == alternatives.end())
        {
            report(command.source, Diag::alternator_mismatch);
            return nullptr;
        }

        command_def = *it;
        assert(command_def);
        this->analyzing_alternative_command = true;
    }
    else
    {
        command_def = cmdman->find_command(command.name);
        if(!command_def)
        {
            report(command.source, Diag::undefined_command);
            return nullptr;
        }
    }

    auto result = SemaIR::Command::create(*command_def, command.source, arena);
    result->not_flag = command.not_flag;

    auto arg_it = command.args.begin();
    auto param_it = command_def->params.begin();

    while(arg_it != command.args.end() && param_it != command_def->params.end())
    {
        if(auto ir_arg = validate_argument(*param_it, **arg_it);
           ir_arg && !failed)
            result->push_arg(ir_arg, arena);
        else
            failed = true;

        ++arg_it;
        if(!param_it->is_optional())
            ++param_it;
    }

    const auto expected_args = (command_def->params.size()
                                - param_it->is_optional());
    const auto got_args = command.args.size();

    if(arg_it != command.args.end())
    {
        failed = true;
        report(command.source, Diag::too_many_arguments)
                .args(expected_args, got_args);
    }
    else if(param_it != command_def->params.end() && !param_it->is_optional())
    {
        failed = true;
        report(command.source, Diag::too_few_arguments)
                .args(expected_args, got_args);
    }

    return failed ? nullptr : result;
}

auto Sema::validate_argument(const CommandManager::ParamDef& param,
                             const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    switch(param.type)
    {
        case ParamType::INT:
        {
            if(analyzing_alternative_command && arg.as_identifier())
            {
                auto cdef = cmdman->find_constant(cmdman->global_enum,
                                                  *arg.as_identifier());
                assert(cdef != nullptr);

                return SemaIR::create_string_constant(*cdef, arg.source, arena);
            }
            return validate_integer_literal(param, arg);
        }
        case ParamType::FLOAT:
        {
            return validate_float_literal(param, arg);
        }
        case ParamType::TEXT_LABEL:
        {
            if(!arg.as_identifier())
            {
                report(arg.source, Diag::expected_text_label);
                return nullptr;
            }

            if(cmdman->find_constant(cmdman->global_enum, *arg.as_identifier()))
            {
                report(arg.source, Diag::cannot_use_string_constant_here);
                return nullptr;
            }

            if(arg.as_identifier()->front() == '$')
                return validate_var_ref(param, arg);
            else
                return validate_text_label(param, arg);
        }
        case ParamType::LABEL:
        {
            return validate_label(param, arg);
        }
        case ParamType::STRING:
        {
            return validate_string_literal(param, arg);
        }
        case ParamType::VAR_INT:
        case ParamType::LVAR_INT:
        case ParamType::VAR_FLOAT:
        case ParamType::LVAR_FLOAT:
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::VAR_INT_OPT:
        case ParamType::LVAR_INT_OPT:
        case ParamType::VAR_FLOAT_OPT:
        case ParamType::LVAR_FLOAT_OPT:
        case ParamType::VAR_TEXT_LABEL_OPT:
        case ParamType::LVAR_TEXT_LABEL_OPT:
        {
            return validate_var_ref(param, arg);
        }
        case ParamType::INPUT_INT:
        {
            if(analyzing_alternative_command)
            {
                // This command has been matched during alternation in a
                // command selector. Thus, according to the command selectors
                // matching rules, an INPUT_INT parameter has an argument
                // that corresponds to a string constant from any enumeration.

                assert(arg.as_identifier());
                auto cdef = cmdman->find_constant_any_means(
                        *arg.as_identifier());
                assert(cdef != nullptr);

                return SemaIR::create_string_constant(*cdef, arg.source, arena);
            }
            else if(arg.as_integer())
            {
                return validate_integer_literal(param, arg);
            }
            else if(arg.as_identifier())
            {
                if(auto cdef = cmdman->find_constant(param.enum_type,
                                                     *arg.as_identifier()))
                {
                    return SemaIR::create_string_constant(*cdef, arg.source,
                                                          arena);
                }
                return validate_var_ref(param, arg);
            }

            report(arg.source, Diag::expected_input_int);
            return nullptr;
        }
        case ParamType::INPUT_FLOAT:
        {
            if(arg.as_float())
                return validate_float_literal(param, arg);
            else if(arg.as_identifier())
            {
                if(cmdman->find_constant(cmdman->global_enum,
                                         *arg.as_identifier()))
                {
                    report(arg.source, Diag::cannot_use_string_constant_here);
                    return nullptr;
                }
                return validate_var_ref(param, arg);
            }

            report(arg.source, Diag::expected_input_float);
            return nullptr;
        }
        case ParamType::INPUT_OPT:
        {
            if(arg.as_integer())
            {
                return validate_integer_literal(param, arg);
            }
            else if(arg.as_float())
            {
                return validate_float_literal(param, arg);
            }
            else if(arg.as_identifier())
            {
                if(auto cdef = cmdman->find_constant(cmdman->global_enum,
                                                     *arg.as_identifier()))
                {
                    return SemaIR::create_string_constant(*cdef, arg.source,
                                                          arena);
                }
                return validate_var_ref(param, arg);
            }

            report(arg.source, Diag::expected_input_opt);
            return nullptr;
        }
        case ParamType::OUTPUT_INT:
        case ParamType::OUTPUT_FLOAT:
        {
            if(arg.as_identifier()
               && cmdman->find_constant(cmdman->global_enum,
                                        *arg.as_identifier()))
            {
                report(arg.source, Diag::cannot_use_string_constant_here);
                return nullptr;
            }
            return validate_var_ref(param, arg);
        }
        default:
        {
            assert(false);
            return nullptr;
        }
    }
}

auto Sema::validate_integer_literal(const CommandManager::ParamDef& param,
                                    const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    assert(param.type == ParamType::INT || param.type == ParamType::INPUT_INT
           || param.type == ParamType::INPUT_OPT);

    int32_t value = 0; // always recovers to this value

    if(const auto integer = arg.as_integer())
        value = *integer;
    else
        report(arg.source, Diag::expected_integer);

    return SemaIR::create_integer(value, arg.source, arena);
}

auto Sema::validate_float_literal(const CommandManager::ParamDef& param,
                                  const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    assert(param.type == ParamType::FLOAT
           || param.type == ParamType::INPUT_FLOAT
           || param.type == ParamType::INPUT_OPT);

    float value = 0.0f; // always recovers to this value

    if(const auto floating = arg.as_float())
        value = *floating;
    else
        report(arg.source, Diag::expected_float);

    return SemaIR::create_float(value, arg.source, arena);
}

auto Sema::validate_text_label(const CommandManager::ParamDef& param,
                               const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    assert(param.type == ParamType::TEXT_LABEL);

    std::string_view value = "DUMMY"; // always recovers to this value

    if(const auto ident = arg.as_identifier())
        value = *ident;
    else
        report(arg.source, Diag::expected_text_label);

    return SemaIR::create_text_label(value, arg.source, arena);
}

auto Sema::validate_label(const CommandManager::ParamDef& param,
                          const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    assert(param.type == ParamType::LABEL);

    if(!arg.as_identifier())
    {
        report(arg.source, Diag::expected_label);
        return nullptr;
    }

    auto sym_label = symrepo->lookup_label(*arg.as_identifier());
    if(!sym_label)
    {
        report(arg.source, Diag::undefined_label);
        return nullptr;
    }

    return SemaIR::create_label(sym_label, arg.source, arena);
}

auto Sema::validate_string_literal(const CommandManager::ParamDef& param,
                                   const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    assert(param.type == ParamType::STRING);

    if(!arg.as_string())
    {
        report(arg.source, Diag::expected_string);
        return nullptr;
    }

    return SemaIR::create_string(*arg.as_string(), arg.source, arena);
}

bool Sema::is_matching_alternative(
        const ParserIR::Command& command,
        const CommandManager::CommandDef& alternative)
{
    // Alternators do not admit optional arguments, so it's
    // all good to perform this check beforehand.
    if(command.args.size() != alternative.params.size())
        return false;

    for(size_t i = 0, acount = command.args.size(); i < acount; ++i)
    {
        const auto& arg = *command.args[i];
        const auto& param = alternative.params[i];

        // The global string constants have higher precedence when checking for
        // anything that is an identifier, and that shall only match with INTs.
        if(param.type != ParamType::INT && arg.as_identifier()
           && cmdman->find_constant(cmdman->global_enum, *arg.as_identifier()))
        {
            return false;
        }

        switch(param.type)
        {
            case ParamType::INT:
            {
                if(arg.as_identifier())
                {
                    if(!cmdman->find_constant(cmdman->global_enum,
                                              *arg.as_identifier()))
                        return false;
                }
                else if(!arg.as_integer())
                {
                    return false;
                }
                break;
            }
            case ParamType::FLOAT:
            {
                if(!arg.as_float())
                    return false;
                break;
            }
            case ParamType::VAR_INT:
            case ParamType::VAR_FLOAT:
            case ParamType::VAR_TEXT_LABEL:
            {
                if(!arg.as_identifier())
                    return false;

                const auto arg_ident = *arg.as_identifier();
                auto [var_name, var_source, _] = parse_var_ref(arg_ident,
                                                               arg.source);
                auto sym_var = symrepo->lookup_var(var_name);
                if(!sym_var || !matches_var_type(param.type, sym_var->type))
                    return false;

                break;
            }
            case ParamType::LVAR_INT:
            case ParamType::LVAR_FLOAT:
            case ParamType::LVAR_TEXT_LABEL:
            {
                if(current_scope == -1)
                    return false;

                if(!arg.as_identifier())
                    return false;

                const auto arg_ident = *arg.as_identifier();
                auto [var_name, var_source, _] = parse_var_ref(arg_ident,
                                                               arg.source);
                auto sym_var = symrepo->lookup_var(var_name, current_scope);
                if(!sym_var || !matches_var_type(param.type, sym_var->type))
                    return false;

                break;
            }
            case ParamType::INPUT_INT:
            {
                if(!arg.as_identifier())
                    return false;

                const auto arg_ident = *arg.as_identifier();
                if(!cmdman->find_constant_any_means(arg_ident))
                    return false;

                break;
            }
            case ParamType::TEXT_LABEL:
            {
                if(!arg.as_identifier())
                    return false;
                break;
            }
            default:
            {
                return false;
            }
        }
    }

    return true;
}

auto Sema::validate_var_ref(const CommandManager::ParamDef& param,
                            const ParserIR::Argument& arg)
        -> arena_ptr<SemaIR::Argument>
{
    bool failed = false;
    SymVariable* sym_var{};
    SymVariable* sym_subscript{};

    if(!arg.as_identifier())
    {
        report(arg.source, Diag::expected_variable);
        return nullptr;
    }

    std::string_view arg_ident = *arg.as_identifier();
    SourceRange arg_source = arg.source;

    // For TEXT_LABEL parameters, the identifier begins with a dollar
    // character and its suffix references a variable of text label type.
    if(param.type == ParamType::TEXT_LABEL)
    {
        assert(arg_ident.front() == '$');

        if(arg_ident.size() == 1 || arg_ident[1] == '[' || arg_ident[1] == ']')
        {
            report(arg.source, Diag::expected_varname_after_dollar);
            return nullptr; // cannot recover because of parse_var_ref
        }

        arg_ident = arg_ident.substr(1);
        arg_source = arg_source.substr(1);
    }

    auto [var_name, var_source, subscript] = parse_var_ref(arg_ident,
                                                           arg_source);

    sym_var = lookup_var_lvar(var_name);
    if(!sym_var)
    {
        report(var_source, Diag::undefined_variable);
        return nullptr;
    }

    // Check whether the storage of the variable matches the parameter.
    if(is_gvar_param(param.type) && sym_var->scope != 0)
    {
        failed = true;
        report(var_source, Diag::expected_gvar_got_lvar);
    }
    else if(is_lvar_param(param.type) && sym_var->scope == 0)
    {
        failed = true;
        report(var_source, Diag::expected_lvar_got_gvar);
    }

    // Check whether the type of the variable matches the parameter.
    if(!matches_var_type(param.type, sym_var->type))
    {
        failed = true;
        report(var_source, Diag::var_type_mismatch);
    }

    // An array variable name which is not followed by a subscript
    // behaves as if its zero-indexed element is referenced.
    if(!subscript && sym_var->is_array())
    {
        subscript = VarSubscript{
                var_name,
                var_source,
                0,
        };
    }

    // The program is ill-formed if a variable name is followed by
    // a subscript but the variable is not an array.
    if(subscript && !sym_var->is_array())
    {
        failed = true;
        report(var_source, Diag::subscript_but_var_is_not_array);
    }

    // The program is ill-formed if the array subscript uses a negative
    // or out of bounds value for indexing.
    if(subscript && subscript->literal)
    {
        if(!analyzing_var_decl) // literal == dim would diag
        {
            if(subscript->literal < 0
               || subscript->literal >= sym_var->dim.value_or(1))
            {
                failed = true;
                report(subscript->source, Diag::subscript_out_of_range);
                subscript->literal = 0; // recover
            }
        }
    }

    if(subscript && !subscript->literal)
    {
        sym_subscript = lookup_var_lvar(subscript->name);
        if(!sym_subscript)
        {
            failed = true;
            report(subscript->source, Diag::undefined_variable);
            subscript->literal = 0; // recover
        }
        else if(sym_subscript->type != SymVariable::Type::INT)
        {
            failed = true;
            report(subscript->source, Diag::subscript_var_must_be_int);
            sym_subscript = nullptr; // recover
            subscript->literal = 0;  // recover
        }
        else if(sym_subscript->is_array())
        {
            failed = true;
            report(subscript->source, Diag::subscript_var_must_not_be_array);
            sym_subscript = nullptr; // recover
            subscript->literal = 0;  // recover
        }
    }

    if(failed)
        return nullptr;
    else if(sym_var && sym_subscript)
        return SemaIR::create_variable(sym_var, sym_subscript, arg_source,
                                       arena);
    else if(sym_var && subscript && subscript->literal)
        return SemaIR::create_variable(sym_var, *subscript->literal, arg_source,
                                       arena);
    else
        return SemaIR::create_variable(sym_var, arg_source, arena);
}

void Sema::declare_label(const ParserIR::LabelDef& label_def)
{
    if(auto [_, inserted] = symrepo->insert_label(label_def.name,
                                                  label_def.source);
       !inserted)
    {
        report(label_def.source, Diag::duplicate_label);
    }
}

void Sema::declare_variable(const ParserIR::Command& command, ScopeId scope_id_,
                            SymVariable::Type type)
{
    for(auto& arg : command.args)
    {
        ScopeId scope_id = scope_id_;

        if(!arg->as_identifier())
        {
            report(arg->source, Diag::expected_identifier);
            continue;
        }

        auto [var_name, var_source, subscript] = parse_var_ref(
                *arg->as_identifier(), arg->source);

        if(subscript && !subscript->literal)
        {
            report(subscript->source, Diag::var_decl_subscript_must_be_literal);
            subscript->literal = 1; // recover
        }

        if(subscript && *subscript->literal <= 0)
        {
            report(subscript->source, Diag::var_decl_subscript_must_be_nonzero);
            subscript->literal = 1; // recover
        }

        if(scope_id == -1)
        {
            report(arg->source, Diag::var_decl_outside_of_scope);
            scope_id = 0; // recover
        }

        std::optional<int32_t> subscript_literal;
        if(subscript)
            subscript_literal = subscript->literal;

        if(auto [_, inserted] = symrepo->insert_var(
                   var_name, scope_id, type, subscript_literal, arg->source);
           !inserted)
        {
            if(scope_id == 0)
                report(var_source, Diag::duplicate_var_global);
            else
                report(var_source, Diag::duplicate_var_in_scope);
        }
    }
}

auto Sema::report(SourceRange source, Diag message) -> DiagnosticBuilder
{
    this->report_count++;
    return diag_->report(source.begin(), message).range(source);
}

auto Sema::lookup_var_lvar(std::string_view name) const -> SymVariable*
{
    if(auto gvar = symrepo->lookup_var(name, 0))
        return gvar;

    if(current_scope != -1)
    {
        if(auto lvar = symrepo->lookup_var(name, current_scope))
            return lvar;
    }

    return nullptr;
}

bool Sema::is_gvar_param(ParamType param_type) const
{
    switch(param_type)
    {
        case ParamType::VAR_INT:
        case ParamType::VAR_FLOAT:
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::VAR_INT_OPT:
        case ParamType::VAR_FLOAT_OPT:
        case ParamType::VAR_TEXT_LABEL_OPT:
            return true;
        default:
            return false;
    }
}

bool Sema::is_lvar_param(ParamType param_type) const
{
    switch(param_type)
    {
        case ParamType::LVAR_INT:
        case ParamType::LVAR_FLOAT:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::LVAR_INT_OPT:
        case ParamType::LVAR_FLOAT_OPT:
        case ParamType::LVAR_TEXT_LABEL_OPT:
            return true;
        default:
            return false;
    }
}

bool Sema::matches_var_type(ParamType param_type,
                            SymVariable::Type var_type) const
{
    switch(param_type)
    {
        case ParamType::VAR_INT:
        case ParamType::LVAR_INT:
        case ParamType::VAR_INT_OPT:
        case ParamType::LVAR_INT_OPT:
        case ParamType::INPUT_INT:
        case ParamType::OUTPUT_INT:
            return (var_type == SymVariable::Type::INT);
        case ParamType::VAR_FLOAT:
        case ParamType::LVAR_FLOAT:
        case ParamType::VAR_FLOAT_OPT:
        case ParamType::LVAR_FLOAT_OPT:
        case ParamType::INPUT_FLOAT:
        case ParamType::OUTPUT_FLOAT:
            return (var_type == SymVariable::Type::FLOAT);
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::VAR_TEXT_LABEL_OPT:
        case ParamType::LVAR_TEXT_LABEL_OPT:
        case ParamType::TEXT_LABEL:
            return (var_type == SymVariable::Type::TEXT_LABEL);
        case ParamType::INPUT_OPT:
            return (var_type == SymVariable::Type::INT
                    || var_type == SymVariable::Type::FLOAT);
        default:
            return false;
    }
}

auto Sema::parse_var_ref(std::string_view identifier, SourceRange source)
        -> VarRef
{
    // subscript := '[' (variable_name | integer) ']' ;
    // variable := variable_name [ subscript ] ;

    // We need this parsing function in the semantic phase because, until
    // this point, we could not classify an identifier into either a
    // variable or something else (that something else e.g. labels could
    // contain brackets in its name).

    std::string_view var_name;
    SourceRange var_source;
    std::optional<VarSubscript> subscript;

    const auto is_bracket = [](char c) { return c == '[' || c == ']'; };
    const auto is_digit = [](char c) { return c >= '0' && c <= '9'; };

    assert(identifier.size() > 0);
    assert(!is_bracket(identifier.front()));

    const auto it_open = std::find_if(identifier.begin(), identifier.end(),
                                      is_bracket);
    if(it_open != identifier.end())
    {
        const auto it_open_pos = std::distance(identifier.begin(), it_open);
        if(*it_open == ']')
        {
            report(source.begin() + it_open_pos, Diag::expected_word).args("[");
            // Recovery strategy: Assume *it == ']'
        }

        const auto it_close = std::find_if(it_open + 1, identifier.end(),
                                           is_bracket);
        const auto it_close_pos = std::distance(identifier.begin(), it_close);

        if(it_close == identifier.end() || *it_close == '[')
        {
            report(source.begin() + it_close_pos, Diag::expected_word)
                    .args("]");
            // Recovery strategy: Assume *it_close == ']'
        }

        assert(it_open != identifier.begin());
        var_name = identifier.substr(0, it_open_pos);
        var_source = source.substr(0, it_open_pos);

        if(std::distance(it_open, it_close) <= 1)
        {
            report(source.begin() + it_open_pos + 1, Diag::expected_subscript);
            // Recovery strategy: Assume there is no subscript.
            subscript = std::nullopt;
        }
        else
        {
            const auto subscript_len = it_close - it_open - 1;
            subscript = VarSubscript{
                    identifier.substr(it_open_pos + 1, subscript_len),
                    source.substr(it_open_pos + 1, subscript_len),
            };
        }
    }
    else
    {
        // There is no array subscript. The identifier is the variable name.
        var_name = identifier;
        var_source = source;
    }

    assert(var_name.size() > 0);
    assert(var_name.size() == var_source.size());
    assert(!subscript || subscript->name.size() > 0);
    assert(!subscript || subscript->name.size() == subscript->source.size());

    // We have to validate the subscript is either another identifier
    // or a positive integer literal.
    if(subscript)
    {
        const auto& subval = subscript->name;

        if(subval.front() == '-')
        {
            report(subscript->source, Diag::subscript_must_be_positive);
            // Recovery strategy: Assume there is no subscript.
            subscript = std::nullopt;
        }
        else if(is_digit(subval.front()))
        {
            if(std::all_of(subval.begin(), subval.end(), is_digit))
            {
                using namespace gta3sc::utils;
                int32_t value;

                if(auto [_, ec] = from_chars(&*subval.begin(), &*subval.end(),
                                             value);
                   ec != std::errc())
                {
                    assert(ec == std::errc::result_out_of_range);
                    report(subscript->source, Diag::integer_literal_too_big);
                    // Recovery strategy: Assume there is no subscript.
                    subscript = std::nullopt;
                }
                else
                {
                    subscript->literal = value;
                }
            }
            else
            {
                report(subscript->source, Diag::expected_integer);
                // Recovery strategy: Assume there is no subscript.
                subscript = std::nullopt;
            }

            assert(!subscript || subscript->literal);
        }
        else
        {
            // We could check whether this is a valid identiifer (i.e.
            // it begins in letters (or $) and does not end in a colon.
            //
            // But we won't. There is no benefit in doing so. If this is a
            // invalid identifier, we'll get a diagnostic about undefined
            // variable during the semantic checking of the subscript itself.
        }
    }

    return VarRef{var_name, var_source, std::move(subscript)};
}
}

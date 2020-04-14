#include "charconv.hpp"
#include <gta3sc/syntax/sema.hpp>
using namespace std::literals::string_view_literals;

// gta3script-specs 7fe565c767ee85fb8c99b594b3b3d280aa1b1c80

namespace gta3sc::syntax
{
using ParamType = CommandManager::ParamType;
using EntityId = CommandManager::EntityId;

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
            if(line.command->name == "{"sv)
            {
                assert(current_scope == -1);
                this->current_scope = symrepo->allocate_scope();

                // We need the index of the first local scope in order to
                // enumerate scopes during `check_semantics_pass`.
                if(this->first_scope == -1)
                {
                    this->first_scope = current_scope;
                }
            }
            else if(line.command->name == "}"sv)
            {
                assert(current_scope != -1);
                this->current_scope = -1;
            }
            else if(line.command->name == "VAR_INT"sv)
            {
                declare_variable(*line.command, 0, SymVariable::Type::INT);
            }
            else if(line.command->name == "LVAR_INT"sv)
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::INT);
            }
            else if(line.command->name == "VAR_FLOAT"sv)
            {
                declare_variable(*line.command, 0, SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "LVAR_FLOAT"sv)
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "VAR_TEXT_LABEL"sv)
            {
                declare_variable(*line.command, 0,
                                 SymVariable::Type::TEXT_LABEL);
            }
            else if(line.command->name == "LVAR_TEXT_LABEL"sv)
            {
                declare_variable(*line.command, current_scope,
                                 SymVariable::Type::TEXT_LABEL);
            }
        }
    }

    // Ensure variables do not collide with other names in the same namespace.
    for(ScopeId i = 0; i < symrepo->var_tables.size(); ++i)
    {
        if(i == 0 || i >= first_scope)
        {
            for(const auto& [name, var] : symrepo->var_tables[i])
            {
                if(i != 0 && symrepo->lookup_var(name, 0))
                    report(var->source, Diag::duplicate_var_lvar);

                if(cmdman->find_constant_any_means(name))
                    report(var->source, Diag::duplicate_var_string_constant);
            }
        }
    }

    return (report_count == 0);
}

auto Sema::check_semantics_pass() -> std::optional<LinkedIR<SemaIR>>
{
    assert(report_count == 0);

    LinkedIR<SemaIR> linked;

    this->current_scope = -1;
    ScopeId scope_accum = first_scope;

    this->alternator_set = cmdman->find_alternator("SET"sv);
    this->command_script_name = cmdman->find_command("SCRIPT_NAME"sv);
    this->command_start_new_script = cmdman->find_command("START_NEW_SCRIPT"sv);

    this->model_enum = cmdman->find_enumeration("MODEL");
    this->defaultmodel_enum = cmdman->find_enumeration("DEFAULTMODEL");

    for(auto& line : parser_ir)
    {
        this->analyzing_var_decl = false;
        this->analyzing_alternative_command = false;
        this->analyzing_repeat_command = false;

        linked.push_back(*SemaIR::create(arena));

        if(line.command)
        {
            if(line.command->name == "{"sv)
            {
                assert(this->current_scope == -1);
                assert(scope_accum > 0);
                this->current_scope = scope_accum++;
            }
            else if(line.command->name == "}"sv)
            {
                assert(this->current_scope != -1);
                this->current_scope = -1;
            }
            else if(line.command->name == "VAR_INT"sv
                    || line.command->name == "LVAR_INT"sv
                    || line.command->name == "VAR_FLOAT"sv
                    || line.command->name == "LVAR_FLOAT"sv
                    || line.command->name == "VAR_TEXT_LABEL"sv
                    || line.command->name == "LVAR_TEXT_LABEL"sv)
            {
                this->analyzing_var_decl = true;
            }
            else if(line.command->name == "REPEAT"sv)
            {
                this->analyzing_repeat_command = true;
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
        // This is impossible! All the labels in the input IR were previously
        // defined in `discover_declarations_pass`.
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
        auto it = std::find_if(alternator->begin(), alternator->end(),
                               [&](const auto& alternative) {
                                   return is_matching_alternative(
                                           command, *alternative.command);
                               });
        if(it == alternator->end())
        {
            report(command.source, Diag::alternator_mismatch);
            return nullptr;
        }

        command_def = it->command;
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

    if(!failed)
    {
        if(!validate_hardcoded_command(*result))
            failed = true;
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
                // This command has been matched during alternation in a
                // command selector. Thus, if this is an identifier, it
                // is a valid global string constant.
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
                std::string_view ident = *arg.as_identifier();
                if(is_object_param(param))
                {
                    if(auto cdef = find_defaultmodel_constant(ident))
                    {
                        return SemaIR::create_string_constant(*cdef, arg.source,
                                                              arena);
                    }
                    else if(auto model = modelman->find_model(ident))
                    {
                        auto [uobj, _] = symrepo->insert_used_object(
                                ident, arg.source);
                        assert(uobj != nullptr);
                        return SemaIR::create_used_object(uobj, arg.source,
                                                          arena);
                    }
                }
                else if(auto cdef = cmdman->find_constant(param.enum_type,
                                                          ident))
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

    std::string_view value{"DUMMY"sv}; // always recovers to this value

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
        arg_source = arg_source.subrange(1);
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
        if(!analyzing_repeat_command) // REPEAT hardcodes the acceptance
        {                             // of LVAR_INTs parameters
            failed = true;
            report(var_source, Diag::expected_gvar_got_lvar);
        }
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

    if(param.entity_type != EntityId{0})
    {
        if(param.type == ParamType::OUTPUT_INT
           && sym_var->entity_type == EntityId{0})
        {
            sym_var->entity_type = param.entity_type;
        }

        if(sym_var->entity_type != param.entity_type)
        {
            failed = true;
            report(var_source, Diag::var_entity_type_mismatch);
        }
    }

    if(failed)
        return nullptr;
    else if(sym_subscript)
        return SemaIR::create_variable(sym_var, sym_subscript, arg_source,
                                       arena);
    else if(subscript && subscript->literal)
        return SemaIR::create_variable(sym_var, *subscript->literal, arg_source,
                                       arena);
    else
        return SemaIR::create_variable(sym_var, arg_source, arena);
}

bool Sema::validate_hardcoded_command(const SemaIR::Command& command)
{
    if(&command.def == command_script_name)
        return validate_script_name(command);
    if(&command.def == command_start_new_script)
        return validate_start_new_script(command);
    else if(alternator_set
            && is_alternative_command(command.def, *alternator_set))
        return validate_set(command);
    else
        return true;
}

bool Sema::validate_set(const SemaIR::Command& command)
{
    assert(is_alternative_command(command.def, *alternator_set));

    if(command.args.size() == 2)
    {
        auto lhs = command.args[0]->as_var_ref();
        auto rhs = command.args[1]->as_var_ref();
        if(lhs && rhs)
        {
            if(lhs->def->entity_type == EntityId{0}
               && rhs->def->entity_type != EntityId{0})
            {
                lhs->def->entity_type = rhs->def->entity_type;
            }
            else if(lhs->def->entity_type != rhs->def->entity_type)
            {
                report(command.source, Diag::var_entity_type_mismatch)
                        .range(command.args[0]->source)
                        .range(command.args[1]->source);
                return false;
            }
        }
    }

    return true;
}

bool Sema::validate_script_name(const SemaIR::Command& command)
{
    assert(&command.def == command_script_name);

    if(command.args.size() == 1)
    {
        if(const auto name = command.args[0]->as_text_label())
        {
            auto it = std::find(seen_script_names.begin(),
                                seen_script_names.end(), *name);
            if(it != seen_script_names.end())
            {
                report(command.args[0]->source, Diag::duplicate_script_name);
                return false;
            }
            else
            {
                seen_script_names.push_back(*name);
                return true;
            }
        }
    }

    return true;
}

bool Sema::validate_start_new_script(const SemaIR::Command& command)
{
    assert(&command.def == command_start_new_script);

    if(command.args.size() >= 1)
    {
        if(auto target_label = command.args[0]->as_label())
        {
            if(target_label->scope == 0)
            {
                report(command.args[0]->source,
                       Diag::target_label_not_within_scope);
                return false;
            }

            if(!validate_target_scope_vars(command.args.data() + 1,
                                           command.args.data()
                                                   + command.args.size(),
                                           target_label->scope))
            {
                return false;
            }
        }
    }

    return true;
}

bool Sema::validate_target_scope_vars(SemaIR::Argument** begin,
                                      SemaIR::Argument** end,
                                      ScopeId target_scope_id)
{
    assert(target_scope_id != 0);

    const size_t num_target_vars = end - begin;
    bool failed = false;

    if(num_target_vars == 0)
        return true;

    // Use a temporary buffer in the arena. This will be unused memory after
    // this function returns, but it's no big deal. It only happens for
    // START_NEW_SCRIPT alike commands and the allocation size is proportional
    // to the number of arguments passed.
    SymVariable** target_vars = new(*arena) SymVariable*[num_target_vars];
    std::fill(target_vars, target_vars + num_target_vars, nullptr);

    for(auto& [name, lvar] : symrepo->var_tables[target_scope_id])
    {
        if(lvar->id < num_target_vars)
            target_vars[lvar->id] = lvar;
    }

    for(size_t i = 0; i < num_target_vars; ++i)
    {
        const auto& arg = **(begin + i);
        SymVariable* target_var = target_vars[i];
        if(target_var == nullptr)
        {
            failed = true;
            report(arg.source, Diag::target_scope_not_enough_vars);
        }
        else
        {
            // The parameter type used for passing arguments into target
            // scope variables is INPUT_OPT, which only accepts integers,
            // floating points, global string constants and variable
            // references as argument. The semantic checker already took
            // care of validating that for us, so for sure the argument
            // is one of these. We'll support additionally text labels,
            // but anything else is a logic error in the compiler.
            if(arg.as_punned_integer())
            {
                if(target_vars[i]->type != SymVariable::Type::INT)
                {
                    failed = true;
                    report(arg.source, Diag::target_var_type_mismatch);
                }
            }
            else if(arg.as_punned_float())
            {
                if(target_vars[i]->type != SymVariable::Type::FLOAT)
                {
                    failed = true;
                    report(arg.source, Diag::target_var_type_mismatch);
                }
            }
            else if(arg.as_text_label())
            {
                if(target_vars[i]->type != SymVariable::Type::TEXT_LABEL)
                {
                    failed = true;
                    report(arg.source, Diag::target_var_type_mismatch);
                }
            }
            else if(auto var_ref = arg.as_var_ref())
            {
                const auto* source_var = var_ref->def;
                if(target_vars[i]->type != source_var->type)
                {
                    failed = true;
                    report(arg.source, Diag::target_var_type_mismatch);
                }
                else if(target_vars[i]->entity_type == EntityId{0}
                        && source_var->entity_type != EntityId{0})
                {
                    target_var->entity_type = source_var->entity_type;
                }
                else if(target_var->entity_type != source_var->entity_type)
                {
                    failed = true;
                    report(arg.source, Diag::target_var_entity_type_mismatch);
                }
            }
            else
            {
                failed = true;
                report(arg.source, Diag::internal_compiler_error);
            }
        }
    }

    return !failed;
}

void Sema::declare_label(const ParserIR::LabelDef& label_def)
{
    const auto scope_id = current_scope == -1 ? 0 : current_scope;
    if(auto [_, inserted] = symrepo->insert_label(label_def.name, scope_id,
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

auto Sema::report(SourceLocation loc, Diag message) -> DiagnosticBuilder
{
    this->report_count++;
    return diag_->report(loc, message);
}

auto Sema::report(SourceRange source, Diag message) -> DiagnosticBuilder
{
    return report(source.begin, message).range(source);
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

auto Sema::find_defaultmodel_constant(std::string_view name) const
        -> const CommandManager::ConstantDef*
{
    if(this->defaultmodel_enum)
        return cmdman->find_constant(*defaultmodel_enum, name);
    return nullptr;
}

bool Sema::is_object_param(const CommandManager::ParamDef& param) const
{
    return this->model_enum && param.enum_type == *model_enum;
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

bool Sema::is_alternative_command(
        const CommandManager::CommandDef& command_def,
        const CommandManager::AlternatorDef& from) const
{
    auto it = std::find_if(from.begin(), from.end(),
                           [&](const auto& alternative) {
                               return &command_def == alternative.command;
                           });
    return (it != from.end());
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
            report(source.begin + it_open_pos, Diag::expected_word).args("[");
            // Recovery strategy: Assume *it == ']'
        }

        const auto it_close = std::find_if(it_open + 1, identifier.end(),
                                           is_bracket);
        const auto it_close_pos = std::distance(identifier.begin(), it_close);

        if(it_close == identifier.end() || *it_close == '[')
        {
            report(source.begin + it_close_pos, Diag::expected_word).args("]");
            // Recovery strategy: Assume *it_close == ']'
        }

        assert(it_open != identifier.begin());
        var_name = identifier.substr(0, it_open_pos);
        var_source = source.subrange(0, it_open_pos);

        if(std::distance(it_open, it_close) <= 1)
        {
            report(source.begin + it_open_pos + 1, Diag::expected_subscript);
            // Recovery strategy: Assume there is no subscript.
            subscript = std::nullopt;
        }
        else
        {
            const auto subscript_len = it_close - it_open - 1;
            subscript = VarSubscript{
                    identifier.substr(it_open_pos + 1, subscript_len),
                    source.subrange(it_open_pos + 1, subscript_len),
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

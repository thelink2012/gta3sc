#include "charconv.hpp"
#include <gta3sc/syntax/sema.hpp>
using namespace std::literals::string_view_literals;

// gta3script-specs 7fe565c767ee85fb8c99b594b3b3d280aa1b1c80

namespace gta3sc::syntax
{
using ParamType = CommandTable::ParamType;
using EntityId = CommandTable::EntityId;

static constexpr auto varname_timera = "TIMERA"sv;
static constexpr auto varname_timerb = "TIMERB"sv;

auto Sema::validate() -> std::optional<LinkedIR<SemaIR>>
{
    assert(!ran_analysis);
    this->ran_analysis = true;

    if(discover_declarations_pass())
        return check_semantics_pass();
    else
        return std::nullopt;
}

auto Sema::discover_declarations_pass() -> bool
{
    assert(report_count == 0);
    SourceRange scope_enter_source{};
    this->current_scope = no_local_scope;

    for(auto& line : parser_ir)
    {
        if(line.has_label())
        {
            declare_label(line.label());
        }

        if(line.has_command())
        {
            if(line.command().name() == "{"sv)
            {
                assert(current_scope == no_local_scope);
                this->current_scope = symrepo->new_scope();
                scope_enter_source = line.command().source();

                // We need the index of the first local scope in order to
                // enumerate scopes during `check_semantics_pass`.
                if(this->first_scope == no_local_scope)
                {
                    this->first_scope = current_scope;
                }
            }
            else if(line.command().name() == "}"sv)
            {
                assert(current_scope != no_local_scope);

                // Instead of inserting the timer variable into the symbol table
                // as the scope is activated, we do it just before deactivation.
                // This way, the id of the variables are the last ones in the
                // scope, making the placement of timers more intuitive.

                const auto [_, inserted_timera] = symrepo->insert_var(
                        varname_timera, current_scope,
                        SymbolTable::VarType::INT, std::nullopt,
                        scope_enter_source);

                const auto [__, inserted_timerb] = symrepo->insert_var(
                        varname_timerb, current_scope,
                        SymbolTable::VarType::INT, std::nullopt,
                        scope_enter_source);

                assert(inserted_timera && inserted_timerb);

                this->current_scope = no_local_scope;
                scope_enter_source = SourceRange{};
            }
            else if(line.command().name() == "VAR_INT"sv)
            {
                declare_variable(line.command(), SymbolTable::global_scope,
                                 SymbolTable::VarType::INT);
            }
            else if(line.command().name() == "LVAR_INT"sv)
            {
                declare_variable(line.command(), current_scope,
                                 SymbolTable::VarType::INT);
            }
            else if(line.command().name() == "VAR_FLOAT"sv)
            {
                declare_variable(line.command(), SymbolTable::global_scope,
                                 SymbolTable::VarType::FLOAT);
            }
            else if(line.command().name() == "LVAR_FLOAT"sv)
            {
                declare_variable(line.command(), current_scope,
                                 SymbolTable::VarType::FLOAT);
            }
            else if(line.command().name() == "VAR_TEXT_LABEL"sv)
            {
                declare_variable(line.command(), SymbolTable::global_scope,
                                 SymbolTable::VarType::TEXT_LABEL);
            }
            else if(line.command().name() == "LVAR_TEXT_LABEL"sv)
            {
                declare_variable(line.command(), current_scope,
                                 SymbolTable::VarType::TEXT_LABEL);
            }
        }
    }

    // Allocate `vars_entity_type` for scopes in the symbol table.
    this->vars_entity_type.resize(symrepo->num_scopes());
    for(uint32_t i = 0, num_scopes = symrepo->num_scopes(); i < num_scopes; ++i)
    {
        this->vars_entity_type[i].resize(
                symrepo->scope(SymbolTable::ScopeId{i}).size());
    }

    // Ensure variables do not collide with other names in the same namespace.
    for(uint32_t i = 0; i < symrepo->num_scopes(); ++i)
    {
        if(i == to_integer(SymbolTable::global_scope)
           || i >= to_integer(first_scope))
        {
            for(const auto& var : symrepo->scope(SymbolTable::ScopeId{i}))
            {
                if(i != to_integer(SymbolTable::global_scope)
                   && symrepo->lookup_var(var.name(),
                                          SymbolTable::global_scope))
                {
                    report(var.source(), Diag::duplicate_var_lvar);
                }

                if(cmdman->find_constant_any_means(var.name()))
                    report(var.source(), Diag::duplicate_var_string_constant);
            }
        }
    }

    return (report_count == 0);
}

auto Sema::check_semantics_pass() -> std::optional<LinkedIR<SemaIR>>
{
    assert(report_count == 0);

    LinkedIR<SemaIR> linked;

    this->current_scope = no_local_scope;
    SymbolTable::ScopeId scope_accum = first_scope;

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

        SemaIR::Builder builder(allocator);

        if(line.has_command())
        {
            if(line.command().name() == "{"sv)
            {
                assert(this->current_scope == no_local_scope);
                assert(scope_accum != no_local_scope
                       && scope_accum != SymbolTable::global_scope);
                this->current_scope = scope_accum++;
            }
            else if(line.command().name() == "}"sv)
            {
                assert(this->current_scope != no_local_scope);
                this->current_scope = no_local_scope;
            }
            else if(line.command().name() == "VAR_INT"sv
                    || line.command().name() == "LVAR_INT"sv
                    || line.command().name() == "VAR_FLOAT"sv
                    || line.command().name() == "LVAR_FLOAT"sv
                    || line.command().name() == "VAR_TEXT_LABEL"sv
                    || line.command().name() == "LVAR_TEXT_LABEL"sv)
            {
                this->analyzing_var_decl = true;
            }
            else if(line.command().name() == "REPEAT"sv)
            {
                this->analyzing_repeat_command = true;
            }
        }

        if(line.has_label())
        {
            const SymbolTable::Label* label = validate_label_def(line.label());
            builder.label(label);
        }

        if(line.has_command())
        {
            const SemaIR::Command* command = validate_command(line.command());
            builder.command(command);
        }

        linked.push_back(*std::move(builder).build());
    }

    if(report_count != 0)
        return std::nullopt;

    return linked;
}

auto Sema::validate_label_def(const ParserIR::LabelDef& label_def)
        -> const SymbolTable::Label*
{
    const auto* sym_label = symrepo->lookup_label(label_def.name());
    if(!sym_label)
    {
        // This is impossible! All the labels in the input IR were previously
        // defined in `discover_declarations_pass`.
        report(label_def.source(), Diag::undefined_label);
        return nullptr;
    }
    return sym_label;
}

auto Sema::validate_command(const ParserIR::Command& command)
        -> ArenaPtr<const SemaIR::Command>
{
    bool failed = false;
    const CommandTable::CommandDef* command_def{};

    if(const auto* const alternator = cmdman->find_alternator(command.name()))
    {
        auto it = std::find_if(alternator->begin(), alternator->end(),
                               [&](const auto& alternative) {
                                   return is_matching_alternative(
                                           command, alternative.command());
                               });
        if(it == alternator->end())
        {
            report(command.source(), Diag::alternator_mismatch);
            return nullptr;
        }

        command_def = &it->command();
        assert(command_def);
        this->analyzing_alternative_command = true;
    }
    else
    {
        command_def = cmdman->find_command(command.name());
        if(!command_def)
        {
            report(command.source(), Diag::undefined_command);
            return nullptr;
        }
    }

    SemaIR::Builder builder(allocator);
    builder.command(*command_def, command.source());
    builder.not_flag(command.not_flag());
    builder.with_num_args(command.num_args());

    auto arg_it = command.args().begin();
    auto param_it = command_def->params().begin();

    while(arg_it != command.args().end()
          && param_it != command_def->params().end())
    {
        if(const auto* ir_arg = validate_argument(*param_it, *arg_it);
           ir_arg && !failed)
            builder.arg(ir_arg);
        else
            failed = true;

        ++arg_it;
        if(!param_it->is_optional())
            ++param_it;
    }

    const auto expected_args = command_def->num_min_params();
    const auto got_args = command.num_args();

    if(arg_it != command.args().end())
    {
        failed = true;
        report(command.source(), Diag::too_many_arguments)
                .args(expected_args, got_args);
    }
    else if(param_it != command_def->params().end() && !param_it->is_optional())
    {
        failed = true;
        report(command.source(), Diag::too_few_arguments)
                .args(expected_args, got_args);
    }

    ArenaPtr<const SemaIR::Command> result = std::move(builder).build_command();

    if(!failed)
    {
        if(!validate_hardcoded_command(*result))
            failed = true;
    }

    return failed ? nullptr : result;
}

auto Sema::validate_argument(const CommandTable::ParamDef& param,
                             const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    switch(param.type)
    {
        case ParamType::INT:
        {
            if(analyzing_alternative_command
               && arg.type() == ParserIR::Argument::Type::IDENTIFIER)
            {
                // This command has been matched during alternation in a
                // command selector. Thus, if this is an identifier, it
                // is a valid global string constant.
                const auto* cdef = cmdman->find_constant(
                        CommandTable::global_enum, *arg.as_identifier());
                assert(cdef != nullptr);

                return SemaIR::create_constant(*cdef, arg.source(), allocator);
            }
            return validate_integer_literal(param, arg);
        }
        case ParamType::FLOAT:
        {
            return validate_float_literal(param, arg);
        }
        case ParamType::TEXT_LABEL:
        {
            if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
            {
                report(arg.source(), Diag::expected_text_label);
                return nullptr;
            }

            std::string_view identifier = *arg.as_identifier();

            if(cmdman->find_constant(CommandTable::global_enum, identifier))
            {
                report(arg.source(), Diag::cannot_use_string_constant_here);
                return nullptr;
            }

            if(identifier.starts_with('$'))
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

                assert(arg.type() == ParserIR::Argument::Type::IDENTIFIER);
                const auto* cdef = cmdman->find_constant_any_means(
                        *arg.as_identifier());
                assert(cdef != nullptr);

                return SemaIR::create_constant(*cdef, arg.source(), allocator);
            }

            switch(arg.type())
            {
                case ParserIR::Argument::Type::INT:
                {
                    return validate_integer_literal(param, arg);
                }
                case ParserIR::Argument::Type::IDENTIFIER:
                {
                    std::string_view ident = *arg.as_identifier();
                    if(is_object_param(param))
                    {
                        if(const auto* cdef = find_defaultmodel_constant(ident))
                        {
                            return SemaIR::create_constant(*cdef, arg.source(),
                                                           allocator);
                        }
                        else if(const auto* model = modelman->find_model(ident))
                        {
                            auto [uobj, _] = symrepo->insert_used_object(
                                    ident, arg.source());
                            assert(uobj != nullptr);
                            return SemaIR::create_used_object(
                                    *uobj, arg.source(), allocator);
                        }
                    }
                    else if(const auto* cdef = cmdman->find_constant(
                                    param.enum_type, ident))
                    {
                        return SemaIR::create_constant(*cdef, arg.source(),
                                                       allocator);
                    }
                    return validate_var_ref(param, arg);
                }
                default:
                {
                    report(arg.source(), Diag::expected_input_int);
                    return nullptr;
                }
            }
        }
        case ParamType::INPUT_FLOAT:
        {
            switch(arg.type())
            {
                case ParserIR::Argument::Type::FLOAT:
                {
                    return validate_float_literal(param, arg);
                }
                case ParserIR::Argument::Type::IDENTIFIER:
                {
                    if(cmdman->find_constant(CommandTable::global_enum,
                                             *arg.as_identifier()))
                    {
                        report(arg.source(),
                               Diag::cannot_use_string_constant_here);
                        return nullptr;
                    }
                    return validate_var_ref(param, arg);
                }
                default:
                {
                    report(arg.source(), Diag::expected_input_float);
                    return nullptr;
                }
            }
        }
        case ParamType::INPUT_OPT:
        {
            switch(arg.type())
            {
                case ParserIR::Argument::Type::INT:
                {
                    return validate_integer_literal(param, arg);
                }
                case ParserIR::Argument::Type::FLOAT:
                {
                    return validate_float_literal(param, arg);
                }
                case ParserIR::Argument::Type::IDENTIFIER:
                {
                    if(const auto* cdef = cmdman->find_constant(
                               CommandTable::global_enum, *arg.as_identifier()))
                    {
                        return SemaIR::create_constant(*cdef, arg.source(),
                                                       allocator);
                    }
                    return validate_var_ref(param, arg);
                }
                default:
                {
                    report(arg.source(), Diag::expected_input_opt);
                    return nullptr;
                }
            }
        }
        case ParamType::OUTPUT_INT:
        case ParamType::OUTPUT_FLOAT:
        {
            if(arg.type() == ParserIR::Argument::Type::IDENTIFIER
               && cmdman->find_constant(CommandTable::global_enum,
                                        *arg.as_identifier()))
            {
                report(arg.source(), Diag::cannot_use_string_constant_here);
                return nullptr;
            }
            return validate_var_ref(param, arg);
        }
    }
    assert(false);
}

auto Sema::validate_integer_literal(const CommandTable::ParamDef& param,
                                    const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    assert(param.type == ParamType::INT || param.type == ParamType::INPUT_INT
           || param.type == ParamType::INPUT_OPT);

    int32_t value = 0; // always recovers to this value

    if(const auto integer = arg.as_int())
        value = *integer;
    else
        report(arg.source(), Diag::expected_integer);

    return SemaIR::create_int(value, arg.source(), allocator);
}

auto Sema::validate_float_literal(const CommandTable::ParamDef& param,
                                  const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    assert(param.type == ParamType::FLOAT
           || param.type == ParamType::INPUT_FLOAT
           || param.type == ParamType::INPUT_OPT);

    float value = 0.0F; // always recovers to this value

    if(const auto floating = arg.as_float())
        value = *floating;
    else
        report(arg.source(), Diag::expected_float);

    return SemaIR::create_float(value, arg.source(), allocator);
}

auto Sema::validate_text_label(const CommandTable::ParamDef& param,
                               const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    assert(param.type == ParamType::TEXT_LABEL);

    std::string_view value{"DUMMY"sv}; // always recovers to this value

    if(const auto ident = arg.as_identifier())
        value = *ident;
    else
        report(arg.source(), Diag::expected_text_label);

    return SemaIR::create_text_label(value, arg.source(), allocator);
}

auto Sema::validate_label(const CommandTable::ParamDef& param,
                          const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    assert(param.type == ParamType::LABEL);

    if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
    {
        report(arg.source(), Diag::expected_label);
        return nullptr;
    }

    const auto* sym_label = symrepo->lookup_label(*arg.as_identifier());
    if(!sym_label)
    {
        report(arg.source(), Diag::undefined_label);
        return nullptr;
    }

    return SemaIR::create_label(*sym_label, arg.source(), allocator);
}

auto Sema::validate_string_literal(const CommandTable::ParamDef& param,
                                   const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    assert(param.type == ParamType::STRING);

    if(arg.type() != ParserIR::Argument::Type::STRING)
    {
        report(arg.source(), Diag::expected_string);
        return nullptr;
    }

    return SemaIR::create_string(*arg.as_string(), arg.source(), allocator);
}

auto Sema::validate_var_ref(const CommandTable::ParamDef& param,
                            const ParserIR::Argument& arg)
        -> ArenaPtr<const SemaIR::Argument>
{
    bool failed = false;
    const SymbolTable::Variable* sym_var{};
    const SymbolTable::Variable* sym_subscript{};

    if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
    {
        report(arg.source(), Diag::expected_variable);
        return nullptr;
    }

    std::string_view arg_ident = *arg.as_identifier();
    SourceRange arg_source = arg.source();

    // For TEXT_LABEL parameters, the identifier begins with a dollar
    // character and its suffix references a variable of text label type.
    if(param.type == ParamType::TEXT_LABEL)
    {
        assert(arg_ident.starts_with('$'));

        if(arg_ident.size() == 1 || arg_ident[1] == '[' || arg_ident[1] == ']')
        {
            report(arg.source(), Diag::expected_varname_after_dollar);
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
    if(is_gvar_param(param.type)
       && sym_var->scope() != SymbolTable::global_scope)
    {
        if(!analyzing_repeat_command) // REPEAT hardcodes the acceptance
        {                             // of LVAR_INTs parameters
            failed = true;
            report(var_source, Diag::expected_gvar_got_lvar);
        }
    }
    else if(is_lvar_param(param.type)
            && sym_var->scope() == SymbolTable::global_scope)
    {
        failed = true;
        report(var_source, Diag::expected_lvar_got_gvar);
    }

    // Check whether the type of the variable matches the parameter.
    if(!matches_var_type(param.type, sym_var->type()))
    {
        failed = true;
        report(var_source, Diag::var_type_mismatch);
    }

    // An array variable name which is not followed by a subscript
    // behaves as if its zero-indexed element is referenced.
    if(!subscript && sym_var->is_array())
    {
        subscript = VarSubscript{
                .value = var_name, .source = var_source, .literal = 0};
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
               || subscript->literal >= sym_var->dimensions().value_or(1))
            {
                failed = true;
                report(subscript->source, Diag::subscript_out_of_range);
                subscript->literal = 0; // recover
            }
        }
    }

    if(subscript && !subscript->literal)
    {
        sym_subscript = lookup_var_lvar(subscript->value);
        if(!sym_subscript)
        {
            failed = true;
            report(subscript->source, Diag::undefined_variable);
            subscript->literal = 0; // recover
        }
        else if(sym_subscript->type() != SymbolTable::VarType::INT)
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
           && var_entity_type(*sym_var) == EntityId{0})
        {
            set_var_entity_type(*sym_var, param.entity_type);
        }

        if(var_entity_type(*sym_var) != param.entity_type)
        {
            failed = true;
            report(var_source, Diag::var_entity_type_mismatch);
        }
    }

    if(failed)
        return nullptr;
    else if(sym_subscript)
        return SemaIR::create_variable(*sym_var, *sym_subscript, arg_source,
                                       allocator);
    else if(subscript && subscript->literal)
        return SemaIR::create_variable(*sym_var, *subscript->literal,
                                       arg_source, allocator);
    else
        return SemaIR::create_variable(*sym_var, arg_source, allocator);
}

auto Sema::validate_hardcoded_command(const SemaIR::Command& command) -> bool
{
    if(&command.def() == command_script_name)
        return validate_script_name(command);
    if(&command.def() == command_start_new_script)
        return validate_start_new_script(command);
    else if(alternator_set
            && is_alternative_command(command.def(), *alternator_set))
        return validate_set(command);
    else
        return true;
}

auto Sema::validate_set(const SemaIR::Command& command) -> bool
{
    assert(is_alternative_command(command.def(), *alternator_set));

    if(command.args().size() == 2)
    {
        const auto lhs = command.arg(0).as_var_ref();
        const auto rhs = command.arg(1).as_var_ref();
        if(lhs && rhs)
        {
            const auto lhs_entity_type = var_entity_type(lhs->def());
            const auto rhs_entity_type = var_entity_type(rhs->def());
            if(lhs_entity_type == EntityId{0} && rhs_entity_type != EntityId{0})
            {
                set_var_entity_type(lhs->def(), rhs_entity_type);
            }
            else if(lhs_entity_type != rhs_entity_type)
            {
                report(command.source(), Diag::var_entity_type_mismatch)
                        .range(command.arg(0).source())
                        .range(command.arg(1).source());
                return false;
            }
        }
    }

    return true;
}

auto Sema::validate_script_name(const SemaIR::Command& command) -> bool
{
    assert(&command.def() == command_script_name);

    if(command.args().size() == 1)
    {
        if(const auto name = command.arg(0).as_text_label())
        {
            auto it = std::find(seen_script_names.begin(),
                                seen_script_names.end(), *name);
            if(it != seen_script_names.end())
            {
                report(command.arg(0).source(), Diag::duplicate_script_name);
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

auto Sema::validate_start_new_script(const SemaIR::Command& command) -> bool
{
    assert(&command.def() == command_start_new_script);

    if(!command.args().empty())
    {
        if(const auto* target_label = command.arg(0).as_label())
        {
            if(target_label->scope() == SymbolTable::global_scope)
            {
                report(command.arg(0).source(),
                       Diag::target_label_not_within_scope);
                return false;
            }

            if(!validate_target_scope_vars(command.args().begin() + 1,
                                           command.args().end(),
                                           target_label->scope()))
            {
                return false;
            }
        }
    }

    return true;
}

auto Sema::validate_target_scope_vars(SemaIR::ArgumentView::iterator begin,
                                      SemaIR::ArgumentView::iterator end,
                                      SymbolTable::ScopeId target_scope_id)
        -> bool
{
    assert(target_scope_id != SymbolTable::global_scope);

    const size_t num_target_vars = end - begin;
    bool failed = false;

    if(num_target_vars == 0)
        return true;

    const auto* const target_scope_timera = symrepo->lookup_var(
            varname_timera, target_scope_id);
    const auto* const target_scope_timerb = symrepo->lookup_var(
            varname_timerb, target_scope_id);

    // Use a temporary buffer in the arena. This will be unused memory after
    // this function returns, but it's no big deal. It only happens for
    // START_NEW_SCRIPT alike commands and the allocation size is proportional
    // to the number of arguments passed.
    const auto** target_vars
            = allocator.allocate_object<const SymbolTable::Variable*>(
                    num_target_vars);
    std::uninitialized_fill_n(target_vars, num_target_vars, nullptr);

    for(auto& lvar : symrepo->scope(target_scope_id))
    {
        // Do not use timers as target variables.
        if(&lvar == target_scope_timera || &lvar == target_scope_timerb)
            continue;

        if(lvar.id() < num_target_vars)
            target_vars[lvar.id()] = &lvar;
    }

    for(size_t i = 0; i < num_target_vars; ++i)
    {
        const auto& arg = begin[i];
        const SymbolTable::Variable* target_var = target_vars[i];

        if(target_var == nullptr)
        {
            failed = true;
            report(arg.source(), Diag::target_scope_not_enough_vars);
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
            if(arg.pun_as_int())
            {
                if(target_vars[i]->type() != SymbolTable::VarType::INT)
                {
                    failed = true;
                    report(arg.source(), Diag::target_var_type_mismatch);
                }
            }
            else if(arg.pun_as_float())
            {
                if(target_vars[i]->type() != SymbolTable::VarType::FLOAT)
                {
                    failed = true;
                    report(arg.source(), Diag::target_var_type_mismatch);
                }
            }
            else if(arg.as_text_label())
            {
                if(target_vars[i]->type() != SymbolTable::VarType::TEXT_LABEL)
                {
                    failed = true;
                    report(arg.source(), Diag::target_var_type_mismatch);
                }
            }
            else if(const auto var_ref = arg.as_var_ref())
            {
                const auto& source_var = var_ref->def();
                if(target_vars[i]->type() != source_var.type())
                {
                    failed = true;
                    report(arg.source(), Diag::target_var_type_mismatch);
                }
                else if(var_entity_type(*target_vars[i]) == EntityId{0}
                        && var_entity_type(source_var) != EntityId{0})
                {
                    set_var_entity_type(*target_var,
                                        var_entity_type(source_var));
                }
                else if(var_entity_type(*target_var)
                        != var_entity_type(source_var))
                {
                    failed = true;
                    report(arg.source(), Diag::target_var_entity_type_mismatch);
                }
            }
            else
            {
                failed = true;
                report(arg.source(), Diag::internal_compiler_error);
            }
        }
    }

    return !failed;
}

void Sema::declare_label(const ParserIR::LabelDef& label_def)
{
    const auto scope_id = current_scope == no_local_scope
                                  ? SymbolTable::global_scope
                                  : current_scope;
    if(auto [_, inserted] = symrepo->insert_label(label_def.name(), scope_id,
                                                  label_def.source());
       !inserted)
    {
        report(label_def.source(), Diag::duplicate_label);
    }
}

void Sema::declare_variable(const ParserIR::Command& command,
                            SymbolTable::ScopeId scope_id,
                            SymbolTable::VarType type)
{
    for(auto& arg : command.args())
    {
        SymbolTable::ScopeId var_scope_id = scope_id;

        if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
        {
            report(arg.source(), Diag::expected_identifier);
            continue;
        }

        auto [var_name, var_source, subscript] = parse_var_ref(
                *arg.as_identifier(), arg.source());

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

        if(var_scope_id == no_local_scope)
        {
            report(arg.source(), Diag::var_decl_outside_of_scope);
            var_scope_id = SymbolTable::global_scope; // recover
        }

        std::optional<int32_t> subscript_literal;
        if(subscript)
            subscript_literal = subscript->literal;

        if(var_name == varname_timera || var_name == varname_timerb)
        {
            report(var_source, Diag::duplicate_var_timer);
        }
        else if(auto [var, inserted] = symrepo->insert_var(
                        var_name, var_scope_id, type, subscript_literal,
                        arg.source());
                !inserted)
        {
            if(var_scope_id == SymbolTable::global_scope)
                report(var_source, Diag::duplicate_var_global);
            else
                report(var_source, Diag::duplicate_var_in_scope);
        }
    }
}

auto Sema::report(SourceLocation source, Diag message) -> DiagnosticBuilder
{
    this->report_count++;
    return diag->report(source, message);
}

auto Sema::report(SourceRange source, Diag message) -> DiagnosticBuilder
{
    return report(source.begin, message).range(source);
}

auto Sema::lookup_var_lvar(std::string_view name) const
        -> const SymbolTable::Variable*
{
    if(const auto* gvar = symrepo->lookup_var(name, SymbolTable::global_scope))
        return gvar;

    if(current_scope != no_local_scope)
    {
        if(const auto* lvar = symrepo->lookup_var(name, current_scope))
            return lvar;
    }

    return nullptr;
}

auto Sema::var_entity_type(const SymbolTable::Variable& var) const
        -> CommandTable::EntityId
{
    const auto scope_index = to_integer(var.scope());
    assert(scope_index < vars_entity_type.size());
    assert(var.id() < vars_entity_type[scope_index].size());
    return vars_entity_type[scope_index][var.id()];
}

void Sema::set_var_entity_type(const SymbolTable::Variable& var,
                               CommandTable::EntityId entity_type)
{
    const auto scope_index = to_integer(var.scope());
    assert(to_integer(var.scope()) < vars_entity_type.size());
    assert(var.id() < vars_entity_type[scope_index].size());
    vars_entity_type[scope_index][var.id()] = entity_type;
}

auto Sema::find_defaultmodel_constant(std::string_view name) const
        -> const CommandTable::ConstantDef*
{
    if(this->defaultmodel_enum)
        return cmdman->find_constant(*defaultmodel_enum, name);
    return nullptr;
}

auto Sema::is_object_param(const CommandTable::ParamDef& param) const -> bool
{
    return this->model_enum && param.enum_type == *model_enum;
}

auto Sema::is_gvar_param(ParamType param_type) const -> bool
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
        case ParamType::INT:
        case ParamType::FLOAT:
        case ParamType::LVAR_INT:
        case ParamType::LVAR_FLOAT:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::INPUT_INT:
        case ParamType::INPUT_FLOAT:
        case ParamType::OUTPUT_INT:
        case ParamType::OUTPUT_FLOAT:
        case ParamType::LABEL:
        case ParamType::TEXT_LABEL:
        case ParamType::STRING:
        case ParamType::LVAR_INT_OPT:
        case ParamType::LVAR_FLOAT_OPT:
        case ParamType::LVAR_TEXT_LABEL_OPT:
        case ParamType::INPUT_OPT:
            return false;
    }
    assert(false);
}

auto Sema::is_lvar_param(ParamType param_type) const -> bool
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
        case ParamType::INT:
        case ParamType::FLOAT:
        case ParamType::VAR_INT:
        case ParamType::VAR_FLOAT:
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::INPUT_INT:
        case ParamType::INPUT_FLOAT:
        case ParamType::OUTPUT_INT:
        case ParamType::OUTPUT_FLOAT:
        case ParamType::LABEL:
        case ParamType::TEXT_LABEL:
        case ParamType::STRING:
        case ParamType::VAR_INT_OPT:
        case ParamType::VAR_FLOAT_OPT:
        case ParamType::VAR_TEXT_LABEL_OPT:
        case ParamType::INPUT_OPT:
            return false;
    }
    assert(false);
}

auto Sema::matches_var_type(ParamType param_type,
                            SymbolTable::VarType var_type) const -> bool
{
    switch(param_type)
    {
        case ParamType::VAR_INT:
        case ParamType::LVAR_INT:
        case ParamType::VAR_INT_OPT:
        case ParamType::LVAR_INT_OPT:
        case ParamType::INPUT_INT:
        case ParamType::OUTPUT_INT:
            return (var_type == SymbolTable::VarType::INT);
        case ParamType::VAR_FLOAT:
        case ParamType::LVAR_FLOAT:
        case ParamType::VAR_FLOAT_OPT:
        case ParamType::LVAR_FLOAT_OPT:
        case ParamType::INPUT_FLOAT:
        case ParamType::OUTPUT_FLOAT:
            return (var_type == SymbolTable::VarType::FLOAT);
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::VAR_TEXT_LABEL_OPT:
        case ParamType::LVAR_TEXT_LABEL_OPT:
        case ParamType::TEXT_LABEL:
            return (var_type == SymbolTable::VarType::TEXT_LABEL);
        case ParamType::INPUT_OPT:
            return (var_type == SymbolTable::VarType::INT
                    || var_type == SymbolTable::VarType::FLOAT);
        case ParamType::INT:
        case ParamType::FLOAT:
        case ParamType::LABEL:
        case ParamType::STRING:
            return false;
    }
    assert(false);
}

auto Sema::is_alternative_command(const CommandTable::CommandDef& command_def,
                                  const CommandTable::AlternatorDef& from) const
        -> bool
{
    auto it = std::find_if(from.begin(), from.end(),
                           [&](const auto& alternative) {
                               return &command_def == &alternative.command();
                           });
    return (it != from.end());
}

auto Sema::is_matching_alternative(const ParserIR::Command& command,
                                   const CommandTable::CommandDef& alternative)
        -> bool
{
    // Alternators do not admit optional arguments, so it's
    // all good to perform this check beforehand.
    if(command.num_args() != alternative.num_min_params())
        return false;

    for(size_t i = 0, acount = command.num_args(); i < acount; ++i)
    {
        const auto& arg = command.arg(i);
        const auto& param = alternative.param(i);

        // The global string constants have higher precedence when checking for
        // anything that is an identifier, and that shall only match with INTs.
        if(param.type != ParamType::INT
           && arg.type() == ParserIR::Argument::Type::IDENTIFIER
           && cmdman->find_constant(CommandTable::global_enum,
                                    *arg.as_identifier()))
        {
            return false;
        }

        switch(param.type)
        {
            case ParamType::INT:
            {
                if(arg.type() == ParserIR::Argument::Type::IDENTIFIER)
                {
                    if(!cmdman->find_constant(CommandTable::global_enum,
                                              *arg.as_identifier()))
                        return false;
                }
                else if(arg.type() != ParserIR::Argument::Type::INT)
                {
                    return false;
                }
                break;
            }
            case ParamType::FLOAT:
            {
                if(arg.type() != ParserIR::Argument::Type::FLOAT)
                    return false;
                break;
            }
            case ParamType::VAR_INT:
            case ParamType::VAR_FLOAT:
            case ParamType::VAR_TEXT_LABEL:
            {
                if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
                    return false;

                const auto arg_ident = *arg.as_identifier();
                auto [var_name, var_source, _] = parse_var_ref(arg_ident,
                                                               arg.source());
                const auto* sym_var = symrepo->lookup_var(var_name);
                if(!sym_var || !matches_var_type(param.type, sym_var->type()))
                    return false;

                break;
            }
            case ParamType::LVAR_INT:
            case ParamType::LVAR_FLOAT:
            case ParamType::LVAR_TEXT_LABEL:
            {
                if(current_scope == no_local_scope)
                    return false;

                if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
                    return false;

                const auto arg_ident = *arg.as_identifier();
                auto [var_name, var_source, _] = parse_var_ref(arg_ident,
                                                               arg.source());
                const auto* sym_var = symrepo->lookup_var(var_name,
                                                          current_scope);
                if(!sym_var || !matches_var_type(param.type, sym_var->type()))
                    return false;

                break;
            }
            case ParamType::INPUT_INT:
            {
                if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
                    return false;

                const auto arg_ident = *arg.as_identifier();
                if(!cmdman->find_constant_any_means(arg_ident))
                    return false;

                break;
            }
            case ParamType::TEXT_LABEL:
            {
                if(arg.type() != ParserIR::Argument::Type::IDENTIFIER)
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

    assert(!identifier.empty());
    assert(!is_bracket(identifier.front()));

    const auto* const it_open = std::find_if(identifier.begin(),
                                             identifier.end(), is_bracket);
    if(it_open != identifier.end())
    {
        const auto it_open_pos = std::distance(identifier.begin(), it_open);
        if(*it_open == ']')
        {
            report(source.begin + it_open_pos, Diag::expected_word).args("[");
            // Recovery strategy: Assume *it == ']'
        }

        const auto* const it_close = std::find_if(it_open + 1, identifier.end(),
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
                    .value = identifier.substr(it_open_pos + 1, subscript_len),
                    .source = source.subrange(it_open_pos + 1, subscript_len),
                    .literal = std::nullopt};
        }
    }
    else
    {
        // There is no array subscript. The identifier is the variable name.
        var_name = identifier;
        var_source = source;
    }

    assert(!var_name.empty());
    assert(var_name.size() == var_source.size());
    assert(!subscript || !subscript->value.empty());
    assert(!subscript || subscript->value.size() == subscript->source.size());

    // We have to validate the subscript is either another identifier
    // or a positive integer literal.
    if(subscript)
    {
        const auto& subval = subscript->value;

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
                int32_t value{};

                if(auto [_, ec] = util::from_chars(&*subval.begin(),
                                                   &*subval.end(), value);
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

    return VarRef{
            .name = var_name, .source = var_source, .subscript = subscript};
}
} // namespace gta3sc::syntax

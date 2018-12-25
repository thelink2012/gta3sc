#include "charconv.hpp"
#include <gta3sc/sema.hpp>

// gta3script-specs 7fe565c767ee85fb8c99b594b3b3d280aa1b1c80

// TODO limit dim

namespace gta3sc
{
auto Sema::report(SourceRange source, Diag message) -> DiagnosticBuilder
{
    return diag->report(source.begin(), message).range(source);
}

void Sema::pass_declarations()
{
    ScopeId curr_scope = -1;

    for(auto& line : parser_ir)
    {
        if(line.label)
        {
            act_on_label_def(*line.label);
        }

        if(line.command)
        {
            if(line.command->name == "{")
            {
                assert(curr_scope == -1);
                curr_scope = symrepo->allocate_scope();
            }
            else if(line.command->name == "}")
            {
                assert(curr_scope != -1);
                curr_scope = -1;
            }
            else if(line.command->name == "VAR_INT")
            {
                act_on_var_decl(*line.command, 0, SymVariable::Type::INT);
            }
            else if(line.command->name == "LVAR_INT")
            {
                act_on_var_decl(*line.command, curr_scope,
                                SymVariable::Type::INT);
            }
            else if(line.command->name == "VAR_FLOAT")
            {
                act_on_var_decl(*line.command, 0, SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "LVAR_FLOAT")
            {
                act_on_var_decl(*line.command, curr_scope,
                                SymVariable::Type::FLOAT);
            }
            else if(line.command->name == "VAR_TEXT_LABEL")
            {
                act_on_var_decl(*line.command, 0,
                                SymVariable::Type::TEXT_LABEL);
            }
            else if(line.command->name == "LVAR_TEXT_LABEL")
            {
                act_on_var_decl(*line.command, curr_scope,
                                SymVariable::Type::TEXT_LABEL);
            }
        }
    }

    // Ensure local variables do not collide with global variables.
    for(ScopeId i = 1; i < symrepo->var_tables.size(); ++i)
    {
        for(const auto& [name, lvar] : symrepo->var_tables[i])
        {
            if(auto gvar = symrepo->lookup_var(name, 0))
            {
                report(lvar->source, Diag::duplicate_var_lvar);
            }
        }
    }
}

void Sema::pass_analyze()
{
    for(auto& line : parser_ir)
    {
        // TODO
    }
}

void Sema::act_on_label_def(const ParserIR::LabelDef& label_def)
{
    if(auto [_, inserted] = symrepo->insert_label(label_def.name,
                                                  label_def.source);
       !inserted)
    {
        report(label_def.source, Diag::duplicate_label);
    }
}

void Sema::act_on_var_decl(const ParserIR::Command& command, ScopeId scope_id_,
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

auto Sema::parse_var_ref(std::string_view identifier, SourceRange source)
        -> VarRef
{
    // subscript := '[' (variable_name | integer) ']' ;
    // variable := variable_name [ subscript ] ;

    // We need this parsing function in the semantic phase because, until this
    // point, we could not classify an identifier into either a variable or
    // something else (that something else e.g. labels could contain
    // brackets in its name).

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
            diag->report(source.begin() + it_open_pos, Diag::expected_word)
                    .args("[");
            // Recovery strategy: Assume *it == ']'
        }

        const auto it_close = std::find_if(it_open + 1, identifier.end(),
                                           is_bracket);
        const auto it_close_pos = std::distance(identifier.begin(), it_close);

        if(it_close == identifier.end() || *it_close == '[')
        {
            diag->report(source.begin() + it_close_pos, Diag::expected_word)
                    .args("]");
            // Recovery strategy: Assume *it_close == ']'
        }

        assert(it_open != identifier.begin());
        var_name = identifier.substr(0, it_open_pos);
        var_source = source.substr(0, it_open_pos);

        if(std::distance(it_open, it_close) <= 1)
        {
            diag->report(source.begin() + it_open_pos + 1,
                         Diag::expected_subscript);
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

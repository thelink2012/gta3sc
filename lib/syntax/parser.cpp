#include "charconv.hpp"
#include <gta3sc/syntax/parser.hpp>
using namespace std::literals::string_view_literals;

// grammar from https://git.io/fNxZP f1f8a9096cb7a861e410d3f208f2589737220327

// TODO parse mission start/end such that we ensure its amount of params

namespace gta3sc::syntax
{
// The parser does not know anything about commands, so we have to
// hardcode the special commands in here.
constexpr auto COMMAND_MISSION_START = "MISSION_START"sv;
constexpr auto COMMAND_MISSION_END = "MISSION_END"sv;
constexpr auto COMMAND_VAR_INT = "VAR_INT"sv;
constexpr auto COMMAND_LVAR_INT = "LVAR_INT"sv;
constexpr auto COMMAND_VAR_FLOAT = "VAR_FLOAT"sv;
constexpr auto COMMAND_LVAR_FLOAT = "LVAR_FLOAT"sv;
constexpr auto COMMAND_VAR_TEXT_LABEL = "VAR_TEXT_LABEL"sv;
constexpr auto COMMAND_LVAR_TEXT_LABEL = "LVAR_TEXT_LABEL"sv;
constexpr auto COMMAND_ANDOR = "ANDOR"sv;
constexpr auto COMMAND_GOTO = "GOTO"sv;
constexpr auto COMMAND_GOTO_IF_FALSE = "GOTO_IF_FALSE"sv;
constexpr auto COMMAND_GOTO_IF_TRUE = "GOTO_IF_TRUE"sv;
constexpr auto COMMAND_IF = "IF"sv;
constexpr auto COMMAND_IFNOT = "IFNOT"sv;
constexpr auto COMMAND_ELSE = "ELSE"sv;
constexpr auto COMMAND_ENDIF = "ENDIF"sv;
constexpr auto COMMAND_WHILE = "WHILE"sv;
constexpr auto COMMAND_WHILENOT = "WHILENOT"sv;
constexpr auto COMMAND_ENDWHILE = "ENDWHILE"sv;
constexpr auto COMMAND_REPEAT = "REPEAT"sv;
constexpr auto COMMAND_ENDREPEAT = "ENDREPEAT"sv;
constexpr auto COMMAND_GOSUB_FILE = "GOSUB_FILE"sv;
constexpr auto COMMAND_LAUNCH_MISSION = "LAUNCH_MISSION"sv;
constexpr auto COMMAND_LOAD_AND_LAUNCH_MISSION = "LOAD_AND_LAUNCH_MISSION"sv;
constexpr auto COMMAND_SET = "SET"sv;
constexpr auto COMMAND_CSET = "CSET"sv;
constexpr auto COMMAND_ABS = "ABS"sv;
constexpr auto COMMAND_ADD_THING_TO_THING = "ADD_THING_TO_THING"sv;
constexpr auto COMMAND_SUB_THING_FROM_THING = "SUB_THING_FROM_THING"sv;
constexpr auto COMMAND_MULT_THING_BY_THING = "MULT_THING_BY_THING"sv;
constexpr auto COMMAND_DIV_THING_BY_THING = "DIV_THING_BY_THING"sv;
constexpr auto COMMAND_ADD_THING_TO_THING_TIMED = "ADD_THING_TO_THING_TIMED"sv;
constexpr auto COMMAND_SUB_THING_FROM_THING_TIMED
        = "SUB_THING_FROM_THING_TIMED"sv;
constexpr auto COMMAND_IS_THING_EQUAL_TO_THING = "IS_THING_EQUAL_TO_THING"sv;
constexpr auto COMMAND_IS_THING_GREATER_THAN_THING
        = "IS_THING_GREATER_THAN_THING"sv;
constexpr auto COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING
        = "IS_THING_GREATER_OR_EQUAL_TO_THING"sv;

bool Parser::eof() const
{
    if(has_peek_token[0])
        return false;
    return scanner.eof();
}

auto Parser::source_file() const -> const SourceFile&
{
    return scanner.source_file();
}

auto Parser::diagnostics() const -> DiagnosticHandler&
{
    return scanner.diagnostics();
}

auto Parser::report(const Token& token, Diag message) -> DiagnosticBuilder
{
    return report(token.source, message);
}

auto Parser::report(SourceRange source, Diag message) -> DiagnosticBuilder
{
    return diagnostics().report(source.begin, message).range(source);
}

auto Parser::report_special_name(SourceRange source) -> DiagnosticBuilder
{
    // This method can be specialized to produce a different diagnostic
    // for each special name. Currently we produce a generic message.
    const std::string_view name = source_file().view_of(source);
    return report(source, Diag::unexpected_special_name).args(name);
}

bool Parser::is_special_name(std::string_view name, bool check_var_decl) const
{
    if(check_var_decl && is_var_decl_command(name))
        return true;
    return (name == "{" || name == "}" || name == "NOT" || name == "AND"
            || name == "OR" || name == COMMAND_IF || name == COMMAND_IFNOT
            || name == COMMAND_ELSE || name == COMMAND_ENDIF
            || name == COMMAND_WHILE || name == COMMAND_WHILENOT
            || name == COMMAND_ENDWHILE || name == COMMAND_REPEAT
            || name == COMMAND_ENDREPEAT || name == COMMAND_GOSUB_FILE
            || name == COMMAND_LAUNCH_MISSION
            || name == COMMAND_LOAD_AND_LAUNCH_MISSION
            || name == COMMAND_MISSION_START || name == COMMAND_MISSION_END);
}

bool Parser::is_var_decl_command(std::string_view name) const
{
    return name == COMMAND_VAR_INT || name == COMMAND_VAR_FLOAT
           || name == COMMAND_VAR_TEXT_LABEL || name == COMMAND_LVAR_INT
           || name == COMMAND_LVAR_FLOAT || name == COMMAND_LVAR_TEXT_LABEL;
}

bool Parser::iequal(std::string_view a, std::string_view b) const
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](unsigned char ac, unsigned char bc) {
                          if(ac >= 'a' && ac <= 'z')
                              ac -= 32; // transform into upper
                          if(bc >= 'a' && bc <= 'z')
                              bc -= 32;
                          return ac == bc;
                      });
}

bool Parser::is_relational_operator(Category category) const
{
    switch(category)
    {
        case Category::Less:
        case Category::LessEqual:
        case Category::Greater:
        case Category::GreaterEqual:
            return true;
        default:
            return false;
    }
}

auto Parser::peek(size_t n) -> std::optional<Token>
{
    assert(n < std::size(peek_tokens));

    if(!has_peek_token[n])
    {
        assert(n == 0 || has_peek_token[n - 1]);

        if(n != 0 && peek_tokens[n - 1]
           && peek_tokens[n - 1]->category == Category::EndOfLine)
        {
            has_peek_token[n] = true;
            peek_tokens[n] = peek_tokens[n - 1];
        }
        else
        {
            has_peek_token[n] = true;
            peek_tokens[n] = scanner.next();
        }
    }

    return peek_tokens[n];
}

bool Parser::is_peek(Category category, size_t n)
{
    return peek(n) && peek(n)->category == category;
}

bool Parser::is_peek(Category category, std::string_view lexeme, size_t n)
{
    return is_peek(category, n) && iequal(scanner.spelling(*peek(n)), lexeme);
}

auto Parser::peek_expression_type() -> std::optional<Category>
{
    if(is_peek(Category::PlusPlus) || is_peek(Category::MinusMinus))
        return peek()->category;

    // We want the next token ignoring whitespaces.
    const auto opos = is_peek(Category::Whitespace, 1) ? 2 : 1;

    switch(peek(opos) ? peek(opos)->category : Category::Word)
    {
        case Category::Equal:
        case Category::EqualHash:
        case Category::PlusEqual:
        case Category::MinusEqual:
        case Category::StarEqual:
        case Category::SlashEqual:
        case Category::PlusEqualAt:
        case Category::MinusEqualAt:
        case Category::PlusPlus:
        case Category::MinusMinus:
        case Category::Less:
        case Category::LessEqual:
        case Category::Greater:
        case Category::GreaterEqual:
            return peek(opos)->category;
        default:
            return std::nullopt;
    }
}

auto Parser::consume() -> std::optional<Token>
{
    if(!has_peek_token[0])
        return scanner.next();

    assert(has_peek_token[0] == true);
    auto eat_token = peek_tokens[0];

    // Shift the peek tokens one position to the left.
    size_t n = 1;
    for(; n < size(peek_tokens) && has_peek_token[n]; ++n)
        peek_tokens[n - 1] = peek_tokens[n];

    has_peek_token[n - 1] = false;

    return eat_token;
}

auto Parser::consume_filename() -> std::optional<Token>
{
    // Due to the hack in `consume_whitespace` we need this
    // other hack here. If not for that hack, we could never
    // have a peek token here.
    if(has_peek_token[0] && is_peek(Category::EndOfLine))
    {
        report(*peek(), Diag::expected_identifier);
        return std::nullopt;
    }

    assert(has_peek_token[0] == false);
    return scanner.next_filename();
}

auto Parser::consume(Category category) -> std::optional<Token>
{
    auto token = consume();
    if(!token)
        return std::nullopt;

    if(token->category != category)
    {
        report(*token, Diag::expected_token).args(category);
        return std::nullopt;
    }

    return token;
}

auto Parser::consume_word(std::string_view lexeme) -> std::optional<Token>
{
    auto token = consume(Category::Word);
    if(!token)
        return std::nullopt;

    if(!iequal(scanner.spelling(*token), lexeme))
    {
        report(*token, Diag::expected_word).args(lexeme);
        return std::nullopt;
    }

    return token;
}

auto Parser::consume_whitespace() -> std::optional<Token>
{
    // This is necessary to make diagnostics more helpful.
    // Consider "IF \n". A diagnostic without the following check
    // would ask for a missing whitespace, when in fact it should
    // be asking for a missing command.
    if(is_peek(Category::EndOfLine))
        return *peek();
    return consume(Category::Whitespace);
}

auto Parser::consume_command() -> std::optional<Token>
{
    auto token = consume();
    if(!token)
        return std::nullopt;

    if(token->category != Category::Word)
    {
        report(*token, Diag::expected_command);
        return std::nullopt;
    }

    return token;
}

void Parser::skip_current_line()
{
    while(true)
    {
        // The scanner is guaranted to return a new line at some point.
        auto token = this->consume();
        if(token && token->category == Category::EndOfLine)
            break;
    }
}

bool Parser::is_digit(char c) const
{
    // digit := '0'..'9' ;
    return c >= '0' && c <= '9';
}

bool Parser::is_integer(std::string_view lexeme) const
{
    // integer := ['-'] digit {digit} ;

    size_t num_digits = 0;
    size_t char_pos = 0;

    for(auto c : lexeme)
    {
        ++char_pos;
        if(c == '-' && char_pos == 1)
            continue;
        else if(is_digit(c))
            ++num_digits;
        else
            return false;
    }

    return num_digits > 0;
}

bool Parser::is_float(std::string_view lexeme) const
{
    // floating_form1 := '.' digit { digit | '.' | 'F' } ;
    // floating_form2 := digit { digit } ('.' | 'F') { digit | '.' | 'F' } ;
    // floating := ['-'] (floating_form1 | floating_form2) ;

    auto it = lexeme.begin();

    if(lexeme.size() >= 2 && *it == '-')
        ++it;

    if(*it == '.') // floating_form1
    {
        ++it;
        if(it == lexeme.end() || !is_digit(*it))
            return false;
        ++it;
    }
    else if(*it >= '0' && *it <= '9') // floating_form2
    {
        ++it;
        it = std::find_if_not(it, lexeme.end(), // {digit}
                              [this](char c) { return is_digit(c); });
        if(it == lexeme.end() || (*it != '.' && *it != 'f' && *it != 'F'))
            return false;
        ++it;
    }
    else // not a floating point
    {
        return false;
    }

    // Skip the final, common part: {digit | '.' | 'F'}
    it = std::find_if_not(it, lexeme.end(), [this](char c) {
        return (c == '.' || c == 'f' || c == 'F' || is_digit(c));
    });

    return it == lexeme.end();
}

bool Parser::is_identifier(std::string_view lexeme) const
{
    // identifier := ('$' | 'A'..'Z') {token_char} ;
    //
    // Constraints:
    // An identifier should not end with a `:` character.

    if(lexeme.size() >= 1)
    {
        const auto front = lexeme.front();
        const auto back = lexeme.back();

        if(front == '$' || (front >= 'A' && front <= 'Z')
           || (front >= 'a' && front <= 'z'))
        {
            return back != ':';
        }
    }

    return false;
}

auto Parser::parse_command(bool is_if_line, bool not_flag)
        -> std::optional<arena_ptr<ParserIR>>
{
    // command_name := token_char {token_char} ;
    // command := command_name { sep argument } ;
    // FOLLOW(command) = {eol, sep 'GOTO'}

    auto token = consume_command();
    if(!token)
        return std::nullopt;

    ParserIR::Builder builder(*arena);
    builder.not_flag(not_flag);
    builder.command(scanner.spelling(*token), token->source);

    while(!is_peek(Category::EndOfLine))
    {
        if(is_if_line && is_peek(Category::Whitespace, 0)
           && is_peek(Category::Word, "GOTO", 1)
           && is_peek(Category::Whitespace, 2) && is_peek(Category::Word, 3)
           && is_peek(Category::EndOfLine, 4))
            break;

        if(!consume_whitespace())
            return std::nullopt;

        auto arg = parse_argument();
        if(!arg)
            return std::nullopt;

        builder.arg(*arg);
    }

    assert(is_peek(Category::EndOfLine) || is_peek(Category::Whitespace));

    return std::move(builder).build();
}

auto Parser::parse_argument()
        -> std::optional<arena_ptr<const ParserIR::Argument>>
{
    // argument := integer
    //             | floating
    //             | identifier
    //             | string_literal ;

    auto token = consume();
    if(!token)
        return std::nullopt;

    const auto lexeme = scanner.spelling(*token);

    if(token->category == Category::String)
    {
        auto string = lexeme;
        string.remove_prefix(1);
        string.remove_suffix(1);
        return ParserIR::create_string(string, token->source, arena);
    }
    else if(token->category == Category::Word && is_integer(lexeme))
    {
        using namespace gta3sc::utils;
        int32_t value{};

        if(auto [_, ec] = from_chars(&*lexeme.begin(), &*lexeme.end(), value);
           ec != std::errc())
        {
            assert(ec == std::errc::result_out_of_range);
            report(*token, Diag::integer_literal_too_big);
            return std::nullopt;
        }

        return ParserIR::create_integer(value, token->source, arena);
    }
    else if(token->category == Category::Word && is_float(lexeme))
    {
        using namespace gta3sc::utils;
        float value{};

        if(auto [_, ec] = from_chars(&*lexeme.begin(), &*lexeme.end(), value,
                                     chars_format::fixed);
           ec != std::errc())
        {
            assert(ec == std::errc::result_out_of_range);
            report(*token, Diag::float_literal_too_big);
            return std::nullopt;
        }

        return ParserIR::create_float(value, token->source, arena);
    }
    else if(token->category == Category::Word && is_identifier(lexeme))
    {
        return ParserIR::create_identifier(lexeme, token->source, arena);
    }
    else
    {
        report(*token, Diag::expected_argument);
        return std::nullopt;
    }
}

auto Parser::parse_main_script_file() -> std::optional<LinkedIR<ParserIR>>
{
    // main_script_file := {statement} ;
    return parse_statement_list({});
}

auto Parser::parse_main_extension_file() -> std::optional<LinkedIR<ParserIR>>
{
    // main_script_file := {statement} ;
    return parse_statement_list({});
}

auto Parser::parse_subscript_file() -> std::optional<LinkedIR<ParserIR>>
{
    // subscript_file := 'MISSION_START' eol
    //                  {statement}
    //                  [label_prefix] 'MISSION_END' eol
    //                  {statement} ;

    if(!ensure_mission_start_at_top_of_file())
        return std::nullopt;

    auto mission_start = parse_command();
    if(!mission_start)
        return std::nullopt;

    if(const auto mission_start_command = (*mission_start)->command;
       mission_start_command->args.size() != 0)
    {
        report(mission_start_command->source, Diag::too_many_arguments);
        return std::nullopt;
    }

    auto body_stms = parse_statement_list("MISSION_END");
    if(!body_stms)
        return std::nullopt;

    if(const auto mission_end_command = body_stms->back().command;
       mission_end_command->args.size() != 0)
    {
        report(mission_end_command->source, Diag::too_many_arguments);
        return std::nullopt;
    }

    auto rest_stms = parse_statement_list({});
    if(!rest_stms)
        return std::nullopt;

    LinkedIR<ParserIR> linked_ir;
    linked_ir.push_front(**mission_start);
    linked_ir.splice_back(std::move(*body_stms));
    linked_ir.splice_back(std::move(*rest_stms));
    return linked_ir;
}

auto Parser::parse_mission_script_file() -> std::optional<LinkedIR<ParserIR>>
{
    // mission_script_file := subscript_file ;
    //
    // A mission script file has the same structure as of a subscript file.
    return parse_subscript_file();
}

auto Parser::parse_statement(bool allow_special_name)
        -> std::optional<LinkedIR<ParserIR>>
{
    // statement := labeled_statement
    //            | embedded_statement ;
    //
    // label_def := identifier ':' ;
    // label_prefix := label_def sep ;
    //
    // labeled_statement := label_prefix embedded_statement
    //                    | label_def empty_statement ;
    //
    // empty_statement := eol ;
    //

    arena_ptr<const ParserIR::LabelDef> label{};

    if(is_peek(Category::Word) && scanner.spelling(*peek()).back() == ':')
    {
        auto label_def = *consume();
        auto label_name = scanner.spelling(label_def);
        label_name.remove_suffix(1);

        if(!is_identifier(label_name))
        {
            report(label_def, Diag::expected_identifier);
            return std::nullopt;
        }

        if(!is_peek(Category::EndOfLine))
        {
            if(!consume_whitespace())
                return std::nullopt;
        }

        label = ParserIR::LabelDef::create(label_name, label_def.source, arena);
    }

    auto linked_stmts = parse_embedded_statement(allow_special_name);
    if(!linked_stmts)
        return std::nullopt;

    if(label)
    {
        if(linked_stmts->empty())
        {
            linked_stmts->push_back(*ParserIR::create(label, nullptr, arena));
        }
        else
        {
            assert(linked_stmts->front().label == nullptr);
            const auto command = linked_stmts->front().command;
            linked_stmts->replace(linked_stmts->begin(),
                                  *ParserIR::create(label, command, arena));
        }
    }

    return linked_stmts;
}

auto Parser::parse_statement_list(
        std::initializer_list<std::string_view> stop_when)
        -> std::optional<LinkedIR<ParserIR>>
{
    LinkedIR<ParserIR> linked_stms;

    while(!eof())
    {
        // Call `parse_statement` with the error handling for special names
        // at the top level (i.e. for `command_statement` only) disabled
        // because the `stop_when` commands are usually special names.
        auto stmt_list = parse_statement(true);
        if(!stmt_list)
            return std::nullopt;

        // Command statements are characterized by a single command in the IR.
        if(!stmt_list->empty()
           && std::next(stmt_list->begin()) == stmt_list->end())
        {
            if(auto command = stmt_list->front().command)
            {
                for(auto& name : stop_when)
                {
                    if(command->name == name)
                    {
                        linked_stms.splice_back(*std::move(stmt_list));
                        return linked_stms;
                    }
                }

                // Since the special name checking was disabled, we now
                // have to make sure we do not allow any other special name
                // other than the ones in `stop_when`.
                if(is_special_name(command->name, false))
                {
                    report_special_name(command->source);
                    return std::nullopt;
                }
            }
        }

        linked_stms.splice_back(*std::move(stmt_list));
    }

    if(stop_when.size() == 0)
    {
        return linked_stms;
    }
    else if(stop_when.size() == 1)
    {
        diagnostics()
                .report(scanner.location(), Diag::expected_word)
                .args(*stop_when.begin());
        return std::nullopt;
    }
    else
    {
        diagnostics()
                .report(scanner.location(), Diag::expected_words)
                .args(std::vector(stop_when));
        return std::nullopt;
    }
}

auto Parser::parse_statement_list(std::string_view name)
        -> std::optional<LinkedIR<ParserIR>>
{
    return parse_statement_list({name});
}

auto Parser::parse_embedded_statement(bool allow_special_name)
        -> std::optional<LinkedIR<ParserIR>>
{
    // embedded_statement := empty_statement
    //                      | command_statement
    //                      | expression_statement
    //                      | scope_statement
    //                      | var_statement
    //                      | if_statement
    //                      | ifnot_statement
    //                      | if_goto_statement
    //                      | ifnot_goto_statement
    //                      | while_statement
    //                      | whilenot_statement
    //                      | repeat_statement
    //                      | require_statement ;
    //
    // empty_statement := eol ;
    //
    // command_statement := command eol ;
    //
    // expression_statement := assignment_expression eol
    //                       | conditional_expression eol ;
    //
    // command_var_name := 'VAR_INT'
    //                     | 'LVAR_INT'
    //                     | 'VAR_FLOAT'
    //                     | 'LVAR_FLOAT' ;
    // command_var_param := sep identifier ;
    //
    // var_statement := command_var_name command_var_param {command_var_param} eol ;
    //

    if(is_peek(Category::EndOfLine))
    {
        consume();
        return LinkedIR<ParserIR>();
    }
    else if(is_peek(Category::Word, COMMAND_GOSUB_FILE)
            || is_peek(Category::Word, COMMAND_LAUNCH_MISSION)
            || is_peek(Category::Word, COMMAND_LOAD_AND_LAUNCH_MISSION))
    {
        if(auto require_ir = parse_require_statement())
            return LinkedIR<ParserIR>({*require_ir});

        return std::nullopt;
    }
    else if(auto category = peek_expression_type())
    {
        auto expr_ir = std::optional<LinkedIR<ParserIR>>();

        if(is_relational_operator(*category))
            expr_ir = parse_conditional_expression();
        else
            expr_ir = parse_assignment_expression();

        if(!expr_ir)
            return std::nullopt;

        if(!consume(Category::EndOfLine))
            return std::nullopt;

        return expr_ir;
    }
    else if(is_peek(Category::Word, "{"))
    {
        return parse_scope_statement();
    }
    else if(is_peek(Category::Word, "IF"))
    {
        return parse_if_statement();
    }
    else if(is_peek(Category::Word, "IFNOT"))
    {
        return parse_ifnot_statement();
    }
    else if(is_peek(Category::Word, "WHILE"))
    {
        return parse_while_statement();
    }
    else if(is_peek(Category::Word, "WHILENOT"))
    {
        return parse_whilenot_statement();
    }
    else if(is_peek(Category::Word, "REPEAT"))
    {
        return parse_repeat_statement();
    }
    else
    {
        if(auto ir = parse_command())
        {
            if(!allow_special_name
               && is_special_name((*ir)->command->name, false))
            {
                report_special_name((*ir)->command->source);
                return std::nullopt;
            }

            if(const auto command = (*ir)->command;
               is_var_decl_command(command->name) && command->args.size() == 0)
            {
                report(command->source, Diag::too_few_arguments);
                return std::nullopt;
            }

            if(consume(Category::EndOfLine))
                return LinkedIR<ParserIR>({*ir});
        }
        return std::nullopt;
    }
}

auto Parser::parse_scope_statement() -> std::optional<LinkedIR<ParserIR>>
{
    // scope_statement := '{' eol
    //                    {statement}
    //                    [label_prefix] '}' eol ;
    //
    // Constraints:
    // Lexical scopes cannot be nested.

    if(!is_peek(Category::Word, "{"))
    {
        consume_word("{"); // produces a diagnostic
        return std::nullopt;
    }

    auto open_command = parse_command();
    if(!open_command)
        return std::nullopt;
    if(!consume(Category::EndOfLine))
        return std::nullopt;

    if(this->in_lexical_scope)
    {
        report((*open_command)->command->source, Diag::cannot_nest_scopes);
        return std::nullopt;
    }

    this->in_lexical_scope = true;

    auto linked_stmts = parse_statement_list("}");
    if(!linked_stmts)
        return std::nullopt;

    this->in_lexical_scope = false;

    linked_stmts->push_front(**open_command);
    return linked_stmts;
}

auto Parser::parse_conditional_element(bool is_if_line)
        -> std::optional<arena_ptr<ParserIR>>
{
    // conditional_element := ['NOT' sep] (command | conditional_expression) ;

    bool not_flag = false;

    if(is_peek(Category::Word, "NOT"))
    {
        if(!consume() || !consume_whitespace())
            return std::nullopt;
        not_flag = true;
    }

    auto ir = std::optional<arena_ptr<ParserIR>>();
    if(peek_expression_type())
    {
        if(auto linked = parse_conditional_expression(is_if_line, not_flag))
        {
            // Expressions in conditional context can only have a single
            // command.
            assert(!linked->empty()
                   && std::next(linked->begin()) == linked->end());
            ir = &linked->front();
            linked->erase(linked->begin()); // Safe, list is intrusive.
        }
        else
            return std::nullopt;
    }
    else
    {
        if(ir = parse_command(is_if_line, not_flag); !ir)
            return std::nullopt;
        if(is_special_name((*ir)->command->name, true))
        {
            report_special_name((*ir)->command->source);
            return std::nullopt;
        }
    }

    return ir;
}

auto Parser::parse_conditional_list()
        -> std::pair<std::optional<LinkedIR<ParserIR>>, int32_t>
{
    // conditional_list := conditional_element eol
    //                     ({and_conditional_stmt} | {or_conditional_stmt}) ;

    auto op_cond0 = parse_conditional_element();
    if(!op_cond0)
        return {std::nullopt, 0};
    if(!consume(Category::EndOfLine))
        return {std::nullopt, 0};
    return parse_conditional_list(*op_cond0);
}

auto Parser::parse_conditional_list(arena_ptr<ParserIR> op_cond0)
        -> std::pair<std::optional<LinkedIR<ParserIR>>, int32_t>
{
    // and_conditional_stmt := 'AND' sep conditional_element eol ;
    // or_conditional_stmt := 'OR' sep conditional_element eol ;
    //
    // conditional_list := conditional_element eol
    //                     ({and_conditional_stmt} | {or_conditional_stmt}) ;

    // This method is parsing the AND/OR part of the conditional_list.

    assert(op_cond0 && op_cond0->command);

    auto andor_list = LinkedIR<ParserIR>();
    andor_list.push_back(*op_cond0);

    size_t num_conds = 1;
    int32_t andor_count = 0;

    if(is_peek(Category::Word, "AND") || is_peek(Category::Word, "OR"))
    {
        const auto is_and = is_peek(Category::Word, "AND");
        const auto andor_prefix = is_and ? "AND" : "OR";
        const auto anti_prefix = is_and ? "OR" : "AND";

        while(is_peek(Category::Word, andor_prefix))
        {
            if(!consume() || !consume_whitespace())
                return {std::nullopt, 0};

            auto op_elem = parse_conditional_element();
            if(!op_elem)
                return {std::nullopt, 0};

            if(!consume(Category::EndOfLine))
                return {std::nullopt, 0};

            andor_list.push_back(**op_elem);
            ++num_conds;
        }

        if(is_peek(Category::Word, anti_prefix))
        {
            report(peek()->source, Diag::cannot_mix_andor);
            return {std::nullopt, 0};
        }

        andor_count = is_and ? num_conds - 1 : 20 + num_conds - 1;
    }

    // The runtime has a soft limit of 6 conditions per list.
    // Unfortunately we cannot ignore this limit during the
    // parsing phrase because the generated IL for ANDOR has
    // this limitation embedded in its first parameter.
    if(num_conds > 6)
    {
        report(andor_list.back().command->source, Diag::too_many_conditions);
        return {std::nullopt, 0};
    }

    return {std::move(andor_list), andor_count};
}

auto Parser::parse_if_statement() -> std::optional<LinkedIR<ParserIR>>
{
    return parse_if_statement_detail(false);
}

auto Parser::parse_ifnot_statement() -> std::optional<LinkedIR<ParserIR>>
{
    return parse_if_statement_detail(true);
}

auto Parser::parse_if_statement_detail(bool is_ifnot)
        -> std::optional<LinkedIR<ParserIR>>
{
    // if_statement := 'IF' sep conditional_list
    //                 {statement}
    //                 [[label_prefix] 'ELSE' eol
    //                 {statement}]
    //                 [label_prefix] 'ENDIF' eol ;
    //
    // if_goto_statement := 'IF' sep conditional_element sep 'GOTO' sep
    //                      identifier eol ;
    //
    // ifnot_statement := 'IFNOT' sep conditional_list
    //                 {statement}
    //                 [[label_prefix] 'ELSE' eol
    //                 {statement}]
    //                 [label_prefix] 'ENDIF' eol ;
    //
    // ifnot_goto_statement := 'IFNOT' sep conditional_element sep 'GOTO' sep
    //                          identifier eol;
    //

    const auto if_command = is_ifnot ? COMMAND_IFNOT : COMMAND_IF;
    const auto if_true_command = is_ifnot ? COMMAND_GOTO_IF_FALSE
                                          : COMMAND_GOTO_IF_TRUE;

    auto if_token = consume_word(if_command);
    if(!if_token)
        return std::nullopt;

    if(!consume_whitespace())
        return std::nullopt;

    auto op_cond0 = parse_conditional_element(true);
    if(!op_cond0)
        return std::nullopt;

    const auto src_info = if_token->source;

    if(is_peek(Category::Whitespace))
    {
        if(!consume() || !consume_word("GOTO") || !consume_whitespace())
            return std::nullopt;

        auto arg_label = parse_argument();
        if(!arg_label)
            return std::nullopt;

        if(!consume(Category::EndOfLine))
            return std::nullopt;

        auto linked = LinkedIR<ParserIR>();
        linked.push_back(*ParserIR::Builder(*arena)
                                  .command(COMMAND_ANDOR, src_info)
                                  .arg_int(0, src_info)
                                  .build());
        linked.push_back(**op_cond0);
        linked.push_back(*ParserIR::Builder(*arena)
                                  .command(if_true_command, src_info)
                                  .arg(*arg_label)
                                  .build());
        return linked;
    }
    else
    {
        if(!consume(Category::EndOfLine))
            return std::nullopt;

        auto [andor_list, andor_count] = parse_conditional_list(*op_cond0);
        if(!andor_list)
            return std::nullopt;

        auto body_stms = parse_statement_list({"ELSE", "ENDIF"});
        if(!body_stms)
            return std::nullopt;

        if(const auto else_command = body_stms->back().command;
           else_command->name == COMMAND_ELSE)
        {
            if(else_command->args.size() != 0)
            {
                report(else_command->source, Diag::too_many_arguments);
                return std::nullopt;
            }

            auto else_stms = parse_statement_list("ENDIF");
            if(!else_stms)
                return std::nullopt;

            body_stms->splice_back(*std::move(else_stms));
        }

        if(const auto endif_command = body_stms->back().command;
           endif_command->args.size() != 0)
        {
            report(endif_command->source, Diag::too_many_arguments);
            return std::nullopt;
        }

        body_stms->splice_front(*std::move(andor_list));
        body_stms->push_front(*ParserIR::Builder(*arena)
                                       .command(if_command, src_info)
                                       .arg_int(andor_count, src_info)
                                       .build());

        return body_stms;
    }
}

auto Parser::parse_while_statement() -> std::optional<LinkedIR<ParserIR>>
{
    return parse_while_statement_detail(false);
}

auto Parser::parse_whilenot_statement() -> std::optional<LinkedIR<ParserIR>>
{
    return parse_while_statement_detail(true);
}

auto Parser::parse_while_statement_detail(bool is_whilenot)
        -> std::optional<LinkedIR<ParserIR>>
{
    // while_statement := 'WHILE' sep conditional_list
    //                    {statement}
    //                    [label_prefix] 'ENDWHILE' eol ;
    //
    // whilenot_statement := 'WHILENOT' sep conditional_list
    //                       {statement}
    //                       [label_prefix] 'ENDWHILE' eol ;
    //

    const auto while_command = is_whilenot ? COMMAND_WHILENOT : COMMAND_WHILE;

    auto while_token = consume_word(while_command);
    if(!while_token)
        return std::nullopt;

    if(!consume_whitespace())
        return std::nullopt;

    auto [andor_list, andor_count] = parse_conditional_list();
    if(!andor_list)
        return std::nullopt;

    auto body_stms = parse_statement_list("ENDWHILE");
    if(!body_stms)
        return std::nullopt;

    assert(!body_stms->empty() && body_stms->back().command);

    if(const auto& endwhile_command = body_stms->back().command;
       endwhile_command->args.size() != 0)
    {
        report(endwhile_command->source, Diag::too_many_arguments);
        return std::nullopt;
    }

    const auto src_info = while_token->source;

    body_stms->splice_front(*std::move(andor_list));

    body_stms->push_front(*ParserIR::Builder(*arena)
                                   .command(while_command, src_info)
                                   .arg_int(andor_count, src_info)
                                   .build());

    return body_stms;
}

auto Parser::parse_repeat_statement() -> std::optional<LinkedIR<ParserIR>>
{
    // repeat_statement := 'REPEAT' sep integer sep identifier eol
    //                     {statement}
    //                     [label_prefix] 'ENDREPEAT' eol ;

    if(!is_peek(Category::Word, "REPEAT"))
    {
        consume_word("REPEAT"); // produces a diagnostic
        return std::nullopt;
    }

    auto repeat_command = parse_command();
    if(!repeat_command)
        return std::nullopt;
    if(!consume(Category::EndOfLine))
        return std::nullopt;

    assert(*repeat_command && (*repeat_command)->command != nullptr);
    if(const auto repeat_num_args = (*repeat_command)->command->args.size();
       repeat_num_args < 2)
    {
        report((*repeat_command)->command->source, Diag::too_few_arguments);
        return std::nullopt;
    }
    else if(repeat_num_args > 2)
    {
        report((*repeat_command)->command->source, Diag::too_many_arguments);
        return std::nullopt;
    }

    auto body_stms = parse_statement_list("ENDREPEAT");
    if(!body_stms)
        return std::nullopt;

    assert(!body_stms->empty() && body_stms->back().command != nullptr);
    if(const auto endrepeat_command = body_stms->back().command;
       endrepeat_command->args.size() != 0)
    {
        report((*repeat_command)->command->source, Diag::too_many_arguments);
        return std::nullopt;
    }

    body_stms->push_front(**repeat_command);
    return body_stms;
}

auto Parser::parse_require_statement() -> std::optional<arena_ptr<ParserIR>>
{
    // require_statement := command_gosub_file
    //                    | command_launch_mission
    //                    | command_load_and_launch_mission ;
    //
    // command_gosub_file := 'GOSUB_FILE' sep identifier sep filename eol ;
    // command_launch_mission := 'LAUNCH_MISSION' sep filename eol ;
    // command_load_and_launch_mission := 'LOAD_AND_LAUNCH_MISSION' sep filename eol ;

    auto command = consume_command();
    if(!command)
        return std::nullopt;

    ParserIR::Builder builder(*arena);
    builder.command(scanner.spelling(*command), command->source);

    if(iequal(scanner.spelling(*command), COMMAND_GOSUB_FILE))
    {
        if(!consume_whitespace())
            return std::nullopt;

        auto arg_label = parse_argument();
        if(!arg_label)
            return std::nullopt;

        builder.arg(*arg_label);
    }
    else if(!iequal(scanner.spelling(*command), COMMAND_LAUNCH_MISSION)
            && !iequal(scanner.spelling(*command),
                       COMMAND_LOAD_AND_LAUNCH_MISSION))
    {
        report(command->source, Diag::expected_require_command);
        return std::nullopt;
    }

    if(!consume_whitespace())
        return std::nullopt;

    auto tok_filename = this->consume_filename();
    if(!tok_filename)
        return std::nullopt;

    if(!consume(Category::EndOfLine))
        return std::nullopt;

    builder.arg_filename(scanner.spelling(*tok_filename), tok_filename->source);

    return std::move(builder).build();
}

auto Parser::parse_assignment_expression() -> std::optional<LinkedIR<ParserIR>>
{
    return parse_expression_detail(false, false, false);
}

auto Parser::parse_conditional_expression(bool is_if_line, bool not_flag)
        -> std::optional<LinkedIR<ParserIR>>
{
    return parse_expression_detail(true, is_if_line, not_flag);
}

auto Parser::parse_expression_detail(bool is_conditional, bool is_if_line,
                                     bool not_flag)
        -> std::optional<LinkedIR<ParserIR>>
{
    // binop := '+' | '-' | '*' | '/' | '+@' | '-@' ;
    // asop := '=' | '=#' | '+=' | '-=' | '*=' | '/=' | '+=@' | '-=@' ;
    // unop := '--' | '++' ;
    //
    // expr_assign_abs := identifier {whitespace} '=' {whitespace} 'ABS'
    //                    {whitespace} argument ;
    // expr_assign_binary := identifier {whitespace} asop {whitespace} argument ;
    // expr_assign_ternary := identifier {whitespace} '=' {whitespace} argument
    //                        {whitespace} binop {whitespace} argument ;
    // expr_assign_unary := (unop {whitespace} identifier)
    //                    | (identifier {whitespace} unop) ;
    //
    // assignment_expression := expr_assign_unary
    //                        | expr_assign_binary
    //                        | expr_assign_ternary
    //                        | expr_assign_abs ;
    //
    // relop := '=' | '<' | '>' | '>=' | '<=' ;
    // conditional_expression := argument {whitespace} relop {whitespace} argument ;
    //
    // Constraints:
    //
    //      The arguments of an expression may not allow string literals.
    //
    //      The name of commands used to require script files (e.g.
    //      `GOSUB_FILE`) and its directive commands (i.e. `MISSION_START` and
    //      `MISSION_END`) cannot be on the left hand side of a expression.
    //
    // FOLLOW(assignment_expression) = {eol}
    // FOLLOW(conditional_expression) = {eol, sep 'GOTO'}

    Category cats[6];
    SourceRange spans[6];
    arena_ptr<const ParserIR::Argument> args[6];

    size_t num_toks = 0;
    size_t num_args = 0;

    assert(std::size(cats) >= std::size(args));
    assert(std::size(cats) == std::size(spans));

    // This is a very special part of the language grammar. We can quickly
    // and cleanly parse this by applying some pattern matching on the tokens
    // of the line. That is what we gonna do, take all the tokens in this line
    // and match them against the production rules.

    while(!is_peek(Category::EndOfLine))
    {
        if(is_if_line && is_peek(Category::Whitespace, 0)
           && is_peek(Category::Word, "GOTO", 1)
           && is_peek(Category::Whitespace, 2) && is_peek(Category::Word, 3)
           && is_peek(Category::EndOfLine, 4))
            break;

        // Getting over this number of tokens implies we'll not match
        // any of the productions in the specification.
        if(num_toks == std::size(cats))
        {
            diagnostics().report(spans[0].begin, Diag::invalid_expression);
            return std::nullopt;
        }

        if(peek() == std::nullopt)
        {
            consume(); // skip and produce diagnostic
            return std::nullopt;
        }

        switch(peek()->category)
        {
            case Category::Equal:
            case Category::EqualHash:
            case Category::PlusEqual:
            case Category::MinusEqual:
            case Category::StarEqual:
            case Category::SlashEqual:
            case Category::PlusEqualAt:
            case Category::MinusEqualAt:
            case Category::PlusPlus:
            case Category::MinusMinus:
            case Category::Less:
            case Category::LessEqual:
            case Category::Greater:
            case Category::GreaterEqual:
            case Category::Plus:
            case Category::Minus:
            case Category::Star:
            case Category::Slash:
            case Category::PlusAt:
            case Category::MinusAt:
            {
                spans[num_toks] = peek()->source;
                cats[num_toks++] = consume()->category;
                break;
            }
            case Category::Word:
            {
                spans[num_toks] = peek()->source;
                cats[num_toks++] = Category::Word;
                if(auto arg = parse_argument())
                {
                    args[num_args++] = *arg;
                    assert(num_args <= std::size(args));
                }
                else
                {
                    return std::nullopt;
                }
                break;
            }
            case Category::Whitespace:
            {
                consume();
                break;
            }
            case Category::String:
            {
                // needs special care when comparing for name equality.
                // not implemented yet. TODO
                assert(false);
                return std::nullopt;
            }
            default:
            {
                assert(false);
                return std::nullopt;
            }
        }
    }

    // Cannot produce any match for zero tokens.
    if(num_toks == 0)
    {
        assert(peek() != std::nullopt);
        report(peek()->source, Diag::invalid_expression);
        return std::nullopt;
    }

    // Only binary expressions can be matched in conditional contexts.
    if(is_conditional && num_toks != 3)
    {
        if(num_toks >= 2 && is_relational_operator(cats[1]))
        {
            diagnostics().report(spans[0].begin, Diag::invalid_expression);
            return std::nullopt;
        }
        else
        {
            diagnostics().report(spans[0].begin,
                                 Diag::expected_conditional_expression);
            return std::nullopt;
        }
    }

    // Make sure the left-hand side of a expression is not require commands
    // nor mission directives. This is a constraint of the grammar.
    if(num_args > 0 && cats[0] == Category::Word && args[0]->as_identifier())
    {
        auto lhs = args[0]->as_identifier();
        if(*lhs == COMMAND_GOSUB_FILE || *lhs == COMMAND_LAUNCH_MISSION
           || *lhs == COMMAND_LOAD_AND_LAUNCH_MISSION
           || *lhs == COMMAND_MISSION_START || *lhs == COMMAND_MISSION_END)
        {
            report_special_name(args[0]->source);
            return std::nullopt;
        }
    }

    auto linked = LinkedIR<ParserIR>();

    const auto src_info = SourceRange(spans[0].begin,
                                      spans[num_toks - 1].end - spans[0].begin);

    if(num_toks == 2
       && ((cats[0] == Category::Word && cats[1] == Category::PlusPlus)
           || (cats[0] == Category::PlusPlus && cats[1] == Category::Word)))
    {
        linked.push_back(*ParserIR::Builder(*arena)
                                  .not_flag(not_flag)
                                  .command(COMMAND_ADD_THING_TO_THING, src_info)
                                  .arg(args[0])
                                  .arg_int(1, src_info)
                                  .build());
    }
    else if(num_toks == 2
            && ((cats[0] == Category::Word && cats[1] == Category::MinusMinus)
                || (cats[0] == Category::MinusMinus
                    && cats[1] == Category::Word)))
    {
        linked.push_back(
                *ParserIR::Builder(*arena)
                         .not_flag(not_flag)
                         .command(COMMAND_SUB_THING_FROM_THING, src_info)
                         .arg(args[0])
                         .arg_int(1, src_info)
                         .build());
    }
    else if(num_toks == 4 && cats[0] == Category::Word
            && cats[1] == Category::Equal && cats[2] == Category::Word
            && args[1]->as_identifier() && *args[1]->as_identifier() == "ABS"sv
            && cats[3] == Category::Word)
    {
        const auto a = args[0], b = args[2];
        if(a->is_same_value(*b))
        {
            linked.push_back(*ParserIR::Builder(*arena)
                                      .not_flag(not_flag)
                                      .command(COMMAND_ABS, src_info)
                                      .arg(a)
                                      .build());
        }
        else
        {
            linked.push_back(*ParserIR::Builder(*arena)
                                      .not_flag(not_flag)
                                      .command(COMMAND_SET, src_info)
                                      .arg(a)
                                      .arg(b)
                                      .build());
            linked.push_back(*ParserIR::Builder(*arena)
                                      .not_flag(not_flag)
                                      .command(COMMAND_ABS, src_info)
                                      .arg(a)
                                      .build());
        }
    }
    else if(num_toks == 3 && cats[0] == Category::Word
            && cats[1] != Category::Word && cats[2] == Category::Word)
    {
        using LookupItem = std::pair<Category, std::string_view>;

        static constexpr LookupItem lookup_assignment[] = {
                LookupItem{Category::Equal, COMMAND_SET},
                LookupItem{Category::EqualHash, COMMAND_CSET},
                LookupItem{Category::PlusEqual, COMMAND_ADD_THING_TO_THING},
                LookupItem{Category::MinusEqual, COMMAND_SUB_THING_FROM_THING},
                LookupItem{Category::StarEqual, COMMAND_MULT_THING_BY_THING},
                LookupItem{Category::SlashEqual, COMMAND_DIV_THING_BY_THING},
                LookupItem{Category::PlusEqualAt,
                           COMMAND_ADD_THING_TO_THING_TIMED},
                LookupItem{Category::MinusEqualAt,
                           COMMAND_SUB_THING_FROM_THING_TIMED},
        };

        static constexpr LookupItem lookup_conditional[] = {
                LookupItem{Category::Equal, COMMAND_IS_THING_EQUAL_TO_THING},
                LookupItem{Category::Less, COMMAND_IS_THING_GREATER_THAN_THING},
                LookupItem{Category::LessEqual,
                           COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING},
                LookupItem{Category::Greater,
                           COMMAND_IS_THING_GREATER_THAN_THING},
                LookupItem{Category::GreaterEqual,
                           COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING},
        };

        const auto it_cond = std::find_if(
                std::begin(lookup_conditional), std::end(lookup_conditional),
                [&](const auto& pair) { return pair.first == cats[1]; });

        const auto it_assign = std::find_if(
                std::begin(lookup_assignment), std::end(lookup_assignment),
                [&](const auto& pair) { return pair.first == cats[1]; });

        if(it_cond == std::end(lookup_conditional)
           && it_assign == std::end(lookup_assignment))
        {
            diagnostics().report(spans[0].begin, Diag::invalid_expression);
            return std::nullopt;
        }

        std::string_view command_name;
        auto a = args[0], b = args[1];

        if(is_conditional)
        {
            const auto it = it_cond;

            if(it == std::end(lookup_conditional))
            {
                report(spans[1], Diag::expected_conditional_operator);
                return std::nullopt;
            }

            command_name = it->second;

            // Less (and less than) uses the greater (and greater than)
            // command to perform its comparision. Swap arguments for this
            // to work properly.
            if(it->first == Category::Less || it->first == Category::LessEqual)
                std::swap(a, b);
        }
        else
        {
            const auto it = it_assign;

            if(it == std::end(lookup_assignment))
            {
                report(spans[1], Diag::expected_assignment_operator);
                return std::nullopt;
            }

            command_name = it->second;
        }

        linked.push_back(*ParserIR::Builder(*arena)
                                  .not_flag(not_flag)
                                  .command(command_name, src_info)
                                  .arg(a)
                                  .arg(b)
                                  .build());
    }
    else if(num_toks == 5 && cats[0] == Category::Word
            && cats[1] == Category::Equal && cats[2] == Category::Word
            && cats[3] != Category::Word && cats[4] == Category::Word)
    {
        using LookupItem = std::pair<Category, std::string_view>;

        static constexpr LookupItem lookup_ternary[] = {
                LookupItem{Category::Plus, COMMAND_ADD_THING_TO_THING},
                LookupItem{Category::Minus, COMMAND_SUB_THING_FROM_THING},
                LookupItem{Category::Star, COMMAND_MULT_THING_BY_THING},
                LookupItem{Category::Slash, COMMAND_DIV_THING_BY_THING},
                LookupItem{Category::PlusAt, COMMAND_ADD_THING_TO_THING_TIMED},
                LookupItem{Category::MinusAt,
                           COMMAND_SUB_THING_FROM_THING_TIMED},
        };

        auto it = std::find_if(
                std::begin(lookup_ternary), std::end(lookup_ternary),
                [&](const auto& pair) { return pair.first == cats[3]; });

        if(it == std::end(lookup_ternary))
        {
            report(spans[3], Diag::expected_ternary_operator);
            return std::nullopt;
        }

        const auto a = args[0], b = args[1], c = args[2];

        const bool is_associative = (cats[3] == Category::Plus
                                     || cats[3] == Category::Star);

        if(a->is_same_value(*b))
        {
            linked.push_back(*ParserIR::Builder(*arena)
                                      .command(it->second, src_info)
                                      .arg(a)
                                      .arg(c)
                                      .build());
        }
        else if(a->is_same_value(*c))
        {
            if(!is_associative)
            {
                diagnostics()
                        .report(spans[0].begin,
                                Diag::invalid_expression_unassociative)
                        .args(cats[3]);
                return std::nullopt;
            }

            linked.push_back(*ParserIR::Builder(*arena)
                                      .command(it->second, src_info)
                                      .arg(a)
                                      .arg(b)
                                      .build());
        }
        else
        {
            linked.push_back(*ParserIR::Builder(*arena)
                                      .command(COMMAND_SET, src_info)
                                      .arg(a)
                                      .arg(b)
                                      .build());
            linked.push_back(*ParserIR::Builder(*arena)
                                      .command(it->second, src_info)
                                      .arg(a)
                                      .arg(c)
                                      .build());
        }
    }
    else
    {
        diagnostics().report(spans[0].begin, Diag::invalid_expression);
        return std::nullopt;
    }

    assert(is_peek(Category::EndOfLine) || is_peek(Category::Whitespace));

    return linked;
}

bool Parser::ensure_mission_start_at_top_of_file()
{
    // The MISSION_START command shall be the very first line of the subscript
    // file and shall not be preceded by anything but ASCII spaces ( ) and
    // horizontal tabs (\t). Comments instead of whitespaces are disallowed.

    bool has_mission_start = is_peek(Category::Word, "MISSION_START"sv);

    auto file_contents = source_file().code_view();
    for(auto it = file_contents.begin();
        it != file_contents.end() && has_mission_start; ++it)
    {
        if(*it == 'M' || *it == 'm')
            break;
        if(*it != ' ' || *it != '\t')
            has_mission_start = false;
    }

    if(!has_mission_start)
    {
        diagnostics().report(source_file().location_of(file_contents),
                             Diag::expected_mission_start_at_top);
        return false;
    }

    return true;
}
}

// TODO perform parsing recovery
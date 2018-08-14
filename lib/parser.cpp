#include <gta3sc/parser.hpp>
#include <cstring>
#include <cassert>
using namespace std::literals::string_view_literals;

// grammar from gta3script-specs de0049f5258dc11d3e409371a8981f8a01b28d93

// TODO produce diagnostics
// TODO perform parsing recovery

namespace gta3sc
{
constexpr auto COMMAND_MISSION_START = "MISSION_START"sv;
constexpr auto COMMAND_MISSION_END = "MISSION_END"sv;
constexpr auto COMMAND_ANDOR = "ANDOR"sv;
constexpr auto COMMAND_GOTO = "GOTO"sv;
constexpr auto COMMAND_IF = "IF"sv;
constexpr auto COMMAND_IFNOT = "IFNOT"sv;
constexpr auto COMMAND_ELSE = "ELSE"sv;
constexpr auto COMMAND_ENDIF = "ENDIF"sv;
constexpr auto COMMAND_GOTO_IF_FALSE = "GOTO_IF_FALSE"sv;
constexpr auto COMMAND_GOTO_IF_TRUE = "GOTO_IF_TRUE"sv;
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
constexpr auto COMMAND_SUB_THING_FROM_THING_TIMED = "SUB_THING_FROM_THING_TIMED"sv;
constexpr auto COMMAND_IS_THING_EQUAL_TO_THING = "IS_THING_EQUAL_TO_THING"sv;
constexpr auto COMMAND_IS_THING_GREATER_THAN_THING = "IS_THING_GREATER_THAN_THING"sv;
constexpr auto COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING = "IS_THING_GREATER_OR_EQUAL_TO_THING"sv;

auto Parser::source_file() const -> const SourceFile&
{
    return scanner.source_file();
}

auto Parser::source_info(const Token& token) const -> ParserIR::SourceInfo
{
    return ParserIR::SourceInfo {
        this->source_file(),
        token.lexeme,
    };
}
auto Parser::source_info(SourceLocation begin, SourceLocation end) const
    -> ParserIR::SourceInfo
{
    return ParserIR::SourceInfo {
        this->source_file(),
        SourceRange(begin, end - begin),
    };
}

bool Parser::iequal(std::string_view a, std::string_view b) const
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](unsigned char ac, unsigned char bc) {
                        if(ac >= 'a' && ac <= 'z') ac -= 32;
                        if(bc >= 'a' && bc <= 'z') bc -= 32;
                        return ac == bc;
                      });
}

auto Parser::peek(size_t n) -> std::optional<Token>
{
    assert(n < std::size(peek_tokens));

    if(!has_peek_token[n])
    {
        assert(n == 0 || has_peek_token[n-1]);

        if(n != 0 && peek_tokens[n-1]
            && peek_tokens[n-1]->category == Category::EndOfLine)
        {
            has_peek_token[n] = true;
            peek_tokens[n] = std::nullopt;
        }
        else
        {
            has_peek_token[n] = true;
            peek_tokens[n] = scanner.next();
        }
    }

    return peek_tokens[n];
}

auto Parser::next() -> std::optional<Token>
{
    if(!has_peek_token[0])
        return scanner.next();

    assert(has_peek_token[0] == true);
    auto eat_token = peek_tokens[0];

    // Move the peek tokens one position to the front since the
    // zero position is now empty.
    size_t n = 1;
    for(; n < size(peek_tokens) && has_peek_token[n]; ++n)
        peek_tokens[n-1] = peek_tokens[n];

    has_peek_token[n-1] = false;

    return eat_token;
}

auto Parser::next_filename() -> std::optional<Token>
{
    assert(has_peek_token[0] == false);
    return scanner.next_filename();
}

void Parser::skip_current_line()
{
    // Although the scanner is guaranted to return a new line at some point
    // we'll put a limit on how many iterations we can perform before giving up.
    size_t token_limit_per_line = 2048;
    while(token_limit_per_line--)
    {
        auto token = this->next();
        if(token && token->category == Category::EndOfLine)
            break;
    }
}

bool Parser::is_digit(char c) const
{
    return c >= '0' && c <= '9';
}

bool Parser::is_integer(const Token& token) const
{
    // integer := ['-'] digit {digit} ;

    if(token.category != Category::Word)
        return false;

    size_t i = 0;
    for(auto c : token.lexeme)
    {
        i = i + 1;
        if(is_digit(c))
            continue;
        else if(c == '-' && i == 1)
            continue;
        else
            return false;
    }

    return true;
}

bool Parser::is_float(const Token& token) const
{
    // floating_form1 := '.' digit { digit | '.' | 'F' } ;
    // floating_form2 := digit { digit } ('.' | 'F') { digit | '.' | 'F' } ;
    // floating := ['-'] (floating_form1 | floating_form2) ;

    if(token.category != Category::Word)
        return false;

    auto it = token.lexeme.begin();

    if(token.lexeme.size() >= 2 && *it == '-')
        ++it;

    if(*it == '.') // floating_form1
    {
        ++it;
        if(it == token.lexeme.end() || !is_digit(*it))
            return false;
        ++it;
    }
    else if(*it >= '0' && *it <= '9') // floating_form2
    {
        ++it;
        it = std::find_if_not(it, token.lexeme.end(), // {digit}
                              [this](char c) { return is_digit(c); });
        if(it == token.lexeme.end() || (*it != '.' && *it != 'f' && *it != 'F'))
            return false;
        ++it;
    }
    else // not a floating point
    {
        return false;
    }

    // Skip the final, common part: {digit | '.' | 'F'}
    it = std::find_if_not(it, token.lexeme.end(),
                          [this](char c) { return (c == '.' || c == 'f'
                                                   || c == 'F' || is_digit(c)); });

    return it == token.lexeme.end();
}

bool Parser::is_identifier(const Token& token) const
{
    // identifier := ('$' | 'A'..'Z') {token_char} ;
    //
    // Constraints: 
    // An identifier should not end with a `:` character.

    if(token.category != Category::Word)
        return false;

    if(token.lexeme.size() >= 1)
    {
        const auto front = token.lexeme.front();
        const auto back = token.lexeme.back();

        if(front == '$'
            || (front >= 'A' && front <= 'Z')
            || (front >= 'a' && front <= 'z'))
        {
            return back != ':';
        }
    }

    return false;
}

auto Parser::parse_statement()
    -> std::optional<LinkedIR<ParserIR>>
{
    // statement := labeled_statement 
    //            | embedded_statement ;
    //
    // label_def := identifier ':' ;
    // labeled_statement := label_def (sep embedded_statement | empty_statement) ;
    //
    // empty_statement := eol ;
    //

    arena_ptr<ParserIR::LabelDef> label = nullptr;

    if(is_peek(Category::Word) && peek()->lexeme.back() == ':')
    {
        auto label_def = *consume();
        label_def.lexeme.remove_suffix(1);

        if(!is_identifier(label_def))
            return std::nullopt;

        if(!is_peek(Category::EndOfLine))
        {
            if(!consume(Category::Whitespace))
                return std::nullopt;
        }

        label = ParserIR::LabelDef::create(source_info(label_def),
                                           label_def.lexeme,
                                           arena);
    }

    auto linked_stmts = parse_embedded_statement();
    if(!linked_stmts)
        return std::nullopt;

    if(label)
    {
        if(linked_stmts->empty())
            linked_stmts->push_back(ParserIR::create(arena));

        assert(linked_stmts->front()->label == nullptr);
        linked_stmts->front()->label = label;
    }

    return *std::move(linked_stmts);
    // FIXME https://stackoverflow.com/q/51379597/2679626
}

auto Parser::parse_embedded_statement()
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
    //                      | while_statement (TODO)
    //                      | whilenot_statement (TODO)
    //                      | repeat_statement (TODO)
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
        if(auto stmt_ir = parse_require_statement())
            return LinkedIR<ParserIR>::from_ir(*stmt_ir);
        return std::nullopt;
    }
    else if(auto category = peek_expression_type())
    {
        auto linked = std::optional<LinkedIR<ParserIR>>();
        if(is_relational_operator(*category))
            linked = parse_conditional_expression();
        else
            linked = parse_assignment_expression();

        if(!linked)
            return std::nullopt;

        if(!consume(Category::EndOfLine))
            return std::nullopt;

        return *std::move(linked); // FIXME clang bug
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
    else
    {
        if(auto ir = parse_command())
        {
            if(consume(Category::EndOfLine))
                return LinkedIR<ParserIR>::from_ir(*ir);
        }
        return std::nullopt;
    }
}

auto Parser::peek_expression_type()
    -> std::optional<Category>
{
    if(is_peek(Category::PlusPlus) || is_peek(Category::MinusMinus))
        return peek()->category;

    const auto opos = is_peek(Category::Whitespace, 1)? 2 : 1;
    switch(peek(opos)? peek(opos)->category : Category::Word)
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

bool Parser::is_relational_operator(Category category)
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

auto Parser::parse_if_statement()
    -> std::optional<LinkedIR<ParserIR>>
{
    return parse_if_statement_internal(false);
}

auto Parser::parse_ifnot_statement()
    -> std::optional<LinkedIR<ParserIR>>
{
    return parse_if_statement_internal(true);
}

auto Parser::parse_if_statement_internal(bool is_ifnot)
    -> std::optional<LinkedIR<ParserIR>>
{
    // if_statement := 'IF' sep conditional_list
    //                 {statement}
    //                 ['ELSE'
    //                 {statement}]
    //                 'ENDIF' ;
    //
    // if_goto_statement := 'IF' sep conditional_element sep 'GOTO' sep identifier eol ;
    // 
    // ifnot_statement := 'IFNOT' sep conditional_list
    //                 {statement}
    //                 ['ELSE'
    //                 {statement}]
    //                 'ENDIF' ;
    //
    // ifnot_goto_statement := 'IFNOT' sep conditional_element sep 'GOTO' sep identifier eol; 
    //

    const auto if_command = is_ifnot? COMMAND_IFNOT : COMMAND_IF;
    const auto if_true_command = is_ifnot? COMMAND_GOTO_IF_FALSE : COMMAND_GOTO_IF_TRUE;
   
    auto if_token = consume_word(if_command);
    if(!if_token)
        return std::nullopt;

    if(!consume(Category::Whitespace))
        return std::nullopt;

    auto op_cond0 = parse_conditional_element(true);
    if(!op_cond0)
        return std::nullopt;

    const auto src_info = source_info(*if_token);

    if(is_peek(Category::Whitespace))
    {
        if(!consume() || !consume_word("GOTO") || !consume(Category::Whitespace))
            return std::nullopt;

        auto goto_arg = parse_argument();
        if(!goto_arg)
            return std::nullopt;

        auto op_andor = ParserIR::create_command(src_info, COMMAND_ANDOR, arena);
        op_andor->command->push_arg(ParserIR::create_integer(src_info, 0, arena), arena);
        auto op_goto = ParserIR::create_command(src_info, if_true_command, arena);
        op_goto->command->push_arg(*goto_arg, arena);

        auto linked = LinkedIR<ParserIR>();
        linked.push_back(op_andor);
        linked.push_back(*op_cond0);
        linked.push_back(op_goto);
        return linked;
    }
    else
    {
        if(!consume(Category::EndOfLine))
            return std::nullopt;

        auto [andor_list, andor_count] = parse_conditional_list(*op_cond0);
        if(!andor_list)
            return std::nullopt;

        auto body_stms = parse_statement_list("ELSE", "ENDIF");
        if(!body_stms)
            return std::nullopt;

        if(body_stms->back()->command->name == COMMAND_ELSE)
        {
            auto else_stms = parse_statement_list("ENDIF");
            if(!else_stms)
                return std::nullopt;

            body_stms->splice_back(*std::move(else_stms));
        }

        auto op_if = ParserIR::create_command(src_info, if_command, arena);
        op_if->command->push_arg(ParserIR::create_integer(src_info, andor_count, arena), arena);

        body_stms->splice_front(*std::move(andor_list));
        body_stms->push_front(op_if);

        assert(body_stms->front()->command->name == if_command);
        assert(body_stms->back()->command->name == COMMAND_ENDIF);

        return *std::move(body_stms); // FIXME clang bug
    }
}

auto Parser::parse_conditional_element(bool is_if_line)
    -> std::optional<arena_ptr<ParserIR>>
{
    // conditional_element := ['NOT' sep] (command | conditional_expression) ;

    bool not_flag = false;

    if(is_peek(Category::Word, "NOT"))
    {
        if(!consume() || !consume(Category::Whitespace))
            return std::nullopt;
        not_flag = true;
    }

    auto linked = std::optional<LinkedIR<ParserIR>>();
    if(peek_expression_type())
    {
        linked = parse_conditional_expression(is_if_line);
    }
    else
    {
        if(auto ir = parse_command(is_if_line))
            linked = LinkedIR<ParserIR>::from_ir(*ir);
    }

    if(!linked)
        return std::nullopt;

    auto ir = std::move(*linked).into_ir();
    assert(ir->next == nullptr); // assume single command in the list
    ir->command->not_flag = not_flag;

    return ir;
}

auto Parser::parse_conditional_list(arena_ptr<ParserIR> op_cond0)
    -> std::pair<std::optional<LinkedIR<ParserIR>>, int32_t>
{
    // and_conditional_stmt := 'AND' sep conditional_element eol ;
    // or_conditional_stmt := 'OR' sep conditional_element eol ;
    // 
    // conditional_list := conditional_element eol
    //                     ({and_conditional_stmt} | {or_conditional_stmt}) ;

    assert(op_cond0 && op_cond0->command);

    const auto src_info = op_cond0->command->source_info;
    auto andor_list = LinkedIR<ParserIR>::from_ir(op_cond0);

    size_t num_conds = 1;
    int32_t andor_count = 0;

    if(is_peek(Category::Word, "AND")
        || is_peek(Category::Word, "OR"))
    {
        const auto is_and = is_peek(Category::Word, "AND");
        const auto andor_prefix = is_and? "AND" : "OR";
        const auto anti_prefix = is_and? "OR" : "AND";

        while(is_peek(Category::Word, andor_prefix))
        {
            if(!consume() || !consume(Category::Whitespace))
                return {std::nullopt, 0};

            auto op_elem = parse_conditional_element();
            if(!op_elem)
                return {std::nullopt, 0};

            if(!consume(Category::EndOfLine))
                return {std::nullopt, 0};

            assert((*op_elem)->next == nullptr); // assume single command
            andor_list.push_back(*op_elem);
            ++num_conds;
        }

        if(is_peek(Category::Word, anti_prefix))
            return {std::nullopt, 0};

        andor_count = is_and? num_conds - 1 : 20 + num_conds - 1;
    }

    if(num_conds > 6)
        return {std::nullopt, 0};

    return {std::move(andor_list), andor_count};
}

auto Parser::parse_scope_statement()
    -> std::optional<LinkedIR<ParserIR>>
{
    // scope_statement := '{' eol
    //                    {statement}
    //                    '}' eol ;
    //
    // Constraints: 
    // Lexical scopes cannot be nested.
    
    if(this->in_lexical_scope)
        return std::nullopt;

    if(!is_peek(Category::Word, "{"))
        return std::nullopt;

    auto open_command = parse_command();
    if(!open_command)
        return std::nullopt;
    if(!consume(Category::EndOfLine))
        return std::nullopt;

    // Restore back only if we reach the closing curly.
    this->in_lexical_scope = true;

    auto linked_stmts = parse_statement_list("}");
    if(!linked_stmts)
        return std::nullopt;

    this->in_lexical_scope = false;

    linked_stmts->push_front(*open_command);
    return *std::move(linked_stmts); // FIXME clang bug
}

auto Parser::parse_require_statement()
    -> std::optional<arena_ptr<ParserIR>>
{
    // require_statement := command_gosub_file
    //                    | command_launch_mission
    //                    | command_load_and_launch_mission ;
    //
    // command_gosub_file := 'GOSUB_FILE' sep identifier sep filename eol ;
    // command_launch_mission := 'LAUNCH_MISSION' sep filename eol ;
    // command_load_and_launch_mission := 'LOAD_AND_LAUNCH_MISSION' sep filename eol ;

    auto command = consume(Category::Word);
    if(!command)
        return std::nullopt;

    auto result = ParserIR::create_command(source_info(*command),
                                           command->lexeme,
                                           arena);

    if(iequal(command->lexeme, COMMAND_GOSUB_FILE))
    {
        if(!consume(Category::Whitespace))
            return std::nullopt;

        auto arg = parse_argument();
        if(!arg)
            return std::nullopt;

        result->command->push_arg(*arg, arena);
    }
    else if(!iequal(command->lexeme, COMMAND_LAUNCH_MISSION)
         && !iequal(command->lexeme, COMMAND_LOAD_AND_LAUNCH_MISSION))
    {
        return std::nullopt;
    }

    if(!consume(Category::Whitespace))
        return std::nullopt;

    auto arg = this->next_filename();
    if(!arg)
        return std::nullopt;

    if(!consume(Category::EndOfLine))
        return std::nullopt;

    auto filename = ParserIR::create_filename(source_info(*arg),
                                              arg->lexeme,
                                              arena);
    result->command->push_arg(filename, arena);

    return result;
}

auto Parser::parse_command(bool is_if_line) 
    -> std::optional<arena_ptr<ParserIR>>
{
    // command_name := token_char {token_char} ;
    // command := command_name { sep argument } ;
    // FOLLOW(command) = {eol, sep 'GOTO'}
    
    auto command = consume(Category::Word);
    if(!command)
        return std::nullopt;

    auto result = ParserIR::create_command(source_info(*command),
                                           command->lexeme,
                                           arena);

    while(!is_peek(Category::EndOfLine))
    {
        if(is_if_line
           && is_peek(Category::Whitespace, 0)
           && is_peek(Category::Word, "GOTO", 1)
           && is_peek(Category::Whitespace, 2)
           && is_peek(Category::Word, 3)
           && is_peek(Category::EndOfLine, 4))
            break;

        if(!consume(Category::Whitespace))
            return std::nullopt;
        
        auto arg = parse_argument();
        if(!arg)
            return std::nullopt;

        result->command->push_arg(*arg, arena);
    }

    assert(is_peek(Category::EndOfLine) || is_peek(Category::Whitespace));

    return result;
}

auto Parser::parse_argument() 
    -> std::optional<arena_ptr<ParserIR::Argument>>
{
    // argument := integer
    //             | floating 
    //             | identifier
    //             | string_literal ;

    auto token = consume(Category::Word, Category::String);
    if(!token)
        return std::nullopt;

    const auto src_info = this->source_info(*token);

    if(token->category == Category::String)
    {
        auto string = token->lexeme;
        string.remove_prefix(1);
        string.remove_suffix(1);
        return ParserIR::create_string(src_info, string, arena);
    }
    else if(is_integer(*token))
    {
        constexpr long min_value = (-2147483647-1), max_value = 2147483647;

        static_assert(std::numeric_limits<long>::min() <= min_value
                    && std::numeric_limits<long>::max() >= max_value);

        // Avoid using std::string for this. Use the C library std::strtol
        // function. We may also use std::from_chars when widely available.
        char buffer[64];
        auto length = std::min(token->lexeme.size(), std::size(buffer) - 1);
        std::memcpy(buffer, token->lexeme.data(), length);
        buffer[length] = '\0';

        errno = 0;
        auto value = std::strtol(buffer, nullptr, 10);

        if(errno == ERANGE || value < min_value || value > max_value)
            return std::nullopt;

        return ParserIR::create_integer(src_info, value, arena);
    }
    else if(is_float(*token))
    {
        char buffer[64];
        auto length = std::min(token->lexeme.size(), std::size(buffer) - 1);
        std::memcpy(buffer, token->lexeme.data(), length);
        buffer[length] = '\0';

        errno = 0;
        auto value = std::strtof(buffer, nullptr);

        if(errno == ERANGE)
            return std::nullopt;

        return ParserIR::create_float(src_info, value, arena);
    }
    else if(is_identifier(*token))
    {
        return ParserIR::create_identifier(src_info, token->lexeme, arena);
    }
    else
    {
        return std::nullopt;
    }
}

auto Parser::parse_assignment_expression()
    -> std::optional<LinkedIR<ParserIR>>
{
    return parse_expression_internal(false, false);
}

auto Parser::parse_conditional_expression(bool is_if_line)
    -> std::optional<LinkedIR<ParserIR>>
{
    return parse_expression_internal(true, is_if_line);
}

auto Parser::parse_expression_internal(bool is_conditional, bool is_if_line)
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
    // FOLLOW(assignment_expression) = {eol}
    // FOLLOW(conditional_expression) = {eol, sep 'GOTO'}
    
    Category cats[6];
    SourceRange locs[6];
    arena_ptr<ParserIR::Argument> args[6];

    size_t num_toks = 0;
    size_t num_args = 0;

    assert(std::size(cats) >= std::size(args));
    assert(std::size(cats) == std::size(locs));

    while(!is_peek(Category::EndOfLine))
    {
        if(is_if_line
           && is_peek(Category::Whitespace, 0)
           && is_peek(Category::Word, "GOTO", 1)
           && is_peek(Category::Whitespace, 2)
           && is_peek(Category::Word, 3)
           && is_peek(Category::EndOfLine, 4))
            break;

        if(num_toks == std::size(cats))
            return std::nullopt;

        if(peek() == std::nullopt)
            return std::nullopt;

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
                locs[num_toks] = peek()->lexeme;
                cats[num_toks++] = consume()->category;
                break;
            }
            case Category::Word:
            {
                locs[num_toks] = peek()->lexeme;
                cats[num_toks++] = Category::Word;
                if(auto arg = parse_argument(); arg)
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
                // not implemented yet.
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

    if(num_toks == 0)
        return std::nullopt;

    if(is_conditional && num_toks != 3)
        return std::nullopt;

    if(num_args > 0 && args[0]->as_identifier())
    {
        auto lhs = args[0]->as_identifier();
        if(*lhs == COMMAND_GOSUB_FILE
            || *lhs == COMMAND_LAUNCH_MISSION
            || *lhs == COMMAND_LOAD_AND_LAUNCH_MISSION
            || *lhs == COMMAND_MISSION_START
            || *lhs == COMMAND_MISSION_END)
        {
            return std::nullopt;
        }
    }

    using LookupItem = std::pair<Category, std::string_view>;

    auto src_info = source_info(locs[0].begin(), locs[num_toks-1].end());
    arena_ptr<ParserIR> ir = nullptr;

    if(num_toks == 2
        && ((cats[0] == Category::Word && cats[1] == Category::PlusPlus)
            || (cats[0] == Category::PlusPlus && cats[1] == Category::Word)))
    {
        ir = ParserIR::create_command(src_info, COMMAND_ADD_THING_TO_THING, arena);
        ir->command->push_arg(args[0], arena);
        ir->command->push_arg(ParserIR::create_integer(src_info, 1, arena), arena);
    }
    else if(num_toks == 2
        && ((cats[0] == Category::Word && cats[1] == Category::MinusMinus)
            || (cats[0] == Category::MinusMinus && cats[1] == Category::Word)))
    {
        ir = ParserIR::create_command(src_info, COMMAND_SUB_THING_FROM_THING, arena);
        ir->command->push_arg(args[0], arena);
        ir->command->push_arg(ParserIR::create_integer(src_info, 1, arena), arena);
    }
    else if(num_toks == 4
        && cats[0] == Category::Word
        && cats[1] == Category::Equal
        && cats[2] == Category::Word
        && args[1]->as_identifier() && *args[1]->as_identifier() == "ABS"sv
        && cats[3] == Category::Word)
    {
        const auto a = args[0], b = args[2];
        if(a->is_same_name(*b))
        {
            ir = ParserIR::create_command(src_info, COMMAND_ABS, arena);
            ir->command->push_arg(a, arena);
        }
        else
        {
            ir = ParserIR::create_command(src_info, COMMAND_SET, arena);
            ir->command->push_arg(a, arena);
            ir->command->push_arg(b, arena);
            ir->set_next(ParserIR::create_command(src_info, COMMAND_ABS, arena));
            ir->next->command->push_arg(a, arena);
        }
    }
    else if(num_toks == 3
        && cats[0] == Category::Word
        && cats[1] != Category::Word
        && cats[2] == Category::Word)
    {
        static constexpr LookupItem lookup_assignment[] = {
            LookupItem { Category::Equal, COMMAND_SET },
            LookupItem { Category::EqualHash, COMMAND_CSET },
            LookupItem { Category::PlusEqual, COMMAND_ADD_THING_TO_THING },
            LookupItem { Category::MinusEqual, COMMAND_SUB_THING_FROM_THING },
            LookupItem { Category::StarEqual, COMMAND_MULT_THING_BY_THING },
            LookupItem { Category::SlashEqual, COMMAND_DIV_THING_BY_THING },
            LookupItem { Category::PlusEqualAt, COMMAND_ADD_THING_TO_THING_TIMED },
            LookupItem { Category::MinusEqualAt, COMMAND_SUB_THING_FROM_THING_TIMED },
        };

        static constexpr LookupItem lookup_conditional[] = {
            LookupItem { Category::Equal, COMMAND_IS_THING_EQUAL_TO_THING },
            LookupItem { Category::Less, COMMAND_IS_THING_GREATER_THAN_THING },
            LookupItem { Category::LessEqual, COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING },
            LookupItem { Category::Greater, COMMAND_IS_THING_GREATER_THAN_THING },
            LookupItem { Category::GreaterEqual, COMMAND_IS_THING_GREATER_OR_EQUAL_TO_THING },
        };

        std::string_view command_name;
        auto a = args[0], b = args[1];

        if(is_conditional)
        {
            auto it = std::find_if(std::begin(lookup_conditional),
                                   std::end(lookup_conditional),
                                   [&](const auto& pair) { return pair.first == cats[1]; });
            if(it == std::end(lookup_conditional))
                return std::nullopt;

            command_name = it->second;

            if(it->first == Category::Less || it->first == Category::LessEqual)
                std::swap(a, b);

        }
        else
        {
            auto it = std::find_if(std::begin(lookup_assignment),
                                   std::end(lookup_assignment),
                                   [&](const auto& pair) { return pair.first == cats[1]; });
            if(it == std::end(lookup_assignment))
                return std::nullopt;

            command_name = it->second;
        }

        ir = ParserIR::create_command(src_info, command_name, arena);
        ir->command->push_arg(a, arena);
        ir->command->push_arg(b, arena);
    }
    else if(num_toks == 5
            && cats[0] == Category::Word
            && cats[1] == Category::Equal
            && cats[2] == Category::Word
            && cats[3] != Category::Word
            && cats[4] == Category::Word)
    {
        static constexpr LookupItem lookup_ternary[] = {
            LookupItem { Category::Plus, COMMAND_ADD_THING_TO_THING },
            LookupItem { Category::Minus, COMMAND_SUB_THING_FROM_THING },
            LookupItem { Category::Star, COMMAND_MULT_THING_BY_THING },
            LookupItem { Category::Slash, COMMAND_DIV_THING_BY_THING },
            LookupItem { Category::PlusAt, COMMAND_ADD_THING_TO_THING_TIMED },
            LookupItem { Category::MinusAt, COMMAND_SUB_THING_FROM_THING_TIMED },
        };

        auto it = std::find_if(std::begin(lookup_ternary), std::end(lookup_ternary),
                               [&](const auto& pair) { return pair.first == cats[3]; });

        if(it == std::end(lookup_ternary))
            return std::nullopt;

        const auto a = args[0], b = args[1], c = args[2];

        const bool is_associative = (cats[3] == Category::Plus
                                        || cats[3] == Category::Star);

        if(a->is_same_name(*b))
        {
            ir = ParserIR::create_command(src_info, it->second, arena);
            ir->command->push_arg(a, arena);
            ir->command->push_arg(c, arena);
        }
        else if(a->is_same_name(*c))
        {
            if(!is_associative)
                return std::nullopt;
            ir = ParserIR::create_command(src_info, it->second, arena);
            ir->command->push_arg(a, arena);
            ir->command->push_arg(b, arena);
        }
        else
        {
            ir = ParserIR::create_command(src_info, COMMAND_SET, arena);
            ir->command->push_arg(a, arena);
            ir->command->push_arg(b, arena);
            ir->set_next(ParserIR::create_command(src_info, it->second, arena));
            ir->next->command->push_arg(a, arena);
            ir->next->command->push_arg(c, arena);
        }
    }
    else
    {
        return std::nullopt;
    }

    assert(is_peek(Category::EndOfLine) || is_peek(Category::Whitespace));

    return LinkedIR<ParserIR>::from_ir(ir);
}
}

// INTERESTING TEST CASES:
// REPEAT
// REPEAT
// ENDREPEAT
// ENDRPEEAT
//
// IF
// // unclosed
//
// label on each unexpected place?
//
// 

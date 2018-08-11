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
constexpr auto COMMAND_GOTO = "GOTO"sv;
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
    //                      | if_statement (TODO)
    //                      | ifnot_statement (TODO)
    //                      | if_goto_statement (TODO)
    //                      | ifnot_goto_statement (TODO)
    //                      | while_statement (TODO)
    //                      | whilenot_statement (TODO)
    //                      | repeat_statement (TODO)
    //                      | require_statement ;
    //
    // empty_statement := eol ;
    //
    // expression_statement := assignment_expression eol
    //                       | conditional_expression eol ;
    //

    // TODO should var_statement be handled?
    
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
            return LinkedIR<ParserIR>::from_ir(std::move(*stmt_ir));
        return std::nullopt;
    }
    else if(is_peek(Category::Word, "{"))
    {
        return parse_scope_statement();
    }
    else if(is_peek(Category::Word))
    {
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
            {
                if(auto expr_ir = parse_assignment_expression())
                {
                    if(consume(Category::EndOfLine))
                    {
                        return *std::move(expr_ir); // FIXME clang bug
                    }
                }
                return std::nullopt;
            }
            case Category::Less:
            case Category::LessEqual:
            case Category::Greater:
            case Category::GreaterEqual:
            {
                if(auto expr_ir = parse_conditional_expression())
                {
                    if(consume(Category::EndOfLine))
                        return *std::move(expr_ir); // FIXME clang bug
                }
                return std::nullopt;
            }
            default:
            {
                if(auto stmt_ir = parse_command_statement())
                    return LinkedIR<ParserIR>::from_ir(std::move(*stmt_ir));
                return std::nullopt;
            }
        }
    }
    else if(is_peek(Category::PlusPlus) || is_peek(Category::MinusMinus))
    {
        if(auto expr_ir = parse_assignment_expression())
        {
            if(consume(Category::EndOfLine))
            {
                return *std::move(expr_ir); // FIXME clang bug
            }
        }
        return std::nullopt;
    }
    else
    {
        return std::nullopt;
    }
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
    
    auto linked_stmts = LinkedIR<ParserIR>();

    if(this->in_lexical_scope)
        return std::nullopt;

    auto open_token = consume_word("{");
    if(!open_token)
        return std::nullopt;

    if(!consume(Category::EndOfLine))
        return std::nullopt;

    // We'll set our current state as being in a lexical scope.
    // This is only restored back if we successfully reach the
    // end of the line of a closing curly.
    this->in_lexical_scope = true;

    while(!eof() && !is_peek(Category::Word, "}"))
    {
        if(auto stmt_ir = parse_statement())
            linked_stmts.splice_back(std::move(*stmt_ir));
        else
            return std::nullopt;
    }

    if(eof())
        return std::nullopt;

    auto close_token = consume_word("}");
    assert(close_token != std::nullopt);

    if(!consume(Category::EndOfLine))
        return std::nullopt;

    this->in_lexical_scope = false;

    auto open_ir = ParserIR::create_command(source_info(*open_token),
                                            open_token->lexeme, arena);
    auto close_ir = ParserIR::create_command(source_info(*close_token),
                                             close_token->lexeme, arena);
    linked_stmts.push_front(std::move(open_ir));
    linked_stmts.push_back(std::move(close_ir));
    return linked_stmts;
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

auto Parser::parse_command_statement() 
    -> std::optional<arena_ptr<ParserIR>>
{
    // command_statement := command eol ;
    
    auto result = parse_command();
    if(!result)
        return std::nullopt;

    if(!consume(Category::EndOfLine))
        return std::nullopt;

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
    return parse_expression_internal(false);
}

auto Parser::parse_conditional_expression()
    -> std::optional<LinkedIR<ParserIR>>
{
    return parse_expression_internal(true);
}

auto Parser::parse_expression_internal(bool is_conditional)
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
        if(is_conditional
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

    return LinkedIR<ParserIR>::from_ir(std::move(ir));
}
}

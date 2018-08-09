#include <gta3sc/parser.hpp>
#include <cstring>
#include <cassert>

// grammar from gta3script-specs de0049f5258dc11d3e409371a8981f8a01b28d93

namespace gta3sc
{
constexpr std::string_view COMMAND_GOTO = "GOTO";

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

    auto linked_stmts = LinkedIR<ParserIR>();

    if(is_peek(Category::Word) && peek()->lexeme.back() == ':')
    {
        auto label_def = *consume();
        label_def.lexeme.remove_suffix(1);

        if(!is_identifier(label_def))
            return std::nullopt;

        auto label_ir = ParserIR::create_label_def(source_info(label_def),
                                                   label_def.lexeme,
                                                   arena);

        auto token = consume(Category::Whitespace, Category::EndOfLine);
        if(!token)
            return std::nullopt;

        linked_stmts.push_front(std::move(label_ir));

        if(token->category == Category::EndOfLine)
            return linked_stmts;
    }

    auto resp = parse_embedded_statement();
    if(!resp)
        return std::nullopt;

    linked_stmts.splice_back(std::move(*resp));
    return linked_stmts;
}

auto Parser::parse_embedded_statement()
    -> std::optional<LinkedIR<ParserIR>>
{
    // embedded_statement := empty_statement
    //                      | command_statement
    //                      | expression_statement (TODO)
    //                      | scope_statement
    //                      | var_statement (TODO)
    //                      | if_statement (TODO)
    //                      | ifnot_statement (TODO)
    //                      | if_goto_statement (TODO)
    //                      | ifnot_goto_statement (TODO)
    //                      | while_statement (TODO)
    //                      | whilenot_statement (TODO)
    //                      | repeat_statement (TODO)
    //                      | require_statement (TODO) ;

    if(is_peek(Category::EndOfLine))
    {
        return LinkedIR<ParserIR>();
    }
    else if(is_peek(Category::Word, "{"))
    {
        return parse_scope_statement();
    }
    else
    {
        auto stmt_ir = parse_command_statement();
        if(!stmt_ir)
            return std::nullopt;
        return LinkedIR<ParserIR>::from_ir(std::move(*stmt_ir));
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

    auto eol = consume(Category::EndOfLine);
    if(!eol)
        return std::nullopt;

    this->in_lexical_scope = true;

    while(!is_peek(Category::Word, "}"))
    {
        auto stmt_ir = parse_statement();
        if(!stmt_ir)
            return std::nullopt;
        linked_stmts.splice_back(std::move(*stmt_ir));
    }

    auto close_token = consume_word("}");
    assert(close_token != std::nullopt);

    eol = consume(Category::EndOfLine);
    if(!eol)
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

    arena_ptr<arena_ptr<ParserIR::Argument>> args = nullptr;
    size_t acaps = 0;
    size_t acount = 0;

    while(!is_peek(Category::EndOfLine))
    {
        if(is_if_line
           && is_peek(Category::Whitespace, 0)
           && is_peek(Category::Word, COMMAND_GOTO, 1)
           && is_peek(Category::Whitespace, 2)
           && is_peek(Category::Word, 3)
           && is_peek(Category::EndOfLine, 4))
            break;

        if(!consume(Category::Whitespace))
            return std::nullopt;
        
        auto arg = parse_argument();
        if(!arg)
            return std::nullopt;

        if(acount >= acaps)
        {
            auto new_caps = !acaps? 6 : acaps * 2;
            auto new_args = new (arena) arena_ptr<ParserIR::Argument>[new_caps];
            std::copy(args, args + acount, new_args);
            acaps = new_caps;
            args = new_args;
        }

        args[acount++] = arg.value();
    }

    if(peek() == std::nullopt)
        return std::nullopt;

    auto result = ParserIR::create_command(source_info(*command),
                                           command->lexeme,
                                           arena);

    auto& command_data = std::get<ParserIR::Command>(result->op);
    command_data.arguments = args;
    command_data.num_arguments = acount;

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
}

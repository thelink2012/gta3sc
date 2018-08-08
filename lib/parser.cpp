#include <gta3sc/parser.hpp>
#include <cstring>
#include <cassert>

// grammar from gta3script-specs de0049f5258dc11d3e409371a8981f8a01b28d93

namespace gta3sc
{
constexpr std::string_view COMMAND_GOTO = "GOTO";

void Parser::skip_current_line()
{
    while(true)
    {
        auto token = this->next();
        if(token && token->category == Category::EndOfLine)
            break;
    }
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

    if(it != token.lexeme.end() && *it == '-')
        ++it;

    if(it == token.lexeme.end())
        return false;
    
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
        while(it != token.lexeme.end() && is_digit(*it))
            ++it;

        if(it == token.lexeme.end() || (*it != '.' && *it != 'f' && *it != 'F'))
            return false;
        ++it;
    }
    else // not a floating point
    {
        return false;
    }

    for(; it != token.lexeme.end(); ++it)
    {
        if(is_digit(*it))
            continue;
        else if(*it == '.' || *it == 'f' || *it == 'F')
            continue;
        else
            return false;
    }

    return true;
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
            if(back != ':')
                return true;
        }
    }

    return false;
}

bool Parser::is_digit(char c) const
{
    return c >= '0' && c <= '9';
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

    while(peek() && peek()->category != Category::EndOfLine)
    {
        if(is_if_line
           && peek() && peek()->category == Category::Whitespace
           && peek(1)->category == Category::Word 
           && peek(1)->lexeme == COMMAND_GOTO
           && peek(2) && peek(2)->category == Category::Whitespace
           && peek(3) && peek(3)->category == Category::Word
           && peek(4) && peek(4)->category == Category::EndOfLine)
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

    const auto source_info = ParserIR::SourceInfo {
        this->source_file(),
        command->lexeme,
    };

    auto result = ParserIR::create_command(source_info, command->lexeme, arena);

    auto& command_data = std::get<ParserIR::Command>(result->op);
    command_data.arguments = args;
    command_data.num_arguments = acount;

    assert(peek()->category == Category::EndOfLine
            || peek()->category == Category::Whitespace);

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

    const auto source_info = ParserIR::SourceInfo {
        this->source_file(),
        token->lexeme,
    };

    if(token->category == Category::String)
    {
        auto string = token->lexeme;
        string.remove_prefix(1);
        string.remove_suffix(1);
        return ParserIR::create_string(source_info, string, arena);
    }
    else if(is_integer(*token))
    {
        constexpr long min_value = -2147483648, max_value = 2147483647;

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

        return ParserIR::create_integer(source_info, value, arena);
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

        return ParserIR::create_float(source_info, value, arena);
    }
    else if(is_identifier(*token))
    {
        return ParserIR::create_identifier(source_info, token->lexeme, arena);
    }
    else
    {
        return std::nullopt;
    }
}
}

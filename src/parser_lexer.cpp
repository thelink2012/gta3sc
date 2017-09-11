#include <stdinc.h>
#include "parser.hpp"
#include "program.hpp"

using TokenData = TokenStream::TokenData;

struct LexerContext
{
    ProgramContext& program;
    TokenStream::TextStream stream;

    std::vector<char> cpp_stack;

    bool any_error = false;                 //< True if any error happened during tokenization.
    bool in_dump_mode = false;              //< True if inside a DUMP...ENDDUMP block.
    size_t comment_nest_level = 0;          //< Nest level of /* comments */
    std::vector<TokenData> tokens;          //< Output tokens.
    std::string            line_buffer;     //< Buffer used to parse a line, since we'll be mutating the line.

    explicit LexerContext(ProgramContext& program, std::string data, std::string stream_name) :
        program(program), stream(std::move(data), std::move(stream_name))
    {
        cpp_stack.reserve(32);
        cpp_stack.emplace_back(true);
    }

    void verify_nesting()
    {
        if(this->comment_nest_level != 0)
            this->error(std::make_pair(0, 0), "unterminated /* comment");

        if(cpp_stack.size() > 1)
            this->error(std::make_pair(0, 0), "missing #endif");
    }

    void add_token(Token type, size_t begin_pos, size_t length)
    {
        this->tokens.emplace_back(TokenData{ type, begin_pos, begin_pos + length });
    }

    void hint_will_push_tokens(size_t count)
    {
        this->tokens.reserve(this->tokens.size() + count);
    }

    template<typename... Args>
    void error(std::pair<size_t, size_t> pos, Args&&... args) // pos = <begin_pos, size>
    {
        this->any_error = true;
        this->program.error(TokenStream::TokenInfo(this->stream, pos.first, pos.first + pos.second),
            std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(size_t pos, Args&&... args)
    {
        return error(std::make_pair(pos, 1), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void pedantic(std::pair<size_t, size_t> pos, Args&&... args) // pos = <begin_pos, size>
    {
        if(program.opt.pedantic)
        {
            this->program.pedantic(TokenStream::TokenInfo(this->stream, pos.first, pos.first + pos.second),
                std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void pedantic(size_t pos, Args&&... args)
    {
        return pedantic(std::make_pair(pos, 1), std::forward<Args>(args)...);
    }
};

#define DEFINE_TOKEN(kw)          { #kw, Token :: kw }
#define DEFINE_SYMBOL(sym, tok)   { sym, tok }

static const std::pair<string_view, Token> keycommands[] = {
    DEFINE_SYMBOL("{", Token::ScopeBegin),
    DEFINE_SYMBOL("}", Token::ScopeEnd),
    //DEFINE_TOKEN(NOT),
    //DEFINE_TOKEN(AND),
    //DEFINE_TOKEN(OR),
    //DEFINE_TOKEN(IF),
    DEFINE_TOKEN(ELSE),
    DEFINE_TOKEN(ENDIF),
    //DEFINE_TOKEN(WHILE),
    DEFINE_TOKEN(ENDWHILE),
    DEFINE_TOKEN(REPEAT),
    DEFINE_TOKEN(ENDREPEAT),
    DEFINE_TOKEN(SWITCH),
    DEFINE_TOKEN(ENDSWITCH),
    DEFINE_TOKEN(CASE),
    DEFINE_TOKEN(DEFAULT),
    DEFINE_TOKEN(BREAK),
    DEFINE_TOKEN(CONTINUE),
    DEFINE_TOKEN(MISSION_START),
    DEFINE_TOKEN(MISSION_END),
    DEFINE_TOKEN(SCRIPT_START),
    DEFINE_TOKEN(SCRIPT_END),
    DEFINE_TOKEN(VAR_INT),
    DEFINE_TOKEN(LVAR_INT),
    DEFINE_TOKEN(VAR_FLOAT),
    DEFINE_TOKEN(LVAR_FLOAT),
    DEFINE_TOKEN(VAR_TEXT_LABEL),
    DEFINE_TOKEN(LVAR_TEXT_LABEL),
    DEFINE_TOKEN(VAR_TEXT_LABEL16),
    DEFINE_TOKEN(LVAR_TEXT_LABEL16),
    DEFINE_TOKEN(CONST_INT),
    DEFINE_TOKEN(CONST_FLOAT),

    // Extensions
    DEFINE_TOKEN(DUMP),
    DEFINE_TOKEN(ENDDUMP),
};

static const std::pair<string_view, Token> expr_symbols[] = {
    // Order matters (by length)
    // 3-length
    DEFINE_SYMBOL("+=@", Token::EqTimedPlus),
    DEFINE_SYMBOL("-=@", Token::EqTimedMinus),
    // 2-length
    DEFINE_SYMBOL("+=", Token::EqPlus),
    DEFINE_SYMBOL("-=", Token::EqMinus),
    DEFINE_SYMBOL("*=", Token::EqTimes),
    DEFINE_SYMBOL("/=", Token::EqDivide),
    DEFINE_SYMBOL("++", Token::Increment),
    DEFINE_SYMBOL("--", Token::Decrement),
    DEFINE_SYMBOL("=#", Token::EqCast),
    DEFINE_SYMBOL("+@", Token::TimedPlus),
    DEFINE_SYMBOL("-@", Token::TimedMinus),
    DEFINE_SYMBOL("<=", Token::LesserEqual),
    DEFINE_SYMBOL(">=", Token::GreaterEqual),
    // 1-length
    DEFINE_SYMBOL("=", Token::Equal),
    DEFINE_SYMBOL("+", Token::Plus),
    DEFINE_SYMBOL("-", Token::Minus),
    DEFINE_SYMBOL("*", Token::Times),
    DEFINE_SYMBOL("/", Token::Divide),
    DEFINE_SYMBOL("<", Token::Lesser),
    DEFINE_SYMBOL(">", Token::Greater),
};

/// Checks if `c` is a whitespace.
static bool lex_iswhite(int c)
{
    return c == ' ' || c == '\t' || c == '(' || c == ')' || c == ',' || c == '\r';
}

/// Fast alternative to ::isspace.
static bool lex_isspace2(int c)
{
    return c == ' ' || c == '\t';
}

/// Fast alternative to ::isxdigit
static bool lex_isxdigit(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/// Checks if `token` is equal `string` which has `length`.
static bool lex_istokeq(const std::pair<const char*, size_t>& token, const char* string, size_t length)
{
    return (token.second == length && !strncasecmp(token.first, string, length));
}

/// Checks if `token` is equal to the string literal `string`.
template<std::size_t N>
static bool lex_istokeq(const std::pair<const char*, size_t>& token, const char(&string)[N])
{
    static_assert(N > 0, "");
    return lex_istokeq(token, string, N - 1);
}

/// Checks if `token` is equal to the string in the view `string`.
static bool lex_istokeq(const std::pair<const char*, size_t>& token, const string_view& string)
{
    return lex_istokeq(token, string.data(), string.size());
}

/// Checks if `it` is part of a expression token.
static bool lex_isexprc(const char* it, const char* end)
{
    if(*it == '-')
    {
        auto next = std::next(it);
        if(next != end)
        {
            if(*next == '.' || (*next >= '0' && *next <= '9'))
                return false;
        }
        return true;
    }
    return (*it == '+' || *it == '*' || *it == '/' || *it == '=' || *it == '<' || *it == '>');
}

/// Gets the next whitespace delimited token in `it`.
static auto lex_gettok(const char* it, const char* end) -> optional<std::pair<const char*, size_t>>
{
    auto tok_start = std::find_if_not(it, end, lex_iswhite);
    auto tok_end = std::find_if(tok_start, end, lex_iswhite);

    if(tok_start != tok_end)
        return std::make_pair(tok_start, size_t(tok_end - tok_start));

    return nullopt;
}

/// Reads the next token (number/identifier/string) from `begin` into the `lexer` state.
static auto lex_token(LexerContext& lexer, const char* begin, const char* end, size_t begin_pos) -> const char*
{
    while(begin != end && lex_iswhite(*begin))
        ++begin, ++begin_pos;

    if(begin == end)
        return end;

    auto have_hexadecimal_prefix = [](const std::pair<const char*, size_t>& token)
    {
        return (token.second > 2 && token.first[0] == '0' && (token.first[1] == 'x' || token.first[1] == 'X'));
    };

    auto is_integer = [&have_hexadecimal_prefix](const std::pair<const char*, size_t>& token)
    {
        bool is_hexa = have_hexadecimal_prefix(token);

        for(size_t i = (is_hexa? 2 : 0); i < token.second; ++i)
        {
            if(token.first[i] < '0' || token.first[i] > '9')
            {
                if(is_hexa && ((token.first[i] >= 'A' && token.first[i] <= 'F')
                    || (token.first[i] >= 'a' && token.first[i] <= 'f')))
                {
                    continue;
                }
                else if(token.first[i] == '-')
                {
                    // valid even if i != 0
                    continue;
                }
                return false;
            }
        }
        return true;
    };

    auto is_float = [](const std::pair<const char*, size_t>& token)
    {
        for(size_t i = 0; i < token.second; ++i)
        {
            if(token.first[i] < '0' || token.first[i] > '9')
            {
                if(token.first[i] == '.' || (token.first[i] == '-' && i == 0))
                    continue;

                if(token.first[i] == 'f' || token.first[i] == 'F')
                {
                    // valid even if not in the end of the token
                    continue;
                }

                return false;
            }
        }
        return true;
    };

    if(*begin == '"')
    {
        auto it = std::find(std::next(begin), end, '"');
        if(it == end)
        {
            lexer.error(begin_pos, "missing terminating '\"' character");
            return end;
        }
        lexer.add_token(Token::String, begin_pos, std::distance(begin, it) + 1);
        return std::next(it);
    }

    if((*begin >= '0' && *begin <= '9') || *begin == '-' || *begin == '.')
    {
        auto token = lex_gettok(begin, end).value();

        if(is_integer(token))
        {
            if(lexer.program.opt.pedantic && have_hexadecimal_prefix(token))
                lexer.pedantic({begin_pos, token.second}, "hexadecimal integer literals are a language extension [-pedantic]");

            lexer.add_token(Token::Integer, begin_pos, token.second);
            return token.first + token.second;
        }
        else if(is_float(token))
        {
            lexer.add_token(Token::Float, begin_pos, token.second);
            return token.first + token.second;
        }
    }

    auto token = lex_gettok(begin, end).value();
    auto it = token.first + token.second;
    lexer.add_token(Token::Text, begin_pos, std::distance(begin, it));
    return it;
}

/// Lexes a command context.
static void lex_command(LexerContext& lexer, const char* begin, const char* end, size_t begin_pos, bool had_keycommand)
{
    if(auto cmdtok = lex_gettok(begin, end))
    {
        bool is_keycommand = false;

        auto it = cmdtok->first + cmdtok->second;

        for(auto& keycmd : keycommands)
        {
            if(lex_istokeq(*cmdtok, keycmd.first))
            {
                size_t tok_begin = begin_pos + std::distance(begin, cmdtok->first);

                if(had_keycommand)
                    lexer.error(std::make_pair(tok_begin, cmdtok->second), "unexpected token");

                lexer.add_token(keycmd.second, tok_begin, cmdtok->second);
                is_keycommand = true;
                break;
            }
        }

        if(!is_keycommand)
        {
            size_t tok_begin = begin_pos + std::distance(begin, cmdtok->first);
            lexer.add_token(Token::Command, tok_begin, cmdtok->second);
        }

        for(size_t id = 1; it != end; ++id)
        {
            it = lex_token(lexer, it, end, begin_pos + std::distance(begin, it));
        }
    }
}

/// Lexes a expression context.
static void lex_expr(LexerContext& lexer, const char* begin, const char* end, size_t begin_pos)
{
    for(auto it = begin; ;)
    {
        it = std::find_if_not(it, end, lex_iswhite);

        if(it == end)
            break;

        bool found_symbol = false;
        auto length_left = size_t(std::distance(it, end));

        for(auto& symbol : expr_symbols)
        {
            if(length_left >= symbol.first.size()
                && lex_isexprc(it, end) // for special minus checking
                && !strncmp(it, symbol.first.data(), symbol.first.size()))
            {
                size_t pos = (begin_pos + std::distance(begin, it));
                lexer.add_token(symbol.second, pos, symbol.first.size());
                it = std::next(it, symbol.first.size());
                found_symbol = true;
                break;
            }
        }

        if(!found_symbol)
        {
            // Cannot use lex_gettok due to it not being only lex_iswhite delimited, but also lex_exprc delimited.
            auto tok_begin = it;
            while(it != end && !lex_iswhite(*it) && !lex_isexprc(it, end)) ++it;
            auto tok_end = it;

            it = lex_token(lexer, tok_begin, tok_end, begin_pos + std::distance(begin, tok_begin));
        }
    }
}

/// Lexes a dump context.
static void lex_dump(LexerContext& lexer, const char* begin, const char* end, size_t begin_pos)
{
    auto it = begin;

    lexer.hint_will_push_tokens(std::distance(begin, end) / 3);

    while(auto next_token = lex_gettok(it, end))
    {
        size_t tok_begin = begin_pos + std::distance(begin, next_token->first);

        // Parses a string
        if(next_token->first[0] == '"')
        {
            it = lex_token(lexer, next_token->first, end, begin_pos);
            continue;
        }
        else if((next_token->second % 2) != 0)
        {
            lexer.error({tok_begin, tok_begin + next_token->second}, "hexadecimal tokens must have pairs of two digits");
        }
        else if(!std::all_of(next_token->first, next_token->first + next_token->second, lex_isxdigit))
        {
            lexer.error({ tok_begin, tok_begin + next_token->second }, "invalid hexadecimal token");
        }
        else
        {
            for(size_t xpos = 0; xpos < next_token->second; xpos += 2)
            {
                auto xpair = next_token->first + xpos;
                if(!lex_isxdigit(xpair[0]) || !lex_isxdigit(xpair[1]))
                {
                    lexer.error({ tok_begin + xpos, tok_begin + xpos + 2 }, "invalid hexadecimal token");
                }
                else
                {
                    lexer.add_token(Token::Hexadecimal, tok_begin + xpos, 2);
                }
            }
        }

        it = std::find_if_not(next_token->first + next_token->second, end, lex_iswhite);
    }
}

/// Tranforms comments into whitespaces.
static void lex_comments(LexerContext& lexer, char* begin, char* end, size_t begin_pos)
{
    bool in_quotes = false;

    for(auto it = begin; it != end; ++it)
    {
        if(*it == '"' && lexer.comment_nest_level == 0)
        {
            in_quotes = !in_quotes;
        }
        else if(std::next(it) != end)
        {
            if(in_quotes)
            {
                // Cannot have a comment inside a quoted string.
            }
            else if(*it == '/' && *std::next(it) == '/')
            {
                std::memset(it, ' ', end - it);
                return;
            }
            else if(*it == '/' && *std::next(it) == '*')
            {
                ++lexer.comment_nest_level;
                *it++ = ' '; *it = ' ';
            }
            else if(*it == '*' && *std::next(it) == '/')
            {
                if(lexer.comment_nest_level == 0)
                {
                    lexer.error(begin_pos + std::distance(begin, it), "no comment to close");
                    ++it;
                }
                else
                {
                    --lexer.comment_nest_level;
                    *it++ = ' '; *it = ' ';
                }
            }
        }

        if(lexer.comment_nest_level)
            *it = ' ';
    }
}

/// Processes the mini-preprocessor.
///
/// Returns true in case we can keep reading this line, false otherwise.
static bool lex_cpp(LexerContext& lexer, char* begin, char* end, size_t begin_pos)
{
    auto next_char_it = std::find_if_not(begin, end, lex_isspace2);
    if(next_char_it != end && *next_char_it == '#')
    {
        size_t line_pos = begin_pos + std::distance(begin, next_char_it);

        if(lexer.program.opt.pedantic)
            lexer.pedantic(line_pos, "preprocessor is a language extension [-pedantic]");

        if(auto opt_command = lex_gettok(next_char_it + 1, end))
        {
            string_view command(opt_command->first, opt_command->second);
            std::vector<string_view> tokens;

            auto next_token = opt_command;
            while(next_token = lex_gettok(next_token->first + next_token->second, end))
            {
                tokens.emplace_back(next_token->first, next_token->second);
            }

            if(command == "ifdef" || command == "ifndef")
            {
                if(tokens.size() != 1)
                    lexer.error(line_pos, "wrong number of tokens in preprocessor directive");

                if(tokens.size() >= 1)
                {
                    auto top = lexer.cpp_stack.back();
                    auto isdef = lexer.program.opt.is_defined(tokens[0]);
                    lexer.cpp_stack.emplace_back((command[2] == 'n'? !isdef : isdef) && top);
                }
            }
            else if(command == "else")
            {
                if(tokens.size())
                    lexer.error(line_pos, "too many tokens in preprocessor directive");

                if(lexer.cpp_stack.size() > 1)
                {
                    auto topmost = lexer.cpp_stack[lexer.cpp_stack.size() - 2];
                    if(topmost) // if the #if before the #if being checked is active
                    {
                        lexer.cpp_stack.back() = !(lexer.cpp_stack.back());
                    }
                }
                else
                    lexer.error(line_pos, "#else without #ifdef");
            }
            else if(command == "endif")
            {
                if(tokens.size())
                    lexer.error(line_pos, "too many tokens in preprocessor directive");

                if(lexer.cpp_stack.size() > 1)
                    lexer.cpp_stack.pop_back();
                else
                    lexer.error(line_pos, "#endif without #ifdef");
            }
            else
            {
                lexer.error(line_pos, "unknown preprocessor directive");
            }
        }
        return false;
    }
    return (!!lexer.cpp_stack.back());
}

/// Lexes a line.
static void lex_line(LexerContext& lexer, const char* source_data, size_t begin_pos, size_t end_pos)
{
    lexer.line_buffer.assign(source_data + begin_pos, source_data + end_pos);

    bool had_keycommand = false;

    auto begin = (const char*)(&lexer.line_buffer[0]);
    auto end = begin + lexer.line_buffer.size();
    auto it = begin;

    auto push_token = [&](const std::pair<const char*, size_t>& token, Token type) -> const char*
    {
        size_t tok_begin = begin_pos + std::distance(begin, token.first);
        lexer.add_token(type, tok_begin, token.second);
        return std::find_if_not(token.first + token.second, end, lex_iswhite);
    };

    auto push_newline = [&]()
    {
        lexer.add_token(Token::NewLine, end_pos, 0);
    };

    lex_comments(lexer, &lexer.line_buffer[0], &lexer.line_buffer[0] + lexer.line_buffer.size(), begin_pos);

    if(!lex_cpp(lexer, &lexer.line_buffer[0], &lexer.line_buffer[0] + lexer.line_buffer.size(), begin_pos))
        return;

    it = std::find_if_not(it, end, lex_iswhite);
    end = std::find_if_not(std::make_reverse_iterator(end), std::make_reverse_iterator(it), lex_iswhite).base();

    if(std::distance(it, end) == 0)
        return;

    if(lexer.program.opt.pedantic && lexer.line_buffer.size() > 255)
    {
        lexer.pedantic(begin_pos, "line is too long, miss2 only allows 255 characters [-pedantic]");
    }

    if(lexer.in_dump_mode)
    {
        if(auto opt_first_token = lex_gettok(it, end))
        {
            if(!lex_istokeq(*opt_first_token, "ENDDUMP"))
            {
                lex_dump(lexer, it, end, begin_pos + std::distance(begin, it));
                push_newline();
                return;
            }
        }
    }

    if(auto opt_first_token = lex_gettok(it, end))
    {
        if(opt_first_token->first[opt_first_token->second - 1] == ':')
        {
            it = push_token(*opt_first_token, Token::Label);
        }
    }

    if(auto opt_first_token = lex_gettok(it, end))
    {
        if(lex_istokeq(*opt_first_token, "DUMP"))
        {
            // further token pushing happens in lex_command.
            lexer.in_dump_mode = true;
        }
        else if(lex_istokeq(*opt_first_token, "ENDDUMP"))
        {
            // ditto.
            lexer.in_dump_mode = false;
        }
    }

    if(auto opt_first_token = lex_gettok(it, end))
    {
        if(lex_istokeq(*opt_first_token, "IF"))
        {
            it = push_token(*opt_first_token, Token::IF);
            had_keycommand = true;
        }
        else if(lex_istokeq(*opt_first_token, "WHILE"))
        {
            it = push_token(*opt_first_token, Token::WHILE);
            had_keycommand = true;
        }
    }

    if(auto opt_first_token = lex_gettok(it, end))
    {
        if(lex_istokeq(*opt_first_token, "AND"))
        {
            it = push_token(*opt_first_token, Token::AND);
            had_keycommand = true;
        }
        else if(lex_istokeq(*opt_first_token, "OR"))
        {
            it = push_token(*opt_first_token, Token::OR);
            had_keycommand = true;
        }
    }

    if(auto opt_first_token = lex_gettok(it, end))
    {
        if(lex_istokeq(*opt_first_token, "NOT"))
        {
            it = push_token(*opt_first_token, Token::NOT);
            had_keycommand = true;
        }
    }

    if(it != end)
    {
        auto begin_expr = it;

        if(lex_isexprc(it, end))
        {
            lex_expr(lexer, begin_expr, end, begin_pos + std::distance(begin, begin_expr));
            push_newline();
            return;
        }
        else
        {
            // <identifier>
            while(it != end && !lex_iswhite(*it) && !lex_isexprc(it, end))
                ++it;

            it = std::find_if_not(it, end, lex_iswhite);

            // <expression_token>
            if(it != end && lex_isexprc(it, end))
            {
                lex_expr(lexer, begin_expr, end, begin_pos + std::distance(begin, begin_expr));
                push_newline();
                return;
            }
        }

        it = begin_expr;
    }

    if(it != end)
    {
        lex_command(lexer, it, end, begin_pos + std::distance(begin, it), had_keycommand);
        push_newline();
        return;
    }

    // it == end
    if(had_keycommand)
    {
        push_newline();
        return;
    }
}


//
// TokenStream
//

std::shared_ptr<TokenStream> TokenStream::tokenize(ProgramContext& program, std::string data_, const char* stream_name)
{
    LexerContext lexer(program, std::move(data_), stream_name);

    auto begin = lexer.stream.data.c_str();
    auto end = lexer.stream.data.c_str() + lexer.stream.data.size();

    for(auto it = begin; it != end; )
    {
        const char *line_start = it;
        const char *line_end   = std::find(it, end, '\n');

        lex_line(lexer, begin, std::distance(begin, line_start), std::distance(begin, line_end));
        it = (line_end == end? line_end : std::next(line_end));
    }

    lexer.verify_nesting();

    if(!lexer.any_error)
        return shared_ptr<TokenStream>(new TokenStream(program, std::move(lexer.stream), std::move(lexer.tokens)));
    else
        return nullptr;
}

std::shared_ptr<TokenStream> TokenStream::tokenize(ProgramContext& program, const fs::path& path)
{
    if(auto opt_data = read_file_utf8(path))
    {
        return TokenStream::tokenize(program, *opt_data, path.generic_u8string().c_str());
    }
    else
    {
        program.error(nocontext, "failed to read file '{}'", path.generic_u8string());
        return nullptr;
    }
}

TokenStream::TokenStream(ProgramContext& program, const char* stream_name, std::string data, std::vector<TokenData> tokens)
    : program(program), text(std::move(data), stream_name), tokens(std::move(tokens))
{
}

TokenStream::TokenStream(ProgramContext& program, TextStream stream, std::vector<TokenData> tokens)
    : program(program), text(std::move(stream)), tokens(std::move(tokens))
{
}

TokenStream::TokenStream(TokenStream&& rhs)
    : program(rhs.program), tokens(std::move(rhs.tokens)), text(std::move(rhs.text))
{
    Ensures(&rhs.program == &this->program);
}

TokenStream::TextStream::TextStream(std::string data_, std::string name_)
    : data(std::move(data_)), stream_name(std::move(name_))
{
    this->max_offset = this->data.size();
    if(!this->data.empty())
    {
        // Assumes an average of 100 characters per line at first.
        this->line_offset.reserve(this->data.size() / 100);

        // pushes first line offset
        this->line_offset.emplace_back(0);

        for(const char* it = this->data.c_str(); *it; ++it)
        {
            if(*it == '\n')
            {
                size_t pos = (it - this->data.c_str());
                this->line_offset.emplace_back(pos + 1);
            }
        }

        this->line_offset.shrink_to_fit();
    }
}

std::string TokenStream::TextStream::get_line(size_t lineno) const
{
    size_t offset = offset_for_line(lineno);

    const char* start = this->data.c_str() + offset;
    const char* end;

    for(end = start; *end && *end != '\n' && *end != '\r'; ++end) {
    }

    return std::string(start, end);
}

size_t TokenStream::TextStream::offset_for_line(size_t lineno) const
{
    size_t i = (lineno - 1);
    if(i < line_offset.size())
        return line_offset[i];
    else
        throw std::logic_error("bad `lineno` in TokenStream::offset_for_line");
}

auto TokenStream::TextStream::linecol_from_offset(size_t offset) const -> std::pair<size_t, size_t>
{
    for(auto it = line_offset.begin(), end = line_offset.end(); it != end; ++it)
    {
        auto next_it = std::next(it);
        size_t next_line_offset = (next_it != end)? *next_it : this->max_offset;

        if(offset >= *it && offset < next_line_offset)
        {
            size_t lineno = size_t(std::distance(line_offset.begin(), it) + 1);
            size_t colno = (offset - *it) + 1;
            return std::make_pair(lineno, colno);
        }
    }

    throw std::logic_error("bad offset on linecol_from_offset");
}

string_view TokenStream::TextStream::get_text(size_t begin, size_t end) const
{
    Expects(begin <= end && end <= this->data.size());
    auto data = this->data.c_str();
    return string_view(data + begin, end - begin);
}

std::string TokenStream::to_string() const
{
    std::string output;
    for(auto& token : this->tokens)
    {
        auto string = std::string(this->text.data.c_str() + token.begin, this->text.data.c_str() + token.end);
        output += fmt::format("({}) '{}'\n", (int)(token.type), string);
    }
    return output;
}

auto Miss2Identifier::match(const string_view& value, const Options& options) -> expected<Miss2Identifier, Error>
{
    size_t begin_index = std::string::npos;
    bool is_number_index = true;

    if(!Miss2Identifier::is_identifier(value, options))
        return make_unexpected(Miss2Identifier::InvalidIdentifier);

    for(size_t i = 0; i < value.size(); ++i)
    {
        if(value[i] == '[')
        {
            if(begin_index != std::string::npos)
                return make_unexpected(Miss2Identifier::NestingOfArrays);

            begin_index = i;
        }
        else if(value[i] == ']')
        {
            auto ident = value.substr(0, begin_index);
            auto index = value.substr(begin_index + 1, i - (begin_index + 1));
            try
            {
                using index_type = decltype(Miss2Identifier::index);
                if(is_number_index)
                {
                    int index_value = std::stoi(index.to_string());
                    if(index_value >= 0)
                        return Miss2Identifier{ ident, index_type(index_value) };
                    else
                        return make_unexpected(Miss2Identifier::NegativeIndex);
                }
                else
                    return Miss2Identifier{ ident, index_type(index) };
            }
            catch(const std::out_of_range&)
            {
                return make_unexpected(Miss2Identifier::OutOfRange);
            }
        }
        else if(begin_index != std::string::npos)
        {
            if(!((value[i] >= '0' && value[i] <= '9') || (value[i] == '-')))
                is_number_index = false;
        }
    }

    return Miss2Identifier{ value, nullopt };
}

bool Miss2Identifier::is_identifier(const string_view& value, const Options& options)
{
    auto first_char = value.empty()? '\0' : value.front();
    auto last_char = value.empty()? '\0' : value.back();

    if(last_char == ':')
        return false;
    else if(options.allow_underscore_identifiers && first_char == '_')
        return true;
    else
        return (first_char >= 'a' && first_char <= 'z') || (first_char >= 'A' && first_char <= 'Z') || first_char == '$';
}

optional<int32_t> to_integer(const SyntaxTree& node, ProgramContext& program)
{
    Expects(node.type() == NodeType::Integer);

    try
    {
        auto number = node.text();

        if(number.size() > 2+7 && number[0] == '0' && (number[1] == 'x' || number[1] == 'X'))
        {
            auto ll = std::stoll(number.to_string(), 0, 0);
            static_assert(sizeof(ll) == sizeof(int32_t) * 2, "");

            if(ll > std::numeric_limits<int32_t>::max())
            {
                if(ll > std::numeric_limits<uint32_t>::max())
                    throw std::out_of_range("out of range");

                // long long is inside the unsigned int range
                return static_cast<int32_t>(static_cast<uint32_t>(ll));
            }
            else
            {
                if(ll < std::numeric_limits<int32_t>::min())
                    throw std::out_of_range("out of range");

                // long long is inside the signed int range.
                return static_cast<int32_t>(ll);
            }
        }
        else
        {
            return std::stoi(number.to_string(), 0, 0);
        }
    }
    catch(const std::out_of_range&)
    {
        program.error(node, "integer is out of range");
        return nullopt;
    }
}

optional<float> to_float(const SyntaxTree& node, ProgramContext& program)
{
    Expects(node.type() == NodeType::Float);

    try
    {
        return std::stof(node.text().to_string());
    }
    catch(const std::out_of_range&)
    {
        program.error(node, "float is out of range");
        return nullopt;
    }
}

#include <cassert>
#include <gta3sc/scanner.hpp>

namespace gta3sc
{
auto Scanner::source_file() const -> const SourceFile&
{
    return pp.source_file();
}

auto Scanner::location() const -> SourceLocation
{
    auto loc = pp.location();
    if(peek_char)
        --loc;
    return loc;
}

bool Scanner::eof() const
{
    if(peek_char)
        return false;
    return pp.eof();
}

char Scanner::getc()
{
    return std::exchange(peek_char, pp.next());
}

bool Scanner::is_whitespace(char c) const
{
    return c == ' ' || c == '\t' || c == '(' || c == ')' || c == ',';
}

bool Scanner::is_newline(char c) const
{
    return c == '\r' || c == '\n' || c == '\0';
}

bool Scanner::is_digit(char c) const
{
    return c >= '0' && c <= '9';
}

bool Scanner::is_print(char c) const
{
    return c >= 32 && c <= 126; // printable ASCII
}

bool Scanner::is_word_char(char c) const
{
    if(is_print(c))
    {
        return !is_whitespace(c) && c != '"' && c != '+' && c != '-' && c != '*'
               && c != '/' && c != '=' && c != '<' && c != '>';
    }
    return false;
}

auto Scanner::next_filename() -> std::optional<Token>
{
    if(!peek_char)
        getc(); // first cycle

    auto start_pos = this->location();

    while(is_print(peek_char) && peek_char != '"' && !is_whitespace(peek_char))
        this->getc();

    auto token = Token(Category::Word, start_pos, location());
    if(token.spelling().size() >= 3)
    {
        auto it = token.spelling().rbegin();
        auto c2 = *it++;
        auto c1 = *it++;
        auto c0 = *it++;
        if(c0 == '.' && (c1 == 's' || c1 == 'S') && (c2 == 'c' || c2 == 'C'))
        {
            return token;
        }
    }
    return std::nullopt;
}

auto Scanner::next() -> std::optional<Token>
{
    if(!peek_char)
        getc(); // first cycle

    auto start_pos = this->location();

    switch(peek_char)
    {
    // clang-format off
        newline: case '\r': case '\n': case '\0':
            if(peek_char == '\r') getc();
            if(peek_char == '\n') getc();
            return Token(Category::EndOfLine, start_pos, location());

        case ' ': case '\t': case '(': case ')': case ',':
            this->getc();
            while(is_whitespace(peek_char))
                this->getc();

            // Trailing spaces should be handled as a newline.
            if(is_newline(peek_char))
                goto newline;

            return Token(Category::Whitespace, start_pos, location());

        case '-':
            this->getc();
            if(peek_char == '.')
            {
                goto word_token;
            }
            else if(is_digit(peek_char))
            {
                goto word_token;
            }
            else if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return Token(Category::MinusEqualAt, start_pos, location());
                }
                else
                {
                    return Token(Category::MinusEqual, start_pos, location());
                }
            }
            else if(peek_char == '-')
            {
                this->getc();
                return Token(Category::MinusMinus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return Token(Category::MinusAt, start_pos, location());
            }
            else
            {
                return Token(Category::Minus, start_pos, location());
            }

        case '+':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return Token(Category::PlusEqualAt, start_pos, location());
                }
                else
                {
                    return Token(Category::PlusEqual, start_pos, location());
                }
            }
            else if(peek_char == '+')
            {
                this->getc();
                return Token(Category::PlusPlus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return Token(Category::PlusAt, start_pos, location());
            }
            else
            {
                return Token(Category::Plus, start_pos, location());
            }

        case '*':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::StarEqual, start_pos, location());
            }
            else
            {
                return Token(Category::Star, start_pos, location());
            }

        case '/':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::SlashEqual, start_pos, location());
            }
            else
            {
                return Token(Category::Slash, start_pos, location());
            }

        case '=':
            this->getc();
            if(peek_char == '#')
            {
                this->getc();
                return Token(Category::EqualHash, start_pos, location());
            }
            else
            {
                return Token(Category::Equal, start_pos, location());
            }

        case '<':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::LessEqual, start_pos, location());
            }
            else
            {
                return Token(Category::Less, start_pos, location());
            }

        case '>':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::GreaterEqual, start_pos, location());
            }
            else
            {
                return Token(Category::Greater, start_pos, location());
            }
        
        case '"':
            for(getc(); peek_char != '"'; getc())
            {
                if(is_newline(peek_char))
                    return std::nullopt;
            }

            assert(peek_char == '"');
            this->getc();

            return Token(Category::String, start_pos, location());

        word_token: default:
            if(!is_word_char(peek_char))
            {
                this->getc();
                return std::nullopt;
            }

            this->getc();
            while(is_word_char(peek_char))
                this->getc();

            return Token(Category::Word, start_pos, location());

            // clang-format on
    }
}
}

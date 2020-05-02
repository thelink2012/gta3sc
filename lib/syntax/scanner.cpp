#include <cassert>
#include <gta3sc/syntax/scanner.hpp>

namespace gta3sc::syntax
{
auto Scanner::source_file() const -> const SourceFile&
{
    return pp.source_file();
}

auto Scanner::diagnostics() const -> DiagnosticHandler&
{
    return pp.diagnostics();
}

auto Scanner::spelling(const Token& token) const -> std::string_view
{
    return source_file().view_of(token.source);
}

auto Scanner::location() const -> SourceLocation
{
    auto loc = pp.location();
    if(peek_char)
        --loc;
    return loc;
}

auto Scanner::eof() const -> bool
{
    if(peek_char)
        return false;
    return pp.eof();
}

auto Scanner::getc() -> char
{
    return std::exchange(peek_char, pp.next());
}

auto Scanner::is_whitespace(char c) const -> bool
{
    return c == ' ' || c == '\t' || c == '(' || c == ')' || c == ',';
}

auto Scanner::is_newline(char c) const -> bool
{
    return c == '\r' || c == '\n' || c == '\0';
}

auto Scanner::is_digit(char c) const -> bool
{
    return c >= '0' && c <= '9';
}

auto Scanner::is_print(char c) const -> bool
{
    return c >= 32 && c <= 126; // printable ASCII
}

auto Scanner::is_word_char(char c) const -> bool
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

    auto token = Token(Category::word, start_pos, location());
    auto spell = this->spelling(token);
    if(spell.size() >= 3)
    {
        auto it = spell.rbegin();
        auto c2 = *it++;
        auto c1 = *it++;
        auto c0 = *it++;
        if(c0 == '.' && (c1 == 's' || c1 == 'S') && (c2 == 'c' || c2 == 'C'))
        {
            return token;
        }
    }

    diagnostics()
            .report(token.source.begin, Diag::invalid_filename)
            .range(token.source);

    return std::nullopt;
}

auto Scanner::next() -> std::optional<Token>
{
    if(!peek_char)
        getc(); // first cycle

    auto start_pos = this->location();

    // clang-format off
    switch(peek_char)
    {
        newline: case '\r': case '\n': case '\0':
            if(peek_char == '\r') getc();
            if(peek_char == '\n') getc();
            return Token(Category::end_of_line, start_pos, location());

        case ' ': case '\t': case '(': case ')': case ',':
            this->getc();
            while(is_whitespace(peek_char))
                this->getc();

            // Trailing spaces should be handled as a newline.
            if(is_newline(peek_char))
                goto newline; // NOLINT: modelling a finite state machine

            return Token(Category::whitespace, start_pos, location());

        case '-':
            this->getc();
            if(peek_char == '.' || is_digit(peek_char))
            {
                goto word_token; // NOLINT: modelling a finite state machine
            }
            else if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return Token(Category::minus_equal_at, start_pos, location());
                }
                else
                {
                    return Token(Category::minus_equal, start_pos, location());
                }
            }
            else if(peek_char == '-')
            {
                this->getc();
                return Token(Category::minus_minus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return Token(Category::minus_at, start_pos, location());
            }
            else
            {
                return Token(Category::minus, start_pos, location());
            }

        case '+':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return Token(Category::plus_equal_at, start_pos, location());
                }
                else
                {
                    return Token(Category::plus_equal, start_pos, location());
                }
            }
            else if(peek_char == '+')
            {
                this->getc();
                return Token(Category::plus_plus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return Token(Category::plus_at, start_pos, location());
            }
            else
            {
                return Token(Category::plus, start_pos, location());
            }

        case '*':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::star_equal, start_pos, location());
            }
            else
            {
                return Token(Category::star, start_pos, location());
            }

        case '/':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::slash_equal, start_pos, location());
            }
            else
            {
                return Token(Category::slash, start_pos, location());
            }

        case '=':
            this->getc();
            if(peek_char == '#')
            {
                this->getc();
                return Token(Category::equal_hash, start_pos, location());
            }
            else
            {
                return Token(Category::equal, start_pos, location());
            }

        case '<':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::less_equal, start_pos, location());
            }
            else
            {
                return Token(Category::less, start_pos, location());
            }

        case '>':
            this->getc();
            if(peek_char == '=')
            {
                this->getc();
                return Token(Category::greater_equal, start_pos, location());
            }
            else
            {
                return Token(Category::greater, start_pos, location());
            }
        
        case '"':
            for(getc(); peek_char != '"'; getc())
            {
                if(is_newline(peek_char))
                {
                    diagnostics().report(location(), 
                            Diag::unterminated_string_literal);
                    return std::nullopt;
                }
            }

            assert(peek_char == '"');
            this->getc();

            return Token(Category::string, start_pos, location());

        word_token: default:
            if(!is_word_char(peek_char))
            {
                diagnostics().report(location(), Diag::invalid_char);
                this->getc();
                return std::nullopt;
            }

            this->getc();
            while(is_word_char(peek_char))
                this->getc();

            return Token(Category::word, start_pos, location());
    }
    // clang-format on
}
} // namespace gta3sc::syntax

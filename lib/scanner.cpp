#include <gta3sc/scanner.hpp>
#include <algorithm>
#include <cassert>

namespace gta3sc
{
constexpr Category CATEGORY_ARGUMENT = (Category::Integer 
                                       | Category::Float
                                       | Category::Identifier
                                       | Category::String);

constexpr Category CATEGORY_ASSIGNMENT_OPERATOR = (Category::Equal
                                                  | Category::PlusEqual
                                                  | Category::MinusEqual 
                                                  | Category::StarEqual 
                                                  | Category::SlashEqual 
                                                  | Category::PlusEqualAt 
                                                  | Category::MinusEqualAt 
                                                  | Category::EqualHash);

constexpr Category CATEGORY_RELATION_OPERATOR = (Category::Less
                                                 | Category::LessEqual
                                                 | Category::Greater
                                                 | Category::GreaterEqual);

constexpr Category CATEGORY_UNARY_OPERATOR = (Category::MinusMinus
                                              | Category::PlusPlus);

constexpr Category CATEGORY_BINARY_OPERATOR = (Category::Plus
                                               | Category::Minus
                                               | Category::Star
                                               | Category::Slash
                                               | Category::PlusAt);

constexpr Category CATEGORY_OPERATOR = (CATEGORY_ASSIGNMENT_OPERATOR
                                        | CATEGORY_RELATION_OPERATOR
                                        | CATEGORY_UNARY_OPERATOR
                                        | CATEGORY_BINARY_OPERATOR);

bool Scanner::eof() const
{
    if(peek_char || num_expr_tokens)
        return false;
    return pp.eof();
}

auto Scanner::location() const -> SourceLocation
{
    auto loc = pp.location();
    if(peek_char) --loc;
    return loc;
}

char Scanner::getc()
{
    this->prev_char = peek_char;
    return std::exchange(peek_char, pp.next());
}

auto Scanner::tell() const -> Snapshot
{
    assert(num_expr_tokens == 0);
    return Snapshot { pp.tell(), expression_mode, peek_char, prev_char };
}

void Scanner::seek(const Snapshot& snap)
{
    this->pp.seek(snap.pp);
    this->expression_mode = snap.expression_mode;
    this->peek_char = snap.peek_char;
    this->prev_char = snap.prev_char;
    this->num_expr_tokens = 0;
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

bool Scanner::is_graph(char c) const
{
    return c != '"' && c >= 33 && c <= 126;
}

bool Scanner::is_operator(char c) const
{
    return c == '+' || c == '-' || c == '*' 
        || c == '=' || c == '<' || c == '>';
}

bool Scanner::is_filename(SourceRange range) const
{
    if(range.size() >= 3)
    {
        auto it = range.rbegin();
        const auto c2 = *it++;
        const auto c1 = *it++;
        const auto c0 = *it++;
        if(c0 == '.' && (c1 == 's' || c1 == 'S') && (c2 == 'c' || c2 == 'C'))
            return true;
    }
    return false;
}

auto Scanner::match(Category hint, Category category, 
                    SourceLocation begin, SourceLocation end)
    -> std::optional<Token>
{
    if(is_set(hint, category))
        return Token(category, begin, end);
    return std::nullopt;
}

auto Scanner::next(Category hint) -> std::optional<Token>
{
    // This scanner operates in the following manner:
    //
    // At the beggining of each line, it tries to match a expression
    // within the line. If successful, a stack made of the tokens of
    // this expression is left to be drained by future calls to the
    // scanner. This is explained further later on.
    //
    // Otherwise, it operates normally.
    //
    // A call to the scanner recognizes the correct token by starting
    // from a possible category and reducing it to the next, until it is
    // not possible to reduce to any other category. The hint provides
    // additional context for possibility reduction.
    //
    // If the final category do not match any of the categories in hint,
    // the scan has failed, and it usually recovers by skipping to the
    // next whitespace or newline (except when the expression stack
    // is not empty).
    
    auto start_pos = this->location();
    auto category = Category::EndOfLine;

    // Drain the expression stack before resuming the scanner.
    if(num_expr_tokens)
    {
        --num_expr_tokens;

        auto token = this->expr_tokens[num_expr_tokens];
        if(is_set(hint, token.category))
            return token;

        // In case of match failure and we drained the stack, we
        // should keep the end of line element in the stack
        // for recovery purposes.
        if(num_expr_tokens == 0)
        {
            assert(token.category == Category::EndOfLine);
            num_expr_tokens = 1;
        }

        return std::nullopt;
    }

    // Call the expression scanner in case we are at the front of a line.
    if(!expression_mode)
    {
        if(!prev_char || is_newline(prev_char))
        {
            if(this->scan_expression_line())
            {
                assert(num_expr_tokens > 0);
                return this->next(hint);
            }
        }
    }

    switch(peek_char)
    {
        // clang-format off
        newline: case '\0': case '\n':
            if(is_set(hint, Category::EndOfLine))
            {
                this->getc();
                return Token(Category::EndOfLine, start_pos, location());
            }
            return std::nullopt;

        whitespace: case ' ': case '\t': case '(': case ')': case ',':
            if(is_set(hint, Category::Whitespace | Category::EndOfLine))
            {
                this->getc();
                while(is_whitespace(peek_char))
                    this->getc();

                // Trailing spaces should be handled as a newline.
                if(is_newline(peek_char))
                    goto newline;

                return match(hint, Category::Whitespace, start_pos, location());
            }
            return std::nullopt;

        case '-':
            this->getc();
            if(peek_char == '.')
            {
                goto floating_form1;
            }
            else if(is_digit(peek_char))
            {
                goto integer;
            }
            else if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return match(hint, Category::MinusEqualAt, start_pos, location());
                }
                else
                {
                    return match(hint, Category::MinusEqual, start_pos, location());
                }
            }
            else if(peek_char == '-')
            {
                this->getc();
                return match(hint, Category::MinusMinus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return match(hint, Category::MinusAt, start_pos, location());
            }
            else
            {
                return match(hint, Category::Minus, start_pos, location());
            }

        case '+':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                if(peek_char == '@')
                {
                    this->getc();
                    return match(hint, Category::PlusEqualAt, start_pos, location());
                }
                else
                {
                    return match(hint, Category::PlusEqual, start_pos, location());
                }
            }
            else if(peek_char == '+')
            {
                this->getc();
                return match(hint, Category::PlusPlus, start_pos, location());
            }
            else if(peek_char == '@')
            {
                this->getc();
                return match(hint, Category::PlusAt, start_pos, location());
            }
            else
            {
                return match(hint, Category::Plus, start_pos, location());
            }

        case '*':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                return match(hint, Category::StarEqual, start_pos, location());
            }
            else
            {
                return match(hint, Category::Star, start_pos, location());
            }

        case '/':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                return match(hint, Category::SlashEqual, start_pos, location());
            }
            else
            {
                return match(hint, Category::Slash, start_pos, location());
            }

        case '=':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '#')
            {
                this->getc();
                return match(hint, Category::EqualHash, start_pos, location());
            }
            else
                return match(hint, Category::Equal, start_pos, location());

        case '<':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                return match(hint, Category::LessEqual, start_pos, location());
            }
            else
            {
                return match(hint, Category::Less, start_pos, location());
            }

        case '>':
            this->getc();
            if(!expression_mode)
            {
                goto command_label_filename;
            }
            else if(peek_char == '=')
            {
                this->getc();
                return match(hint, Category::GreaterEqual, start_pos, location());
            }
            else
            {
                return match(hint, Category::Greater, start_pos, location());
            }
        
        integer:
        floating_form2:
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            bool got_minus = false;
            bool got_float = false;
            for(; ; this->getc())
            {
                if(is_digit(peek_char))
                    continue;
                else if(peek_char == '-')
                {
                    // cannot have a minus in the middle of a literal
                    // while in expression mode.
                    if(expression_mode)
                        break;
                    got_minus = true;
                }
                else if(peek_char == '.' || peek_char == 'f' || peek_char == 'F')
                    got_float = true;
                else
                    break;
            }

            // the minus sign cannot appear in the middle of a
            // floating-point literal.
            if(got_float && got_minus)
            {
                assert(!expression_mode);
                goto command_label_filename;
            }

            category = got_float? Category::Float : Category::Integer;
            goto finish_matching_argument;
        }

        floating_form1: case '.':
        {
            this->getc();
            if(!is_digit(peek_char))
                goto command_label_filename;

            while(is_digit(peek_char) || peek_char == '.' 
                    || peek_char == 'f' || peek_char == 'F')
            {
                this->getc();
            }

            category = Category::Float;
            goto finish_matching_argument;
        }

        case '$':
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'G': case 'H':
        case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P':
        case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z': case 'a': case 'b':
        case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v':
        case 'w': case 'x': case 'y': case 'z':
            for(getc(); is_graph(peek_char); getc())
            {
                if(expression_mode && is_operator(peek_char))
                    break;
            }

            category = Category::Identifier;
            goto finish_matching_argument;

        case '"':
            for(getc(); peek_char != '"'; getc())
            {
                if(is_newline(peek_char))
                    return std::nullopt;
            }

            assert(peek_char == '"');
            this->getc();

            category = Category::String;
            goto finish_matching_argument;

        default:
            assert(is_graph(peek_char));
            goto command_label_filename;

        command_label_filename:
        {
            while(is_graph(peek_char))
                this->getc();

            // by definition only whitespace, newline or quotation marks may
            // follow a command or label. quotation marks causes a failure.
            if(!is_whitespace(peek_char) && !is_newline(peek_char))
            {
                assert(peek_char == '"');
                this->next(Category::String);
                return std::nullopt;
            }

            // This may happen if we goto'ed to command_label_filename
            // from a string token not finished in whitespaces so that
            // we recover from the bad token (see finish_matching_argument).
            if(category == Category::String)
                return std::nullopt;

            if(prev_char == ':' && is_set(hint, Category::Label))
                category = Category::Label;
            else
                category = Category::Command;

            if(is_set(hint, Category::Filename))
            {
                auto lexeme = SourceRange(start_pos, location() - start_pos);
                if(is_filename(lexeme))
                    category = Category::Filename;
            }

            return match(hint, category, start_pos, location());
        }

        finish_matching_argument:
        {
            assert(category == Category::Integer
                    || category == Category::Float
                    || category == Category::Identifier
                    || category == Category::String);

            // following an argument must be a separator
            if(!is_whitespace(peek_char) && !is_newline(peek_char))
            {
                if(expression_mode && is_operator(peek_char))
                    /* allow operators following an argument in such a case */;
                else
                    goto command_label_filename;
            }

            if(category != Category::String)
            {
                if(is_set(hint, Category::Command))
                    goto command_label_filename;

                if(prev_char == ':')
                {
                    assert(category == Category::Identifier);
                    goto command_label_filename;
                }

                if(is_set(hint, Category::Filename))
                {
                    auto lexeme = SourceRange(start_pos, location() - start_pos);
                    if(is_filename(lexeme))
                        goto command_label_filename;
                }
            }

            return match(hint, category, start_pos, location());
        }

        // clang-format on
    }
}

bool Scanner::scan_expression_line()
{
    // The expression scanner switches the scanner into a special mode,
    // called the expression mode, where it may match operators. Then it
    // repeatedly calls the scanner trying to match a pattern that ressembles
    // a expression.
    //
    // If a pattern is found, the expression stack is filled with the tokens
    // of this line. Otherwise, the scanner is rewound back to the its original
    // state at the front of the line.
    //
    // This is effectively a parser inside a lexer. Such improper hierarchy.

    Category cats[6];
    size_t num_tokens = 0;

    const auto scan_mask = (CATEGORY_ARGUMENT | CATEGORY_OPERATOR 
                            | Category::Whitespace | Category::EndOfLine);

    // This function builds the state of the expression stack.
    assert(this->num_expr_tokens == 0);

    // Ensure we have enough slots for the expression and the end of line.
    assert(std::size(this->expr_tokens) >= 1 + std::size(cats));

    // Save the scanner state and switch it into expression mode.
    const auto tell_pos = this->tell();
    this->expression_mode = true;

    // Skip any label in the start of the line.
    auto token = this->next(Category::Label | scan_mask);
    if(token && token->category == Category::Label)
    {
        token = this->next(scan_mask);
    }

    while(token && token->category != Category::EndOfLine)
    {
        if(num_tokens == std::size(cats))
        {
            // Too many tokens to be an expression, stop.
            token = std::nullopt;
            break;
        }

        if(token->category != Category::Whitespace)
        {
            cats[num_tokens] = token->category;
            this->expr_tokens[num_tokens] = std::move(*token);
            ++num_tokens;
        }

        token = this->next(scan_mask);
    }

    bool matches_valid_expr = false;
    if(token)
    {
        if(num_tokens == 2
            && is_set(cats[0], CATEGORY_UNARY_OPERATOR) 
            && is_set(cats[1], CATEGORY_ARGUMENT))
        {
            matches_valid_expr = true;
        }
        else if(num_tokens == 2 
                && is_set(cats[0], CATEGORY_ARGUMENT)
                && is_set(cats[1], CATEGORY_UNARY_OPERATOR))
        {
            matches_valid_expr = true;
        }
        else if(num_tokens == 3
                && is_set(cats[0], CATEGORY_ARGUMENT) 
                && is_set(cats[1], CATEGORY_ASSIGNMENT_OPERATOR) 
                && is_set(cats[2], CATEGORY_ARGUMENT))
        {
            matches_valid_expr = true;
        }
        else if(num_tokens == 3
                && is_set(cats[0], CATEGORY_ARGUMENT) 
                && is_set(cats[1], CATEGORY_RELATION_OPERATOR)
                && is_set(cats[2], CATEGORY_ARGUMENT))
        {
            matches_valid_expr = true;
        }
        else if(num_tokens == 5
                && is_set(cats[0], CATEGORY_ARGUMENT) 
                && is_set(cats[1], Category::Equal) 
                && is_set(cats[2], CATEGORY_ARGUMENT)
                && is_set(cats[3], CATEGORY_BINARY_OPERATOR)
                && is_set(cats[4], CATEGORY_ARGUMENT))
        {
            matches_valid_expr = true;
        }
    }

    if(matches_valid_expr)
    {
        assert(token->category == Category::EndOfLine);
        this->expr_tokens[num_tokens] = std::move(*token);
        this->num_expr_tokens = ++num_tokens;

        // Leave expression mode and reverse the stack for use by the scanner.
        this->expression_mode = false;
        std::reverse(&this->expr_tokens[0], &this->expr_tokens[num_tokens]);

        return true;
    }

    this->seek(tell_pos);
    return false;
}
}

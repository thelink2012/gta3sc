#include <gta3sc/scanner.hpp>

    // whitespaces / sep
    // newline / eol
    // eof / eol
    // command_name => FIRST() = graph_char; FOLLOW() = {sep, eol}
    // label_stmt => FIRST() => graph_char; FOLLOW() = {sep, eol}
    // integer => FIRST() = {'-', '0'...'9'}; FOLLOW() => {sep, eol} U operators
    // floating => FIRST() = {'.', '-', '0'...'9'}; FOLLOW() => {sep, eol} U operators
    // identifier => FIRST() => {'$', 'A'..'Z'}; FOLLOW() => {sep, eol} U operators
    // string => FIRST() => {'"'}; FOLLOW() => {sep, eol}
    // operators => FIRST() => {'=', '+', '-', '*', '/', '<', '>'}; FOLLOW() => {sep} U integer U floating U identifier
    
namespace gta3sc
{
constexpr Category CATEGORY_ARGUMENT = (Category::Integer 
                                       | Category::Float
                                       | Category::Identifier
                                       | Category::StringLiteral);

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

bool Scanner::is_comment_start(SourceLocation p) const
{
    return *p == '/' && (*std::next(p) == '*' || *std::next(p) == '/');
}

bool Scanner::is_whitespace(SourceLocation p) const
{
    return *p == ' ' || *p == '\t' || *p == '(' || *p == ')' || *p == ',';
}

bool Scanner::is_newline(SourceLocation p) const
{
    return *p == '\r' || *p == '\n' || *p == '\0';
}

bool Scanner::is_digit(SourceLocation p) const
{
    return *p >= '0' && *p <= '9';
}

bool Scanner::is_graph(SourceLocation p) const
{
    return *p != '"' && *p >= 32 && *p <= 126;
}

bool Scanner::is_operator(SourceLocation p) const
{
    if(*p == '/')
        return !(*std::next(p) == '*' || *std::next(p) == '/');
    else
        return *p == '+' || *p == '-' || *p == '*' 
            || *p == '=' || *p == '<' || *p == '>';
}

auto Scanner::next(Category hint) -> std::optional<Token>
{
scan_again:
    auto start_pos = cursor;
    auto category = Category::EndOfLine;

    if(num_cached_tokens)
    {
        --num_cached_tokens;
        return std::move(cached_tokens[num_cached_tokens]);
    }

    if(num_block_comments)
    {
        goto parse_block_comment;
    }

    if(start_of_line)
    {
        if(!is_whitespace(cursor) && !is_comment_start(cursor) && !is_newline(cursor))
        {
            this->start_of_line = false;
            if(this->scan_expression_line())
                goto scan_again;
        }
    }

    switch(*cursor)
    {
        // clang-format off
        newline: case '\0': case '\r': case '\n':
            if(*cursor == '\0')
            {
                this->end_of_stream = true;
            }
            else
            {
                if(*cursor == '\r') ++cursor;
                if(*cursor == '\n') ++cursor;
                this->start_of_line = true;
            }
            return Token(Category::EndOfLine, start_pos, cursor);

        whitespace: case ' ': case '\t': case '(': case ')': case ',':
            while(is_whitespace(cursor))
                ++cursor;

            if(is_comment_start(cursor))
                goto comment;

            if(is_newline(cursor))
                goto newline;

            if(start_of_line)
            {
                this->start_of_line = false;
                this->scan_expression_line();
                goto scan_again;
            }

            return Token(Category::Whitespace, start_pos, cursor);

        parse_block_comment:
            while(!is_newline(cursor))
            {
                if(*cursor == '/' && *std::next(*cursor) == '*')
                {
                    std::advance(cursor, 2);
                    ++num_block_comments;
                }
                else if(*cursor == '*' && *std::next(*cursor) == '/')
                {
                    std::advance(cursor, 2);
                    --num_block_comments;
                    if(num_block_comments == 0)
                        goto whitespace;
                }
                else
                {
                    ++cursor;
                }
            }
            goto newline;


        case '-':
            ++cursor;
            if(*cursor == '.')
            {
                goto floating_form1;
            }
            else if(is_digit(cursor))
            {
                goto integer;
            }
            else if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=' && *std::next(cursor) == '@')
            {
                std::advance(cursor, 2);
                return Token(Category::MinusEqualAt, start_pos, cursor);
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::MinusEqual, start_pos, cursor);
            }
            else if(*cursor == '-')
            {
                ++cursor;
                return Token(Category::MinusMinus, start_pos, cursor);
            }
            else if(*cursor == '@')
            {
                ++cursor;
                return Token(Category::MinusAt, start_pos, cursor);
            }
            else
            {
                return Token(Category::Minus, start_pos, cursor);
            }

        case '+':
            ++cursor;
            if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=' && *std::next(cursor) == '@')
            {
                std::advance(cursor, 2);
                return Token(Category::PlusEqualAt, start_pos, cursor);
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::PlusEqual, start_pos, cursor);
            }
            else if(*cursor == '+')
            {
                ++cursor;
                return Token(Category::PlusPlus, start_pos, cursor);
            }
            else if(*cursor == '@')
            {
                ++cursor;
                return Token(Category::PlusAt, start_pos, cursor);
            }
            else
            {
                return Token(Category::Plus, start_pos, cursor);
            }

        case '*':
            ++cursor;
            if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::StarEqual, start_pos, cursor);
            }
            else
            {
                return Token(Category::Star, start_pos, cursor);
            }

        comment: case '/':
            ++cursor;
            if(*cursor == '/')
            {
                ++cursor;
                while(!is_newline(*cursor))
                    ++cursor;
                goto newline;
            }
            else if(*cursor == '*')
            {
                ++cursor;
                num_block_comments = 1;
                goto parse_block_comment;
            }
            else if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::SlashEqual, start_pos, cursor);
            }
            else
            {
                return Token(Category::Slash, start_pos, cursor);
            }

        case '=':
            ++cursor;
            if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '#')
            {
                ++cursor;
                return Token(Category::EqualHash, start_pos, cursor);
            }
            else
                return Token(Category::Equal, start_pos, cursor);

        case '<':
            ++cursor;
            if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::LessEqual, start_pos, cursor);
            }
            else
            {
                return Token(Category::Less, start_pos, cursor);
            }

        case '>':
            ++cursor;
            if(!expression_mode)
            {
                goto continue_as_graph_char;
            }
            else if(*cursor == '=')
            {
                ++cursor;
                return Token(Category::GreaterEqual, start_pos, cursor);
            }
            else
            {
                return Token(Category::Greater, start_pos, cursor);
            }
        
        integer:
        floating_form2:
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            bool got_minus = false;
            bool got_float = false;
            for(; ; ++cursor)
            {
                if(is_digit(cursor))
                    continue;
                else if(*cursor == '-')
                {
                    if(expression_mode)
                        break;
                    got_minus = true;
                }
                else if(*cursor == '.' || *cursor == 'f' || *cursor == 'F')
                    got_float = true;
                else
                    break;
            }

            // the minus sign cannot appear in the middle of a floating-point literal
            if(got_float && got_minus)
                goto continue_as_graph_char;

            category = got_float? Category::Float : Category::Integer;
            goto finish_matching_argument;
        }

        floating_form1: case '.':
        {
            ++cursor;
            if(!is_digit(cursor))
                goto continue_as_graph_char;

            while(is_digit(cursor) || *cursor == '.' || *cursor == 'f' || *cursor == 'F')
                ++cursor;

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
            ++cursor;
            while(is_graph(cursor))
            {
                if(expression_mode && is_operator(cursor))
                    break;
                ++cursor;
            }

            category = Category::Identifier;
            goto finish_matching_argument;

        case '"':
            ++cursor;
            while(*cursor != '"' && !is_newline(cursor))
                ++cursor;

            if(is_newline(cursor))
                // TODO error missing closing quote (already error recovered btw)
                return std::nullopt;

            ++cursor;
            assert(*cursor == '"');

            category = Category::String;
            goto finish_matching_argument;

        default:
            assert(is_graph(cursor));
            ++cursor;
            goto continue_as_graph_char;

        continue_as_graph_char:
            if(!is_set(hint, Category::Command) && !is_set(hint, Category::Label))
                goto error_recovery;

            if(expression_mode && !is_set(hint, Category::Label))
                goto error_recovery;

            while(is_graph(cursor))
                ++cursor;

            if(is_set(hint, Category::Label) && *std::prev(cursor) == ':')
                category = Category::Label;
            else
                category = Category::Command;

            if(expression_mode && category != Category::Label)
                goto error_recovery;

            // following a command or label definition is always a separator
            if(!is_whitespace(cursor) && !is_comment_start(cursor) && !is_newline(cursor))
            {
                assert(*cursor == '"');
                goto error_recovery;
            }

            return Token(category, start_pos, cursor);

        finish_matching_argument:
            // following a argument is always a separator
            if(!is_whitespace(cursor) && !is_comment_start(cursor) && !is_newline(cursor))
            {
                if(expression_mode && is_operator(cursor))
                    /* allow operators following a argument in such a case */;
                else
                    goto continue_as_graph_char;
            }

            // Identifiers should not end in a colon.
            if(category == Category::Identifier && *std::prev(cursor) == ':')
                goto continue_as_graph_char;

            // Give priority to commands over arguments unless we're in an expression.
            if(!expression_mode && is_set(hint, Category::Command))
                goto continue_as_graph_char;

            return Token(category, start_pos, cursor);

        error_recovery:
            while(!is_whitespace(cursor) && !is_comment_start(cursor) && !is_newline(cursor))
            {
                if(expression_mode && is_operator(cursor))
                    break;
                ++cursor;
            }
            return std::nullopt;

        // clang-format on
    }
}

bool Scanner::scan_expression_line()
{
    Category cats[6];
    size_t num_tokens = 0;

    // This function is triggered by a start of line. If we use the scanner
    // while in this state, we'll get into a infinite loop.
    assert(this->start_of_line == false);

    // This function may change the state of cached tokens.
    assert(this->num_cached_tokens == 0);

    // Ensure we have enough slots for the expression and the end of line.
    assert(std::size(this->cached_tokens) >= 1 + std::size(cats));

    // Save the scanner state and switch it into expression mode.
    const auto tell_pos = this->tell();
    this->expression_mode = true;

    // Skip any label in the start of the line.
    auto token = this->next(Category::Label | CATEGORY_ARGUMENT);
    if(token && token->category == Category::Label)
        token = this->next(CATEGORY_ARGUMENT);

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
            this->cached_tokens[num_tokens] = std::move(*token);
            ++num_tokens;
        }

        token = this->next(CATEGORY_ARGUMENT | CATEGORY_OPERATOR);
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
        this->cached_tokens[num_tokens] = std::move(*token);
        this->num_cached_tokens = ++num_tokens;
        return true;
    }

    this->seek(tell_pos);
    return false;
}
}

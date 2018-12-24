#include <gta3sc/preprocessor.hpp>

namespace gta3sc
{
bool Preprocessor::is_whitespace(SourceLocation p) const
{
    return *p == ' ' || *p == '\t' || *p == '(' || *p == ')' || *p == ',';
}

bool Preprocessor::is_newline(SourceLocation p) const
{
    return *p == '\r' || *p == '\n' || *p == '\0';
}

auto Preprocessor::source_file() const -> const SourceFile&
{
    return *this->source;
}

auto Preprocessor::diagnostics() const -> DiagnosticHandler&
{
    return *this->diag;
}

bool Preprocessor::eof() const
{
    return this->end_of_stream;
}

auto Preprocessor::location() const -> SourceLocation
{
    return this->cursor;
}

char Preprocessor::next()
{
    // The caller should see a stream that has no blank characters at
    // the beggining of a line and also has comments replaced by blanks.
    //
    // The problem is block comments can be sorrounded by whitespaces at
    // the front of a line. We cannot return anything before skipping
    // such combination of comments and whitespaces.
    //
    // We run this thing in a loop until such a condition is met.
    while(true)
    {
        constexpr auto max_nesting
                = (std::numeric_limits<decltype(num_block_comments)>::max());

        if(*cursor == '\0')
        {
            if(!end_of_stream && num_block_comments > 0)
                diagnostics().report(location(), Diag::unterminated_comment);

            this->end_of_stream = true;
            return '\0';
        }
        else if(is_newline(cursor))
        {
            if(*cursor == '\r')
                ++cursor;
            if(*cursor == '\n')
                ++cursor;
            this->start_of_line = true;
            this->inside_quotes = false;
            return '\n';
        }
        else if(num_block_comments)
        {
            // Parse the block comment until it ends or we reach a new line.
            while(num_block_comments && !is_newline(cursor))
            {
                if(*cursor == '/' && *std::next(cursor) == '*')
                {
                    if(num_block_comments == max_nesting)
                        diagnostics().report(location(),
                                             Diag::limit_block_comments);
                    else
                        ++num_block_comments;

                    std::advance(cursor, 2);
                }
                else if(*cursor == '*' && *std::next(cursor) == '/')
                {
                    --num_block_comments;
                    std::advance(cursor, 2);
                }
                else
                {
                    ++cursor;
                }
            }

            // We have three possibilities at this point:
            // 1) The block comment has not ended (i.e. we reached a new line);
            //    Continue the scan, thus returning a newline character.
            // 2) The block comment has ended, but we have not found any
            //    non-blank char in this line; Continue the scan, until we
            //    leave the state of being in the start of the line.
            // 3) The block comment has ended and we are not at the beggining of
            //    the line; Then return a whitespace to represent the contents
            //    of the comment.
            if(num_block_comments == 0 && !start_of_line)
                return ' ';
        }
        else if(start_of_line && is_whitespace(cursor))
        {
            // Skip over the whitespaces at the begginig of the line.
            ++cursor;
            while(is_whitespace(cursor))
                ++cursor;

            // We may not have skipped every blank because a comment may follow.
            // Thus continue the scan.
        }
        else if(!inside_quotes && *cursor == '/' && *std::next(cursor) == '*')
        {
            std::advance(cursor, 2);
            num_block_comments = 1;
        }
        else if(!inside_quotes && *cursor == '/' && *std::next(cursor) == '/')
        {
            std::advance(cursor, 2);
            while(!is_newline(cursor))
                ++cursor;
        }
        else
        {
            this->start_of_line = false;

            if(*cursor == '"')
                this->inside_quotes = !this->inside_quotes;

            return *cursor++;
        }
    }
}
}

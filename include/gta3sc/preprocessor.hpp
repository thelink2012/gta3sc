#pragma once
#include <gta3sc/sourceman.hpp>

namespace gta3sc
{
/// The preprocessor is a character stream over a source file.
///
/// This stream is stripped of comments, leading whitespaces in lines 
/// and line terminators are normalized to LF.
class Preprocessor
{
public:
    explicit Preprocessor(const SourceFile& source) :
        source(source)
    {
        this->cursor = &source.view_with_terminator()[0];
        this->start_of_line = true;
        this->end_of_stream = false;
        this->inside_quotes = false;
        this->num_block_comments = 0;
    }

    Preprocessor(const Preprocessor&) = delete;
    Preprocessor& operator=(const Preprocessor&) = delete;

    Preprocessor(Preprocessor&&) = default;
    Preprocessor& operator=(Preprocessor&&) = default;

    /// Gets the next character in the stream.
    char next();

    /// Checks whether the stream reached the end of file.
    bool eof() const;

    /// Checks whether the stream has an unclosed block comment at eof.
    bool inside_block_comment() const;

    /// Gets the current source location.
    auto location() const -> SourceLocation;

    /// Gets the source file associated with this preprocessor.
    auto source_file() const -> const SourceFile&;

private:
    bool is_whitespace(SourceLocation) const;
    bool is_newline(SourceLocation) const;

private:
    SourceLocation cursor;
    bool start_of_line;
    bool end_of_stream;
    bool inside_quotes;
    uint8_t num_block_comments;

    const SourceFile& source;
};
}

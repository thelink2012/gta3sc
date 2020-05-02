#pragma once
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/sourceman.hpp>

namespace gta3sc::syntax
{
/// The preprocessor is a character stream over a source file.
///
/// This stream is stripped of comments, leading whitespaces in lines
/// and line terminators are normalized to LF.
class Preprocessor
{
public:
    explicit Preprocessor(SourceFile source, DiagnosticHandler& diag) :
        source(std::move(source)), diag(&diag)
    {
        // TODO if we move cursor initialization we can make this noexcept
        this->cursor = this->source.code_data();
        this->start_of_line = true;
        this->end_of_stream = false;
        this->inside_quotes = false;
        this->num_block_comments = 0;
    }

    Preprocessor(const Preprocessor&) = delete;
    Preprocessor& operator=(const Preprocessor&) = delete;

    Preprocessor(Preprocessor&&) noexcept = default;
    Preprocessor& operator=(Preprocessor&&) noexcept = default;

    ~Preprocessor() noexcept = default;

    /// Gets the next character in the stream.
    char next();

    /// Checks whether the stream reached the end of file.
    bool eof() const;

    /// Gets the current source location.
    auto location() const -> SourceLocation;

    /// Gets the source file associated with this preprocessor.
    auto source_file() const -> const SourceFile&;

    /// Gets the diagnostic handler associated with this preprocessor.
    auto diagnostics() const -> DiagnosticHandler&;

private:
    bool is_whitespace(const char* p) const;
    bool is_newline(const char* p) const;

private:
    SourceFile source;
    DiagnosticHandler* diag;

    const char* cursor;
    bool start_of_line;
    bool end_of_stream;
    bool inside_quotes;
    uint8_t num_block_comments;
};
} // namespace gta3sc::syntax

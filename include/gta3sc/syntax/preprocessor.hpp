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
    auto operator=(const Preprocessor&) -> Preprocessor& = delete;

    Preprocessor(Preprocessor&&) noexcept = default;
    auto operator=(Preprocessor&&) noexcept -> Preprocessor& = default;

    ~Preprocessor() noexcept = default;

    /// Gets the next character in the stream.
    auto next() -> char;

    /// Checks whether the stream reached the end of file.
    [[nodiscard]] auto eof() const -> bool;

    /// Gets the current source location.
    [[nodiscard]] auto location() const -> SourceLocation;

    /// Gets the source file associated with this preprocessor.
    [[nodiscard]] auto source_file() const -> const SourceFile&;

    /// Gets the diagnostic handler associated with this preprocessor.
    [[nodiscard]] auto diagnostics() const -> DiagnosticHandler&;

private:
    [[nodiscard]] auto is_whitespace(const char* p) const -> bool;
    [[nodiscard]] auto is_newline(const char* p) const -> bool;

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

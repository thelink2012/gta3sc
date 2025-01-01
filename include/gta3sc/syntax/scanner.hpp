#pragma once
#include <gta3sc/syntax/preprocessor.hpp>

namespace gta3sc::syntax
{
/// Lexical category of a token.
enum class Category : uint8_t
{
    word,
    string,
    whitespace,
    end_of_line,
    equal,
    plus_equal,
    minus_equal,
    star_equal,
    slash_equal,
    plus_equal_at,
    minus_equal_at,
    equal_hash,
    minus_minus,
    plus_plus,
    less,
    less_equal,
    greater,
    greater_equal,
    plus,
    minus,
    star,
    slash,
    plus_at,
    minus_at
};

/// Classified lexeme.
struct Token
{
    Category category{Category::end_of_line}; ///< Category of this token.
    SourceRange source;                       ///< Origin of this token.
};

/// The scanner transforms a stream of characters into a stream of tokens.
class Scanner
{
public:
    explicit Scanner(Preprocessor pp) noexcept : pp(std::move(pp))
    {}

    Scanner(const Scanner&) = delete;
    auto operator=(const Scanner&) -> Scanner& = delete;

    Scanner(Scanner&&) noexcept = default;
    auto operator=(Scanner&&) noexcept -> Scanner& = default;

    ~Scanner() noexcept = default;

    /// Consumes the next token in the stream of characters.
    ///
    /// It is guaranted that a finite number of calls to this function
    /// (even if they fail) reaches the end of the stream.
    ///
    /// The end of line token is returned infinitely when the scanner
    /// is at the end of the stream.
    ///
    /// When the next token is lexically invalid, an error is reported
    /// to the `DiagnosticManager` and `std::nullopt` is returned.
    auto next() -> std::optional<Token>;

    /// Consumes the next filename token in the stream of characters.
    ///
    /// The difference between this scan and the usual one is that it
    /// does not consider operators as individual tokens, but as part
    /// of a word token (e.g. `file-name.sc`). Additionally, it is
    /// not guaranted that a finite number of calls to this function
    /// reaches the end of the stream.
    ///
    /// A filename must end in `.sc`, otherwise `std::nullopt` gets
    /// returned.
    ///
    /// The category of a filename is `Category::word`.
    ///
    /// If the next token is not a valid filename, an error is reported
    /// to the `DiagnosticManager` and `std::nullopt` is returned.
    auto next_filename() -> std::optional<Token>;

    /// Checks whether the end of stream has been reached.
    [[nodiscard]] auto eof() const -> bool;

    /// Gets the current location in the character stream.
    [[nodiscard]] auto location() const -> SourceLocation;

    /// Gets the source file associated with this scanner.
    [[nodiscard]] auto source_file() const -> const SourceFile&;

    /// Gets the diagnostic handler associated with this scanner.
    [[nodiscard]] auto diagnostics() const -> DiagnosticHandler&;

    /// Returns a view to the characters of a token in the source code.
    [[nodiscard]] auto spelling(const Token& token) const -> std::string_view;

private:
    /// Consumes the next character in the character stream.
    auto getc() -> char;

    [[nodiscard]] auto is_whitespace(char c) const -> bool;
    [[nodiscard]] auto is_newline(char c) const -> bool;
    [[nodiscard]] auto is_digit(char c) const -> bool;
    [[nodiscard]] auto is_print(char c) const -> bool;
    [[nodiscard]] auto is_word_char(char c) const -> bool;

private:
    Preprocessor pp;
    char peek_char{};
};
} // namespace gta3sc::syntax

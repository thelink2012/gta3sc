#pragma once
#include <gta3sc/preprocessor.hpp>

namespace gta3sc
{
/// Lexical category of a token.
enum class Category : uint8_t
{
    Word,
    String,
    Whitespace,
    EndOfLine,
    Equal,
    PlusEqual,
    MinusEqual,
    StarEqual,
    SlashEqual,
    PlusEqualAt,
    MinusEqualAt,
    EqualHash,
    MinusMinus,
    PlusPlus,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Plus,
    Minus,
    Star,
    Slash,
    PlusAt,
    MinusAt,
};

/// Classified lexeme.
struct Token
{
    Category category;  ///< Category of this token.
    SourceRange lexeme; ///< Characters of this token.

    explicit Token() : category(Category::EndOfLine), lexeme() {}

    explicit Token(Category category, SourceLocation begin,
                   SourceLocation end) :
        category(category),
        lexeme(std::addressof(*begin), end - begin)
    {}
};

/// The scanner transforms a stream of characters into a stream of tokens.
class Scanner
{
public:
    explicit Scanner(Preprocessor pp_) : pp(std::move(pp_)), peek_char(0) {}

    Scanner(const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;

    Scanner(Scanner&&) = default;
    Scanner& operator=(Scanner&&) = default;

    /// Consumes the next token in the stream of characters.
    ///
    /// It is guaranted that a finite number of calls to this function
    /// (even if they fail) reaches the end of the stream.
    ///
    /// The end of line token is returned infinitely when the scanner
    /// is at the end of the stream.
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
    /// The category of a filename is `Category::Word`.
    auto next_filename() -> std::optional<Token>;

    /// Checks whether the end of stream has been reached.
    bool eof() const;

    /// Gets the source file associated with this scanner.
    auto source_file() const -> const SourceFile&;

private:
    /// Consumes the next character in the character stream.
    char getc();

    /// Gets the current location in the character stream.
    auto location() const -> SourceLocation;

    bool is_whitespace(char) const;
    bool is_newline(char) const;
    bool is_digit(char) const;
    bool is_print(char) const;
    bool is_word_char(char) const;

private:
    Preprocessor pp;
    char peek_char;
};
}

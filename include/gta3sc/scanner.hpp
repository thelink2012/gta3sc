#include <gta3sc/preprocessor.hpp>

namespace gta3sc
{
/// Lexical category of a token.
///
/// This may be used as a bitmask to the scanner hint parameter.
/// In all other cases, this is asssumed to be a single value.
enum class Category : uint64_t
{
    Integer = 1ULL << 0,
    Float = 1ULL << 1,
    Identifier = 1ULL << 2,
    Filename = 1ULL << 3,
    Equal = 1ULL << 4,
    PlusEqual = 1ULL << 5,
    MinusEqual = 1ULL << 6,
    StarEqual = 1ULL << 7,
    SlashEqual = 1ULL << 8,
    PlusEqualAt = 1ULL << 9,
    MinusEqualAt = 1ULL << 10,
    EqualHash = 1ULL << 11,
    Less = 1ULL << 12,
    LessEqual = 1ULL << 13,
    Greater = 1ULL << 14,
    GreaterEqual = 1ULL << 15,
    MinusMinus = 1ULL << 16,
    PlusPlus = 1ULL << 17,
    Plus = 1ULL << 18,
    Minus = 1ULL << 19,
    Star = 1ULL << 20,
    Slash = 1ULL << 21,
    PlusAt = 1ULL << 22,
    EndOfLine = 1ULL << 23,
    Whitespace = 1ULL << 24,
    MinusAt = 1ULL << 25,
    String = 1ULL << 26,
    Command = 1ULL << 27,
    Label = 1ULL << 28,
};

constexpr bool operator!(Category category)
{
    return static_cast<uint64_t>(category) == 0;
}

constexpr Category operator|(Category lhs, Category rhs)
{
    return static_cast<Category>(
                        static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
}

constexpr bool is_set(Category lhs, Category rhs)
{
    return (static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)) != 0;
}

/// Classified word
struct Token
{
    Category category;  //< Category of this token.
    SourceRange lexeme; //< Characters of this token.

    explicit Token() :
        category(Category::EndOfLine), lexeme()
    {}

    explicit Token(Category category, 
                   SourceLocation begin, 
                   SourceLocation end) :
        category(category), lexeme(begin, std::distance(begin, end))
    {}
};

/// The scanner is a stream of tokens.
///
/// It transforms a stream of characters into one of tokens.
///
/// The scanner is context-sensitive. In order to read the next token, a hint
/// specifying what is expected to be read must be provided.
class Scanner
{
public:
    explicit Scanner(Preprocessor pp_) :
        pp(std::move(pp_))
    {
        this->expression_mode = false;
        this->num_expr_tokens = 0;
        this->peek_char = this->pp.next();
        this->prev_char = '\0';
    }

    /// Scans the next token in the stream of characters.
    ///
    /// A bitmask of possible categories to match must be specified in `mask`.
    ///
    /// In case it's not possible to match any of the specified categories,
    /// or any other error occurs, the stream recovers by skipping certain
    /// characters, and you may call the scanner again afterwards.
    /// 
    /// The scanner is guaranteed to reach a newline after enough failures.
    auto next(Category mask) -> std::optional<Token>;

    /// Checks whether the end of stream has been reached.
    bool eof() const;

private:
    struct Snapshot;

    /// Takes a snapshot of the scanner state.
    auto tell() const -> Snapshot;

    /// Restores the state of the scanner from a snapshot.
    void seek(const Snapshot& snap);

    /// Consumes the next character in the character stream.
    char getc();

    /// Gets the current location in the character stream.
    auto location() const -> SourceLocation;

    bool is_whitespace(char) const;
    bool is_newline(char) const;
    bool is_digit(char) const;
    bool is_graph(char) const;
    bool is_operator(char) const;
    bool is_filename(SourceRange) const;

    auto match(Category hint, Category category, 
               SourceLocation begin, SourceLocation end) -> std::optional<Token>;

    bool scan_expression_line();

private:
    Preprocessor pp;

    size_t num_expr_tokens;
    Token expr_tokens[7];

    bool expression_mode;
    char peek_char;
    char prev_char;

    struct Snapshot
    {
        Preprocessor::Snapshot pp;
        bool expression_mode;
        char peek_char;
        char prev_char;
    };

    // Keep the snapshot small.
    static_assert(sizeof(Snapshot) <= 3 * sizeof(void*));
};
}


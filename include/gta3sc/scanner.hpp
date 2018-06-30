#include <gta3sc/sourceman.hpp>
#include <cassert>

namespace gta3sc
{
enum class Category : uint64_t
{
    Integer = 1ULL << 0,
    Float = 1ULL << 1,
    Identifier = 1ULL << 2,
    StringLiteral = 1ULL << 3,
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

struct Token
{
    Category category;
    SourceRange lexeme;

    explicit Token() :
        category(Category::EndOfLine), lexeme()
    {}

    explicit Token(Category category, SourceLocation begin, SourceLocation end) :
        category(category), lexeme(begin, std::distance(begin, end))
    {}
};

/// This is a scanner for GTA3script.
///
/// The scanner is context-sensitive. In order to read the next token, a hint
/// specifying what is expected to be read must be provided.
///
/// The hint does not mean only the categories specified should be scanned. Instead,
/// it is used to resolve cases where a token may resolve to multiple categories.
///
class Scanner
{
public:
    explicit Scanner(const SourceFile& source) :
        source(source)
    {
        this->cursor = source.view_with_terminator().begin();
        this->start_of_line = true;
        this->end_of_stream = false;
        this->expression_mode = false;
        this->num_cached_tokens = 0;
    }

    /// Scans the next token in the stream of characters.
    ///
    /// In case of failure, the scanner is still
    /// functional and may be used to scan more tokens.
    ///
    /// \returns the token or `std::nullopt` in case of failure.
    auto next(Category hints) -> std::optional<Token>;

    /// \returns whether the end of stream has been reached.
    bool is_eof() const
    {
        if(num_cached_tokens)
            return false;
        return end_of_stream;
    }

private:
    struct State;

    /// Returns the current state of the scanner.
    auto tell() const -> State
    {
        assert(num_cached_tokens == 0);
        return State { cursor, start_of_line, end_of_stream, expression_mode };
    }

    /// Restores the state of the scanner.
    void seek(const State& state)
    {
        this->cursor = state.cursor;
        this->start_of_line = state.start_of_line;
        this->end_of_stream = state.end_of_stream;
        this->expression_mode = state.expression_mode;
        this->num_cached_tokens = 0;
    }

private:
    bool is_whitespace(SourceLocation) const;
    bool is_newline(SourceLocation) const;
    bool is_digit(SourceLocation) const;
    bool is_graph(SourceLocation) const;
    bool is_operator(SourceLocation) const;
    bool scan_expression_line();


private:
    const SourceFile& source;

    size_t num_cached_tokens;
    Token cached_tokens[7];

    SourceLocation cursor;
    bool start_of_line;
    bool end_of_stream;
    bool expression_mode;

    struct State
    {
        SourceLocation cursor;
        bool start_of_line;
        bool end_of_stream;
        bool expression_mode;
    };
};
}


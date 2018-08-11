#pragma once
#include <gta3sc/scanner.hpp>
#include <gta3sc/arena-allocator.hpp>
#include <gta3sc/ir/parser-ir.hpp> 
#include <gta3sc/ir/linked-ir.hpp> 

namespace gta3sc
{
class Parser
{
public:
    explicit Parser(Scanner scanner_, ArenaMemoryResource& arena_) :
        scanner(std::move(scanner_)),
        arena(arena_)
    {
        assert(std::size(has_peek_token) == std::size(peek_tokens));
        std::fill(std::begin(has_peek_token), std::end(has_peek_token), false);
    }

    /// \returns the source file associated with this parser.
    auto source_file() const -> const SourceFile&;

    /// Skips to the next line in the token stream.
    void skip_current_line();

    auto parse_statement()
        -> std::optional<LinkedIR<ParserIR>>;

private:
    auto parse_embedded_statement()
        -> std::optional<LinkedIR<ParserIR>>;

    auto parse_scope_statement()
        -> std::optional<LinkedIR<ParserIR>>;

    auto parse_require_statement()
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_assignment_expression()
        -> std::optional<LinkedIR<ParserIR>>;

    auto parse_conditional_expression()
        -> std::optional<LinkedIR<ParserIR>>;

    auto parse_expression_internal(bool is_conditional)
        -> std::optional<LinkedIR<ParserIR>>;

    auto parse_command_statement()
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_command(bool is_if_line = false) 
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_expression()
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_argument() 
        -> std::optional<arena_ptr<ParserIR::Argument>>;

    bool is_digit(char) const;
    bool is_integer(const Token&) const;
    bool is_float(const Token&) const;
    bool is_identifier(const Token&) const;

    auto source_info(const Token&) const -> ParserIR::SourceInfo;

    auto source_info(SourceLocation begin, SourceLocation end) const
        -> ParserIR::SourceInfo;

private:
    /// Looks ahead by N tokens in the token stream.
    /// 
    /// In order to look into the token `N`, it is necessary to first look
    /// into the token `N-1` (unless`N == 0`). A call to a token consumer
    /// (such as `next` or `consume`) reduces the number of already peeked
    /// tokens by one.
    ///
    /// The lookahead is restricted to the line of the current token,
    /// thus trying to read past the end of the line yields `std::nullopt`.
    ///
    /// The 0th peek token is the current token in the stream.
    ///
    /// \returns the token at N or `std::nullopt` if such token is invalid.
    auto peek(size_t n = 0) -> std::optional<Token>;

    /// Consumes and returns the current token in the stream.
    ///
    /// \returns the current token or `std::nullopt` if such token is invalid.
    auto next() -> std::optional<Token>;

    /// Consumes and returns the current token in the stream assuming its
    /// category is of a filename.
    ///
    /// All peek tokens must have been consumed for this to work.
    auto next_filename() -> std::optional<Token>;

    /// Consumes and returns the current token in the stream.
    ///
    /// If the token category is none of `categories`, then `std::nullopt`
    /// is returned. Notice that is not the only case `std::nullopt` may
    /// get returned. Please see `next`.
    template<typename... Args>
    auto consume(Args... category0) -> std::optional<Token>
    {
        static_assert((std::is_same_v<Args, Category> && ...));

        if(sizeof...(Args) == 0)
            return next();

        auto token = next();
        if(!token)
            return std::nullopt;

        if(((token->category != category0) && ...))
        {
            // TODO emit expected error
            return std::nullopt;
        }

        return token;
    }

    /// Produces the same effect as `consume(Category::Word)`, but additionally
    /// returns `std::nullopt` if the word lexeme is not equal `lexeme`.
    auto consume_word(std::string_view lexeme) -> std::optional<Token>
    {
        auto token = consume(Category::Word);
        if(!token)
            return std::nullopt;
        if(token->lexeme != lexeme)
            return std::nullopt;
        return token;
    }

    /// Returns whether the lookahead token N has the specified category.
    bool is_peek(Category category, size_t n = 0)
    {
        return peek(n) && peek(n)->category == category;
    }

    /// Returns whether the lookahead token N has the specified category
    /// and compares equal to lexeme. The comparision is case insensitive.
    bool is_peek(Category category, std::string_view lexeme, size_t n = 0)
    {
        return is_peek(category, n) && iequal(peek(n)->lexeme, lexeme);
    }

    bool eof() const
    {
        if(has_peek_token[0])
            return false;
        return scanner.eof();
    }

    bool iequal(std::string_view lhs, std::string_view rhs) const;

private:
    Scanner scanner;
    ArenaMemoryResource& arena;

    bool has_peek_token[6];
    std::optional<Token> peek_tokens[6];
    
    bool in_lexical_scope = false;

};
}

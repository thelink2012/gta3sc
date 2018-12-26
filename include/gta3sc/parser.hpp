#pragma once
#include <gta3sc/arena-allocator.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/scanner.hpp>

namespace gta3sc
{
/// The parser checks the syntactical validity of a stream of tokens.
///
/// It produces as output not only whether the stream is valid, but also
/// an intermediate representation for further processing.
///
/// This parser produces IR instead of performing syntax-directed translation
/// because that does not work well in this language. The semantic analysis
/// phrase requires the content of all the script files spliced together
/// and needs so in a very specific order.
///
/// Please refer to https://git.io/fNxZP for details of the language grammar.
class Parser
{
public:
    /// \param scanner the scanner to consume tokens from.
    /// \param arena the arena that should be used to allocate IR in.
    explicit Parser(Scanner scanner, ArenaMemoryResource& arena) :
        scanner(std::move(scanner)),
        arena(&arena)
    {
        assert(std::size(has_peek_token) == std::size(peek_tokens));
    }

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(Parser&&) = default;
    Parser& operator=(Parser&&) = default;

    /// Gets the source file associated with this parser.
    auto source_file() const -> const SourceFile&;

    /// Gets the diagnostic handler associated with this parser.
    auto diagnostics() const -> DiagnosticHandler&;

    /// Checks whether the end of stream has been reached.
    bool eof() const;

    /// Skips to the next line in the token stream.
    void skip_current_line();

    /// Parses a main script file.
    auto parse_main_script_file() -> std::optional<LinkedIR<ParserIR>>;

    /// Parses the next statement in the token stream.
    ///
    /// Any parsing error causes the parser to produce a diagnostic
    /// and return `std::nullopt`.
    auto parse_statement(bool allow_special_name = false)
            -> std::optional<LinkedIR<ParserIR>>;

    /// Parses a sequence of statements until (and including) a
    /// command statement with the same command name as one in the
    /// initializer list.
    ///
    /// If the initializer list is empty, all statements until the
    /// end of the file are parsed. If there are no statements
    /// until the end of the file, the returned IR is empty.
    ///
    /// All the names in the initializer list must be uppercase.
    ///
    /// The names in the initializer list may be special command names.
    ///
    /// Any parsing error causes the parser to produce a diagnostic
    /// and return `std::nullopt`.
    auto parse_statement_list(std::initializer_list<std::string_view>)
            -> std::optional<LinkedIR<ParserIR>>;

    /// Produces the same effect as `parse_statement_list({name})`.
    auto parse_statement_list(std::string_view)
            -> std::optional<LinkedIR<ParserIR>>;

private:
    /// Looks ahead by N tokens in the token stream.
    ///
    /// In order to look into the token `N`, it is necessary to first look
    /// into the token `N-1` (unless`N == 0`). Do note a call to a token
    /// consumer (such as `next` or `consume`) reduces the number of already
    /// peeked in tokens by one.
    ///
    /// The lookahead is restricted to the line of the current token,
    /// trying to read past the end of the line yields more end of lines.
    ///
    /// The above restrictions are imposed by the fact one may acidentally
    /// peek what should have been a `next_filename` token, which needs
    /// special lexical handling.
    ///
    /// The 0th peek token is the current token in the stream.
    ///
    /// In case `N` is peeked for the first time and it is a lexically invalid
    /// token, a diagnostic is emitted.
    ///
    /// Returns the token at N or `std::nullopt` if such token is
    /// lexically invalid.
    auto peek(size_t n = 0) -> std::optional<Token>;

    /// Peeks the lookahead token `N` and checks whether it has
    /// the specified category.
    ///
    /// Please refer to `peek` for additional details on peeking.
    bool is_peek(Category category, size_t n = 0);

    /// Produces the same effect as `is_peek(category, n)` but additionally
    /// checks the lexeme of the token. The comparision is case insensitive.
    bool is_peek(Category category, std::string_view lexeme, size_t n = 0);

    /// Peeks the type of the expression in the current line.
    ///
    /// If the expression is a ternary expression, then `Category::Equal`
    /// is returned. Otherwise, the category of the operator token.
    ///
    /// Please refer to `peek` for additional details on peeking.
    auto peek_expression_type() -> std::optional<Category>;

    /// Consumes the current token in the stream.
    ///
    /// In case the token is lexically invalid, a diagnostic is produced
    /// (unless a peek call already did so) and `std::nullopt` is returned.
    auto consume() -> std::optional<Token>;

    /// Consumes the current token in the stream assuming its category
    /// is of a filename.
    ///
    /// There must be no peek token available for this operation to work.
    ///
    /// Filenames need special handling because they may contain characters
    /// that could otherwise be considered multiple tokens (e.g. `1-2.sc`).
    ///
    /// In case the token is not a filename, a diagnostic is produced
    /// and `std::nullopt` is returned.
    auto consume_filename() -> std::optional<Token>;

    /// Consumes and returns the current token in the stream assuming
    /// its category is the same as the specified one.
    ///
    /// If the token category is not the expected ones, a diagnostic is
    /// produced and `std::nullopt` returned.
    auto consume(Category) -> std::optional<Token>;

    /// Produces the same effect as `consume(Category::Word)`, but additionally
    /// checks (also producing a diagnostic) whether the lexeme of the word is
    /// equal `lexeme`.
    auto consume_word(std::string_view lexeme) -> std::optional<Token>;

    /// Behaves like `consume(Category::Word)` except with a custom diagnostic.
    auto consume_command() -> std::optional<Token>;

    /// Behaves like `consume(Category::Whitespace)` except when right behind
    /// an end of line in which case it peeks the end of line.
    auto consume_whitespace() -> std::optional<Token>;

    /// Compares two strings for equality in a case-insensitive manner.
    bool iequal(std::string_view lhs, std::string_view rhs) const;

    /// Checks whether the specified name is a command name used by
    /// the language grammar for special purposes (e.g. 'REPEAT', `VAR_INT`).
    bool is_special_name(std::string_view name, bool check_var_decl) const;

    /// Checks whether the specified lexical category is of a relational
    /// comparision operator (i.e. `<=`, `<`, `>`, `>=`).
    bool is_relational_operator(Category) const;

    /// Produces a diagnostic report associated with a given token.
    auto report(const Token& token, Diag message) -> DiagnosticBuilder;

    /// Produces a diagnostic report associated with a given range.
    auto report(SourceRange range, Diag message) -> DiagnosticBuilder;

    /// Produces a diagnostic regarding a unexpected grammar name.
    auto report_special_name(SourceRange range) -> DiagnosticBuilder;

private:
    // The following should behave according to the language grammar.
    // Please see https://git.io/fNxZP for details.
    //
    // A lexical or syntactical error causes a diagnostic report and and
    // `std::nullopt` to be returned.
    //
    // The parameters named `is_if_line` indicate productions on which
    // the behaviour changes depending on whether the current line began
    // with either `IF` or `IFNOT` tokens.
    //
    // The parameter named `allow_special_name` specifies whether it may
    // parse a command statement with special command names. Please see
    // `is_special_name` for details. Note variable declarations are
    // whitelisted in the check.
    //
    // The methods returns a ParserIR instance when only a single command is
    // returned. Otherwise, a LinkedIR is used.

    auto parse_command(bool is_if_line = false)
            -> std::optional<arena_ptr<ParserIR>>;

    auto parse_argument() -> std::optional<arena_ptr<ParserIR::Argument>>;

    auto parse_embedded_statement(bool allow_special_name)
            -> std::optional<LinkedIR<ParserIR>>;

    auto parse_scope_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_conditional_element(bool is_if_line = false)
            -> std::optional<arena_ptr<ParserIR>>;

    auto parse_conditional_list()
            -> std::pair<std::optional<LinkedIR<ParserIR>>, int32_t>;

    /// Continues parsing a conditional list after parsing the first
    /// conditional element. That is, parses AND/OR conditional lines.
    auto parse_conditional_list(arena_ptr<ParserIR> op_cond0)
            -> std::pair<std::optional<LinkedIR<ParserIR>>, int32_t>;

    auto parse_if_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_ifnot_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_if_statement_detail(bool is_ifnot)
            -> std::optional<LinkedIR<ParserIR>>;

    auto parse_while_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_whilenot_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_while_statement_detail(bool is_whilenot)
            -> std::optional<LinkedIR<ParserIR>>;

    auto parse_repeat_statement() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_require_statement() -> std::optional<arena_ptr<ParserIR>>;

    auto parse_assignment_expression() -> std::optional<LinkedIR<ParserIR>>;

    auto parse_conditional_expression(bool is_if_line = false)
            -> std::optional<LinkedIR<ParserIR>>;

    auto parse_expression_detail(bool is_conditional, bool is_if_line)
            -> std::optional<LinkedIR<ParserIR>>;

    bool is_digit(char) const;
    bool is_integer(std::string_view) const;
    bool is_float(std::string_view) const;
    bool is_identifier(std::string_view) const;

private:
    Scanner scanner;
    ArenaMemoryResource* arena;

    bool in_lexical_scope = false;
    bool has_peek_token[6] = {};
    std::optional<Token> peek_tokens[6];
};
}

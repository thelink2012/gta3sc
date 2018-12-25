#pragma once
#include <functional>
#include <gta3sc/sourceman.hpp>
#include <memory>
#include <variant>
#include <vector>

namespace gta3sc
{
enum class Category : uint8_t;
class DiagnosticHandler;
class DiagnosticBuilder;

enum class Diag
{
    cannot_nest_scopes,
    cannot_mix_andor,
    too_many_conditions,
    subscript_must_be_positive,
    expected_token,  // %0 => Category
    expected_word,   // %0 => string
    expected_words,  // %0 => vector<string>
    expected_command,
    expected_require_command,
    expected_argument,
    expected_identifier,
    expected_integer,
    expected_subscript,
    expected_conditional_expression,
    expected_conditional_operator,
    expected_assignment_operator,
    expected_ternary_operator,
    unexpected_special_name, // %0 => string
    invalid_char,
    invalid_filename,
    invalid_expression,
    invalid_expression_unassociative, // %0 => Category
    unterminated_comment,
    unterminated_string_literal,
    integer_literal_too_big,
    float_literal_too_big,
    limit_block_comments,
    duplicate_var_global,
    duplicate_var_in_scope,
    duplicate_var_lvar,
    duplicate_label,
    var_decl_outside_of_scope,
    var_decl_subscript_must_be_literal,
    var_decl_subscript_must_be_nonzero,
};

/// Information about a diagnostic.
struct Diagnostic
{
    using Arg = std::variant<Category, std::string, std::vector<std::string>>;

    Diag message;       ///< The diagnostic message.
    SourceLocation loc; ///< Location from where the diagnostic was reported.
    std::vector<SourceRange> ranges; ///< Locations related to the diagnostic.
    std::vector<Arg> args;           ///< Arguments for formatting the message.

    explicit Diagnostic(SourceLocation loc, Diag message) :
        loc(loc),
        message(message)
    {}
};

/// Helper class to construct a `Diagnostic`.
class DiagnosticBuilder
{
public:
    explicit DiagnosticBuilder(SourceLocation loc, Diag message,
                               DiagnosticHandler& handler) :
        handler(&handler),
        diag(std::make_unique<Diagnostic>(loc, message))
    {}

    /// Hands the diagnostic to the handler.
    ~DiagnosticBuilder() noexcept;

    DiagnosticBuilder(const DiagnosticBuilder&) = delete;
    DiagnosticBuilder& operator=(const DiagnosticBuilder&) = delete;

    DiagnosticBuilder(DiagnosticBuilder&&) = default;
    DiagnosticBuilder& operator=(DiagnosticBuilder&&) = default;

    /// Adds a source range to provide more context to the diagnostic.
    auto range(SourceRange range) && -> DiagnosticBuilder&&
    {
        diag->ranges.push_back(range);
        return std::move(*this);
    }

    /// Adds an argument to the diagnostic.
    template<typename... Args>
    auto args(Args&&... args) && -> DiagnosticBuilder&&
    {
        ((void)diag->args.push_back(convert(std::forward<Args>(args))), ...);
        return std::move(*this);
    }

private:
    template<typename T>
    static auto convert(T&& arg) -> Diagnostic::Arg
    {
        using Ty = std::decay_t<T>;
        if constexpr(std::is_same_v<Ty, std::string_view>)
        {
            return std::string(arg);
        }
        else if constexpr(std::is_same_v<Ty, std::vector<std::string_view>>)
        {
            std::vector<std::string> vec(arg.size());
            std::transform(arg.begin(), arg.end(), vec.begin(),
                           [](const auto& view) { return view; });
            return vec;
        }
        else
        {
            return std::forward<T>(arg);
        }
    }

private:
    DiagnosticHandler*
            handler; ///< The handler that will receive the diagnostic.
    std::unique_ptr<Diagnostic> diag; ///< Diagnostic being constructed.
};

/// A diagnostic handler.
///
/// Diagnostics are reported and treated through this handler. Once reported, a
/// diagnostic is passed to an emitter (a function callback) responsible to
/// treat it. It may do anything, like ignore the error entirely or print
/// it into a stream.
class DiagnosticHandler
{
public:
    using Emitter = std::function<void(const Diagnostic&)>;

public:
    explicit DiagnosticHandler(Emitter emitter) : emitter(std::move(emitter)) {}

    DiagnosticHandler(const DiagnosticHandler&) = delete;
    DiagnosticHandler& operator=(const DiagnosticHandler&) = delete;

    DiagnosticHandler(DiagnosticHandler&&) = default;
    DiagnosticHandler& operator=(DiagnosticHandler&&) = default;

    /// Helper function to facilitate the construction of a `DiagnosticBuilder`.
    auto report(SourceLocation loc, Diag message) -> DiagnosticBuilder
    {
        return DiagnosticBuilder(loc, message, *this);
    }

    /// Sets a new emitter to be called at diagnostic emission.
    void set_emitter(Emitter emitter) { this->emitter = std::move(emitter); }

protected:
    friend class DiagnosticBuilder;
    void emit(std::unique_ptr<Diagnostic> diag);

private:
    Emitter emitter;
};

// The builder must be a small object.
static_assert(sizeof(DiagnosticBuilder) <= 2 * sizeof(size_t));
}

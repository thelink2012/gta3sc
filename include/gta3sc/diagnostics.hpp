#pragma once
#include <algorithm>
#include <functional>
#include <gta3sc/sourceman.hpp>
#include <memory>
#include <variant>
#include <vector>

namespace gta3sc::syntax
{
enum class Category : uint8_t;
} // namespace gta3sc::syntax

namespace gta3sc
{
class DiagnosticHandler;
class DiagnosticBuilder;

enum class Diag : uint32_t  // NOLINT(performance-enum-size)
{
    internal_compiler_error,
    cannot_nest_scopes,
    cannot_mix_andor,
    cannot_use_string_constant_here,
    too_many_conditions,
    too_few_arguments,  // %0 => int (expected), %1 => int (got)
    too_many_arguments, // %0 => int (expected), %1 => int (got)
    expected_token,     // %0 => Category
    expected_word,      // %0 => string
    expected_words,     // %0 => vector<string>
    expected_command,
    expected_require_command,
    expected_mission_start_at_top,
    expected_argument,
    expected_identifier,
    expected_integer,
    expected_float,
    expected_text_label,
    expected_label,
    expected_string,
    expected_input_int,
    expected_input_float,
    expected_input_opt,
    expected_variable,
    expected_subscript,
    expected_varname_after_dollar,
    expected_gvar_got_lvar,
    expected_lvar_got_gvar,
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
    duplicate_var_string_constant,
    duplicate_label,
    duplicate_script_name,
    duplicate_var_timer,
    var_decl_outside_of_scope,
    var_decl_subscript_must_be_literal,
    var_decl_subscript_must_be_nonzero,
    var_type_mismatch,
    var_entity_type_mismatch,
    subscript_must_be_positive,
    subscript_out_of_range,
    subscript_but_var_is_not_array,
    subscript_var_must_be_int,
    subscript_var_must_not_be_array,
    undefined_label,
    undefined_command,
    undefined_variable,
    alternator_mismatch,
    target_label_not_within_scope,
    target_scope_not_enough_vars,
    target_var_type_mismatch,
    target_var_entity_type_mismatch,
    codegen_label_at_local_zero_offset,
    codegen_label_ref_across_segments,
    codegen_target_does_not_support_command,
    config_models_invalid_ide_line,
    config_models_could_not_open_file, // %0 => string
};

/// Information about a diagnostic.
struct Diagnostic
{
    using Arg = std::variant<int64_t, syntax::Category, std::string,
                             std::vector<std::string>>;

    Diag message; ///< The diagnostic message.
    SourceLocation
            location; ///< Location from where the diagnostic was reported.
    std::vector<SourceRange> ranges; ///< Locations related to the diagnostic.
    std::vector<Arg> args;           ///< Arguments for formatting the message.

    explicit Diagnostic(SourceLocation location, Diag message) noexcept :
        message(message), location(location)
    {}
};

/// Helper class to construct a `Diagnostic`.
///
/// Upon destruction, this class hands the produced diagnostic to the
/// diagnostic handler given in the constructor.
class DiagnosticBuilder
{
public:
    explicit DiagnosticBuilder(SourceLocation loc, Diag message,
                               DiagnosticHandler& handler) :
        handler(&handler), diag(std::make_unique<Diagnostic>(loc, message))
    {}

    /// Hands the diagnostic to the handler.
    ~DiagnosticBuilder() noexcept;

    DiagnosticBuilder(const DiagnosticBuilder&) = delete;
    auto operator=(const DiagnosticBuilder&) -> DiagnosticBuilder& = delete;

    DiagnosticBuilder(DiagnosticBuilder&&) noexcept = default;
    auto operator=(DiagnosticBuilder&&) noexcept
            -> DiagnosticBuilder& = default;

    /// Adds a source range to provide more context to the diagnostic.
    auto range(SourceRange range) && -> DiagnosticBuilder&&
    {
        diag->ranges.push_back(range);
        return std::move(*this);
    }

    /// Adds an argument to the diagnostic.
    template<typename Arg, typename... Args>
    auto args(Arg&& arg, Args&&... args) && -> DiagnosticBuilder&&
    {
        // auto args(Args&&... args) =>
        //  ((void)diag->args.push_back(convert(std::forward<Args>(args))), ...)
        //
        // The line you see above using fold expressions crashes MSVC, thus we
        // use recursion instead to append the arguments to the diagnostic.
        diag->args.push_back(convert(std::forward<Arg>(arg)));
        return std::move(*this).args(std::forward<Args>(args)...);
    }

    /// Adds an argument to the diagnostic.
    auto args() && -> DiagnosticBuilder&& { return std::move(*this); }

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
        else if constexpr(std::is_integral_v<Ty>)
        {
            return Diagnostic::Arg(std::in_place_type_t<int64_t>(),
                                   std::forward<T>(arg));
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
    explicit DiagnosticHandler(Emitter emitter) noexcept :
        emitter(std::move(emitter))
    {}

    DiagnosticHandler(const DiagnosticHandler&) = delete;
    auto operator=(const DiagnosticHandler&) -> DiagnosticHandler& = delete;

    DiagnosticHandler(DiagnosticHandler&&) noexcept = default;
    auto operator=(DiagnosticHandler&&) noexcept
            -> DiagnosticHandler& = default;

    ~DiagnosticHandler() noexcept = default;

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
} // namespace gta3sc

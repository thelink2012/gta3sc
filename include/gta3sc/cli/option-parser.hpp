#pragma once
#include <charconv>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace gta3sc::cli
{
/// Result of attempting to match a value-bearing option.
///
/// Separates "did the option name match?" from "was a value present?".
template<typename T>
struct OptionMatch
{
    bool matched{false};    ///< The option name matched at the cursor.
    std::optional<T> value; ///< The value; empty when the match errored.

    explicit operator bool() const noexcept { return matched; }
};

/// Forward cursor over command-line arguments with GCC-style option matching.
///
/// Matchers consume from the front of the cursor on a successful match and
/// leave the cursor untouched on no-match. Errors are recorded in a parser
/// local failure state.
///
/// The returned `string_view`s during parse points to and must not outlive the
/// argument array.
class OptionParser
{
public:
    explicit OptionParser(std::span<const std::string_view> args) noexcept;

    /// Whether there are no more options to parse.
    [[nodiscard]] auto eof() const noexcept -> bool;

    /// Peeks the next token in the options stream.
    ///
    /// Returns `std::nullopt` if the stream has no more options.
    [[nodiscard]] auto peek() const noexcept -> std::optional<std::string_view>;

    /// Consumes the next token in the options stream.
    ///
    /// Returns `std::nullopt` if the stream has no more options.
    auto next() noexcept -> std::optional<std::string_view>;

    /// Consumes the next token if it is a positional (i.e. does not start
    /// with `"-"`).
    [[nodiscard]] auto positional() noexcept -> std::optional<std::string_view>;

    /// Matches a valueless option, e.g. `option("-h", "--help")`.
    ///
    /// Either \p short_opt or \p long_opt may be empty to disable that form.
    auto option(std::string_view short_opt,
                std::string_view long_opt) noexcept -> std::optional<bool>;

    /// Matches a `-fname` / `-fno-name` flag toggle.
    ///
    /// The prefix letter before \p name can be any ASCII letter (e.g. `-m`).
    ///
    /// \returns `true` for the positive form, `false` for the `no-` form, or
    /// `std::nullopt` when the cursor token does not match the option name.
    auto toggle(std::string_view name) noexcept -> std::optional<bool>;

    /// Matches a positive-only `-fname`-style flag.
    ///
    /// \p name is the full positive spelling (e.g. `"-fsomething"`).
    ///
    /// \see toggle for more details.
    auto flag(std::string_view name) noexcept -> std::optional<bool>;

    /// Matches an option taking exactly one value (e.g. `--opt=v`, `--opt v`,
    /// `-ov`, `-o v`).
    ///
    /// Either \p short_opt or \p long_opt may be empty to disable that form.
    ///
    /// On a name match with a missing value, records a failure and the
    /// returned `OptionMatch.value` is `std::nullopt`.
    auto value(std::string_view short_opt, std::string_view long_opt) noexcept
            -> OptionMatch<std::string_view>;

    /// Matches a value option whose value is an integer.
    ///
    /// Records a failure when the value is absent, not an integer, or out of
    /// range for \p IntegralType.
    ///
    /// \see value for more details.
    template<typename IntegralType>
    auto
    value_int(std::string_view long_opt) noexcept -> OptionMatch<IntegralType>;

    /// Whether a matcher hit a recognised-but-invalid argument.
    [[nodiscard]] auto failed() const noexcept -> bool;

    /// The current failure message (valid when \ref failed).
    [[nodiscard]] auto error() const noexcept -> std::string_view;

    /// Records a failure with the given message.
    ///
    /// Overwrites any previously recorded failure.
    void fail(std::string message) noexcept;

    /// Records a failure with a formatted message.
    ///
    /// Overwrites any previously recorded failure.
    template<typename... Args>
    void fail(std::format_string<Args...> fmt, Args&&... args) noexcept
    {
        fail(std::format(fmt, std::forward<Args>(args)...));
    }

private:
    /// Advances the cursor by one argument.
    void advance() noexcept;

private:
    std::span<const std::string_view> args; ///< Remaining unparsed arguments.
    std::string error_message;              ///< Failure message; see error().
    bool has_failed{false};
};

template<typename IntegralType>
auto OptionParser::value_int(std::string_view long_opt) noexcept
        -> OptionMatch<IntegralType>
{
    auto m = value("", long_opt);
    if(!m || !m.value)
        return {m.matched, std::nullopt};

    IntegralType result{};
    auto [ptr, ec] = std::from_chars(m.value->data(),
                                     m.value->data() + m.value->size(), result);
    if(ec != std::errc{} || ptr != m.value->data() + m.value->size())
    {
        fail("invalid integer value '{}' for option '{}'", *m.value, long_opt);
        return {true, std::nullopt};
    }
    return {true, result};
}

} // namespace gta3sc::cli

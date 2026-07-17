#include <cassert>
#include <gta3sc/cli/option-parser.hpp>

namespace gta3sc::cli
{
OptionParser::OptionParser(std::span<const std::string_view> args) noexcept :
    args(args)
{}

void OptionParser::advance() noexcept
{
    args = args.subspan(1);
}

auto OptionParser::eof() const noexcept -> bool
{
    return args.empty();
}

auto OptionParser::peek() const noexcept -> std::optional<std::string_view>
{
    if(eof())
        return std::nullopt;
    return args.front();
}

auto OptionParser::next() noexcept -> std::optional<std::string_view>
{
    if(eof())
        return std::nullopt;
    auto tok = args.front();
    advance();
    return tok;
}

auto OptionParser::positional() noexcept -> std::optional<std::string_view>
{
    auto tok = peek();
    if(!tok || tok->starts_with("-"))
        return std::nullopt;
    advance();
    return tok;
}

auto OptionParser::option(std::string_view short_opt,
                          std::string_view long_opt) noexcept
        -> std::optional<bool>
{
    auto tok = peek();
    if(!tok)
        return std::nullopt;

    bool matched = (!short_opt.empty() && *tok == short_opt)
                   || (!long_opt.empty() && *tok == long_opt);
    if(!matched)
        return std::nullopt;

    advance();
    return true;
}

auto OptionParser::toggle(std::string_view name) noexcept -> std::optional<bool>
{
    auto tok = peek();
    if(!tok)
        return std::nullopt;

    // Positive form: exact match of `-Xfeature`.
    if(*tok == name)
    {
        advance();
        return true;
    }

    // Negative form: `-Xno-feature` when `name` is `-Xfeature`.
    if(name.size() < 2)
        return std::nullopt;

    const auto prefix = name.substr(0, 2); // "-X"
    const auto feature = name.substr(2);   // "feature"
    if(!tok->starts_with(prefix))
        return std::nullopt;

    const auto after_prefix = tok->substr(prefix.size());
    constexpr std::string_view no{"no-"};
    if(!after_prefix.starts_with(no)
       || after_prefix.substr(no.size()) != feature)
        return std::nullopt;

    advance();
    return false;
}

auto OptionParser::flag(std::string_view name) noexcept -> std::optional<bool>
{
    auto tok = peek();
    if(!tok || *tok != name)
        return std::nullopt;

    advance();
    return true;
}

auto OptionParser::value(std::string_view short_opt,
                         std::string_view long_opt) noexcept
        -> OptionMatch<std::string_view>
{
    auto tok = peek();
    if(!tok)
        return {};

    // Long form: --opt=value or --opt value
    if(!long_opt.empty())
    {
        if(*tok == long_opt)
        {
            advance();
            auto v = next();
            if(!v)
            {
                fail("option '{}' requires an argument", long_opt);
                return {true, std::nullopt};
            }
            return {true, v};
        }

        if(tok->size() > long_opt.size() + 1 && tok->starts_with(long_opt)
           && (*tok)[long_opt.size()] == '=')
        {
            advance();
            return {true, tok->substr(long_opt.size() + 1)};
        }
    }

    // Short form: -o value or -ovalue
    if(!short_opt.empty())
    {
        if(*tok == short_opt)
        {
            advance();
            auto v = next();
            if(!v)
            {
                fail("option '{}' requires an argument", short_opt);
                return {true, std::nullopt};
            }
            return {true, v};
        }

        if(tok->starts_with(short_opt) && tok->size() > short_opt.size())
        {
            advance();
            return {true, tok->substr(short_opt.size())};
        }
    }

    return {};
}

auto OptionParser::failed() const noexcept -> bool
{
    return has_failed;
}

auto OptionParser::error() const noexcept -> std::string_view
{
    assert(has_failed);
    return error_message;
}

void OptionParser::fail(std::string message) noexcept
{
    has_failed = true;
    error_message = std::move(message);
}

} // namespace gta3sc::cli

#include "doctest.h"
#include <gta3sc/scanner.hpp>
#include <cstring>
#include <ostream>
using gta3sc::Category;

namespace
{
template<std::size_t N>
auto make_source(const char (&data)[N]) -> gta3sc::SourceFile
{
    auto ptr = std::make_unique<char[]>(N);
    std::memcpy(ptr.get(), data, N);
    return gta3sc::SourceFile(std::move(ptr), N-1);
}

auto make_scanner(const gta3sc::SourceFile& source) -> gta3sc::Scanner
{
    auto pp = gta3sc::Preprocessor(source);
    return gta3sc::Scanner(std::move(pp));
}
}

namespace gta3sc
{
std::ostream& operator<<(std::ostream& os, const Category& category)
{
    os << "Category(";
    for(uint64_t i = 0, mask = 1; i != 63; mask <<= 1, ++i)
    {
        if(static_cast<uint64_t>(category) & mask)
            os << "bit " << i;
    }
    os << ")";
    return os;
}
}

TEST_CASE("scanner with empty stream")
{
    auto source = make_source("");
    auto scanner = make_scanner(source);

    CHECK(scanner.eof());
    CHECK(scanner.next(Category::EndOfLine)->category == Category::EndOfLine);
    CHECK(scanner.eof());
    CHECK(scanner.next(Category::Command) == std::nullopt);
    CHECK(scanner.next(Category::EndOfLine)->lexeme == "");
    CHECK(scanner.next(Category::EndOfLine)->category == Category::EndOfLine);
    CHECK(scanner.eof());
}

TEST_CASE("scanner with leading and trailing whitespaces")
{
    auto source = make_source("  , COMMAND  (\t)  \n");
    auto scanner = make_scanner(source);
    CHECK(scanner.next(Category::Command)->category == Category::Command);
    CHECK(scanner.next(Category::EndOfLine)->category == Category::EndOfLine);
}

TEST_CASE("scanner with whitespaces in the middle")
{
    auto source = make_source("  , COMMAND  1,\t,2  (\t)  ");
    auto scanner = make_scanner(source);
    CHECK(scanner.next(Category::Command)->category == Category::Command);
    CHECK(scanner.next(Category::Whitespace)->category == Category::Whitespace);
    CHECK(scanner.next(Category::Integer)->category == Category::Integer);
    CHECK(scanner.next(Category::Whitespace)->category == Category::Whitespace);
    CHECK(scanner.next(Category::Integer)->category == Category::Integer);
    CHECK(scanner.next(Category::EndOfLine)->category == Category::EndOfLine);
}

TEST_CASE("scanner with integer")
{
    auto source = make_source("  123 010 -39 -432-10   123a 0x10 +39 432+10  ");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {"123", Category::Integer},
        {"010", Category::Integer},
        {"-39", Category::Integer},
        {"-432-10", Category::Integer},
        {"123a", std::nullopt},
        {"0x10", std::nullopt},
        {"+39", std::nullopt},
        {"432+10", std::nullopt},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Integer);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Integer);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with float")
{
    auto source = make_source(
                              // integer
                              "  1"
                              "  -1"
                              // floating_form1
                              "  .1"
                              "  -.1"
                              "  .1f"
                              "  .1F"
                              "  .11"
                              "  .1.9"
                              "  .1a"
                              "  .1fa"
                              "  .1.a"
                              "  .1-.1"
                              "  -.1."
                              // floating_form2
                              "  1F"
                              "  -1f"
                              "  1."
                              "  1.1"
                              "  1.f"
                              "  1.."
                              "  -1.."
                              "  1..a");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {"1", std::nullopt},
        {"-1", std::nullopt},
        {".1", Category::Float},
        {"-.1", Category::Float},
        {".1f", Category::Float},
        {".1F", Category::Float},
        {".11", Category::Float},
        {".1.9", Category::Float},
        {".1a", std::nullopt},
        {".1fa", std::nullopt},
        {".1.a", std::nullopt},
        {".1-.1", std::nullopt},
        {"-.1.", Category::Float},
        {"1F", Category::Float},
        {"-1f", Category::Float},
        {"1.", Category::Float},
        {"1.1", Category::Float},
        {"1.f", Category::Float},
        {"1..", Category::Float},
        {"-1..", Category::Float},
        {"1..a", std::nullopt},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Float);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Float);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with identifier")
{
    auto source = make_source(" $abc abc AbC _abc @abc 1abc a@_1$ abc: ");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {"$abc", Category::Identifier},
        {"abc", Category::Identifier},
        {"AbC", Category::Identifier},
        {"_abc", std::nullopt},
        {"@abc", std::nullopt},
        {"1abc", std::nullopt},
        {"a@_1$", Category::Identifier},
        {"abc:", std::nullopt},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Identifier);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Identifier);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with string literal")
{
    auto source = make_source(" \"this\tI$ /* a // \\n (%1teral),\" \n"
                              " \"                                  \n"
                              " \"\"                                \n"
                              " \"string\"abc                       \n"
                              " not_string                          \n");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {"\"this\tI$ /* a // \\n (%1teral),\"", Category::String},
        {"\"", std::nullopt},
        {"\"\"", Category::String},
        {"\"string\"abc", std::nullopt},
        {"not_string", std::nullopt},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::String);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::String);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with filename")
{
    auto source = make_source(" .sc a.SC @.sc 1.sc 1.0sc SC C \n"
                              " \"a\".sc                      \n"
                              " filename.sc                   \n");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {".sc", Category::Filename},
        {"a.SC", Category::Filename},
        {"@.sc", Category::Filename},
        {"1.sc", Category::Filename},
        {"1.0sc", std::nullopt},
        {"SC", std::nullopt},
        {"C", std::nullopt},
        {"\"a\".sc", std::nullopt},
        {"filename.sc", Category::Filename},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Filename);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Filename);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with label")
{
    auto source = make_source(" : ::: abc: AbC: abc %: $: .: 1: 1.: \n"
                              " lab\"el\": \n"
                              " \"label\": \n"
                              " lab\"el:   \n"
                              " label:     \n");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {":", Category::Label},
        {":::", Category::Label},
        {"abc:", Category::Label},
        {"AbC:", Category::Label},
        {"abc", std::nullopt},
        {"%:", Category::Label},
        {"$:", Category::Label},
        {".:", Category::Label},
        {"1:", Category::Label},
        {"1.:", Category::Label},
        {"lab\"el\":", std::nullopt},
        {"\"label\":", std::nullopt},
        {"lab\"el:", std::nullopt},
        {"label:", Category::Label},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Label);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Label);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            if(token) fprintf(stderr, ">>%s\n", std::string(token->lexeme).c_str());
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with command")
{
    auto source = make_source(" C c c: a.sc \"a\" % $ 1 .1 -1 \n"
                              " \"C\"                         \n"
                              " C\"C                          \n"
                              " C\"C\"D                       \n"
                              " COMMAND                       \n");
    auto scanner = make_scanner(source);

    const std::pair<std::string_view, std::optional<Category>> expected[] {
        {"C", Category::Command},
        {"c", Category::Command},
        {"c:", Category::Command},
        {"a.sc", Category::Command},
        {"\"a\"", std::nullopt},
        {"%", Category::Command},
        {"$", Category::Command},
        {"1", Category::Command},
        {".1", Category::Command},
        {"-1", Category::Command},
        {"\"C\"", std::nullopt},
        {"C\"C", std::nullopt},
        {"C\"C\"D", std::nullopt},
        {"COMMAND", Category::Command},
    };

    for(auto [lexeme, category] : expected)
    {
        auto token = scanner.next(Category::Command);
        if(category)
        {
            CHECK(token != std::nullopt);
            CHECK(token->category == Category::Command);
            CHECK(token->lexeme == lexeme);
        }
        else
        {
            CHECK(token == std::nullopt);
        }

        auto ws = scanner.next(Category::Whitespace | Category::EndOfLine);
        CHECK((ws->category == Category::Whitespace 
                    || ws->category == Category::EndOfLine));
    }

    CHECK(scanner.eof());
}

TEST_CASE("scanner with operators not in expression")
{
    auto source = make_source(" += -1   \n"
                             "command / \n"
                             "  *= +@ * \n");
    auto scanner = make_scanner(source);

    CHECK(scanner.next(Category::PlusEqual) == std::nullopt);
    CHECK(scanner.next(Category::Whitespace) != std::nullopt);
    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Command) != std::nullopt);
    CHECK(scanner.next(Category::Whitespace) != std::nullopt);
    CHECK(scanner.next(Category::Slash) == std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Command) != std::nullopt);
    CHECK(scanner.next(Category::Whitespace) != std::nullopt);
    CHECK(scanner.next(Category::Command) != std::nullopt);
    CHECK(scanner.next(Category::Whitespace) != std::nullopt);
    CHECK(scanner.next(Category::Command) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.eof());
}

TEST_CASE("scanner with operators in expression")
{
    auto source = make_source("1 += -1        \n"
                              "x = command / 2\n"
                              "command *= 2   \n"
                              "a++            \n"
                              "a < 2          \n");
    auto scanner = make_scanner(source);

    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::PlusEqual) != std::nullopt);
    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Identifier) != std::nullopt);
    CHECK(scanner.next(Category::Equal) != std::nullopt);
    CHECK(scanner.next(Category::Command) == std::nullopt);
    CHECK(scanner.next(Category::Slash) != std::nullopt);
    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Command) == std::nullopt);
    CHECK(scanner.next(Category::StarEqual) != std::nullopt);
    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Identifier) != std::nullopt);
    CHECK(scanner.next(Category::PlusPlus) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::Identifier) != std::nullopt);
    CHECK(scanner.next(Category::Less) != std::nullopt);
    CHECK(scanner.next(Category::Integer) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.eof());
}

TEST_CASE("scanner chooses expression or command based on context")
{
    auto source = make_source("--b        \n"
                              "--b c      \n");
    auto scanner = make_scanner(source);

    CHECK(scanner.next(Category::MinusMinus | Category::Command)->category 
            == Category::MinusMinus);
    CHECK(scanner.next(Category::Identifier) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.next(Category::MinusMinus | Category::Command)->category
            == Category::Command);
    CHECK(scanner.next(Category::Whitespace) != std::nullopt);
    CHECK(scanner.next(Category::Identifier) != std::nullopt);
    CHECK(scanner.next(Category::EndOfLine) != std::nullopt);

    CHECK(scanner.eof());
}

#include "syntax-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/syntax/scanner.hpp>
using namespace gta3sc::test::syntax; // NOLINT
using gta3sc::syntax::Category;
using gta3sc::syntax::Token;

namespace gta3sc::test::syntax
{
class ScannerFixture : public SyntaxFixture
{
public:
    ScannerFixture() :
        scanner(gta3sc::syntax::Preprocessor(make_source(""), diagman))
    {}

protected:
    void build_scanner(std::string_view src)
    {
        auto pp = gta3sc::syntax::Preprocessor(make_source(src), diagman);
        this->scanner = gta3sc::syntax::Scanner(std::move(pp));
    }

    auto spelling(const Token& token) const -> std::string_view
    {
        return scanner.spelling(token);
    }

protected:
    gta3sc::syntax::Scanner scanner; // NOLINT
};
} // namespace gta3sc::test::syntax

namespace gta3sc::syntax
{
auto operator<<(std::ostream& os, const Category& category) -> std::ostream&
{
    os << "Category(" << static_cast<uint32_t>(category) << ")";
    return os;
}
} // namespace gta3sc::syntax

TEST_CASE_FIXTURE(ScannerFixture, "scanner with empty stream")
{
    build_scanner("");

    REQUIRE(!scanner.eof());
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
    REQUIRE(spelling(*scanner.next()) == "");
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture,
                  "scanner with leading and trailing whitespaces")
{
    build_scanner("  , COMMAND  (\t)  \n");
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::EndOfLine);
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with whitespaces in the middle")
{
    build_scanner("  , COMMAND  1,\t,2  (\t)  ");
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::EndOfLine);
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with word")
{
    build_scanner("1234 123a -123a -.abc         \n"
                  "4x4.sc .sc                    \n"
                  "word: word: word              \n"
                  "%$&~ AbC {}                   \n");

    gta3sc::syntax::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1234");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-.abc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "4x4.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "word");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "%$&~");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "AbC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "{}");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with string literal")
{
    build_scanner(" \"this\tI$ /* a // \\n (%1teral),\" \n"
                  " \"                                  \n"
                  " \"\"                                \n"
                  " \"string\"abc                       \n"
                  " not_string                          \n");

    gta3sc::syntax::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(spelling(token) == "\"this\tI$ /* a // \\n (%1teral),\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(consume_diag().message
            == gta3sc::Diag::unterminated_string_literal);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(spelling(token) == "\"\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(spelling(token) == "\"string\"");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "abc"); // fine at scanning time
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "not_string");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with filename")
{
    build_scanner(" .sc a.SC @.sc 1.sc 1.0sc SC   \n"
                  " b\"a\".sc                     \n"
                  " file-nam+@e.sc                \n"
                  " file-nam+@e.sc                \n");

    gta3sc::syntax::Token token;

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "a.SC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "@.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // 1.0sc
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    REQUIRE(scanner.next()->category == Category::Whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // SC
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.next_filename() == std::nullopt); // b
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(spelling(token) == "\"a\"");
    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "file-nam+@e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "file");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "nam");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(spelling(token) == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

// NOLINTNEXTLINE: FIXME test is too big and needs refactoring
TEST_CASE_FIXTURE(ScannerFixture, "scanner with operators")
{
    build_scanner("+ - * / +@ -@        \n"
                  "+= -= *= /= +=@ -=@  \n"
                  "<= < = =# > >=       \n"
                  "--++ - -             \n"
                  "<< <=> +-*/+@-@      \n"
                  "1--1 1- -1 +1        \n"
                  "-. -.1 -1.0          \n"
                  "+ @   - @   = #  + = \n");

    gta3sc::syntax::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(spelling(token) == "*");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(spelling(token) == "/");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(spelling(token) == "+@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(spelling(token) == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqual);
    REQUIRE(spelling(token) == "+=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqual);
    REQUIRE(spelling(token) == "-=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::StarEqual);
    REQUIRE(spelling(token) == "*=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::SlashEqual);
    REQUIRE(spelling(token) == "/=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqualAt);
    REQUIRE(spelling(token) == "+=@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqualAt);
    REQUIRE(spelling(token) == "-=@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(spelling(token) == "<=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(spelling(token) == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::EqualHash);
    REQUIRE(spelling(token) == "=#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(spelling(token) == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::GreaterEqual);
    REQUIRE(spelling(token) == ">=");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(spelling(token) == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusPlus);
    REQUIRE(spelling(token) == "++");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(spelling(token) == "<");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(spelling(token) == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(spelling(token) == "<=");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(spelling(token) == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(spelling(token) == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(spelling(token) == "*");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(spelling(token) == "/");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(spelling(token) == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(spelling(token) == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(spelling(token) == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(spelling(token) == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "1");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-.");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-.1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "-1.0");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(spelling(token) == "#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with invalid ASCII")
{
    build_scanner("HI \x01 BYE\n"
                  "\"HI \x02 BYE\"\n");

    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_char);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    // TODO maybe we should emit an warning for this case
    REQUIRE(scanner.next()->category == Category::String);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

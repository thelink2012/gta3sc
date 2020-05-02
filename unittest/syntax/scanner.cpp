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

    [[nodiscard]] auto spelling(const Token& token) const -> std::string_view
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
    REQUIRE(scanner.next()->category == Category::end_of_line);
    REQUIRE(scanner.eof());
    REQUIRE(spelling(*scanner.next()) == "");
    REQUIRE(scanner.next()->category == Category::end_of_line);
    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture,
                  "scanner with leading and trailing whitespaces")
{
    build_scanner("  , COMMAND  (\t)  \n");
    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::end_of_line);
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with whitespaces in the middle")
{
    build_scanner("  , COMMAND  1,\t,2  (\t)  ");
    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::whitespace);
    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::whitespace);
    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::end_of_line);
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with word")
{
    build_scanner("1234 123a -123a -.abc         \n"
                  "4x4.sc .sc                    \n"
                  "word: word: word              \n"
                  "%$&~ AbC {}                   \n");

    gta3sc::syntax::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1234");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "123a");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-123a");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-.abc");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "4x4.sc");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "word:");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "word:");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "word");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "%$&~");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "AbC");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "{}");
    REQUIRE(scanner.next()->category == Category::end_of_line);

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
    REQUIRE(token.category == Category::string);
    REQUIRE(spelling(token) == "\"this\tI$ /* a // \\n (%1teral),\"");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(consume_diag().message
            == gta3sc::Diag::unterminated_string_literal);
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::string);
    REQUIRE(spelling(token) == "\"\"");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::string);
    REQUIRE(spelling(token) == "\"string\"");
    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "abc"); // fine at scanning time
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "not_string");
    REQUIRE(scanner.next()->category == Category::end_of_line);

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
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "a.SC");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "@.sc");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1.sc");
    REQUIRE(scanner.next()->category == Category::whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // 1.0sc
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    REQUIRE(scanner.next()->category == Category::whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // SC
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    REQUIRE(scanner.next()->category == Category::end_of_line);

    REQUIRE(scanner.next_filename() == std::nullopt); // b
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_filename);
    token = scanner.next().value();
    REQUIRE(token.category == Category::string);
    REQUIRE(spelling(token) == "\"a\"");
    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == ".sc");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "file-nam+@e.sc");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "file");
    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "nam");
    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_at);
    REQUIRE(spelling(token) == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "e.sc");
    REQUIRE(scanner.next()->category == Category::end_of_line);

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
    REQUIRE(token.category == Category::plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::star);
    REQUIRE(spelling(token) == "*");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::slash);
    REQUIRE(spelling(token) == "/");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_at);
    REQUIRE(spelling(token) == "+@");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_at);
    REQUIRE(spelling(token) == "-@");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_equal);
    REQUIRE(spelling(token) == "+=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_equal);
    REQUIRE(spelling(token) == "-=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::star_equal);
    REQUIRE(spelling(token) == "*=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::slash_equal);
    REQUIRE(spelling(token) == "/=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_equal_at);
    REQUIRE(spelling(token) == "+=@");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_equal_at);
    REQUIRE(spelling(token) == "-=@");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::less_equal);
    REQUIRE(spelling(token) == "<=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::less);
    REQUIRE(spelling(token) == "<");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::equal_hash);
    REQUIRE(spelling(token) == "=#");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::greater);
    REQUIRE(spelling(token) == ">");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::greater_equal);
    REQUIRE(spelling(token) == ">=");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_minus);
    REQUIRE(spelling(token) == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_plus);
    REQUIRE(spelling(token) == "++");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::less);
    REQUIRE(spelling(token) == "<");
    token = scanner.next().value();
    REQUIRE(token.category == Category::less);
    REQUIRE(spelling(token) == "<");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::less_equal);
    REQUIRE(spelling(token) == "<=");
    token = scanner.next().value();
    REQUIRE(token.category == Category::greater);
    REQUIRE(spelling(token) == ">");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus);
    REQUIRE(spelling(token) == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::star);
    REQUIRE(spelling(token) == "*");
    token = scanner.next().value();
    REQUIRE(token.category == Category::slash);
    REQUIRE(spelling(token) == "/");
    token = scanner.next().value();
    REQUIRE(token.category == Category::plus_at);
    REQUIRE(spelling(token) == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_at);
    REQUIRE(spelling(token) == "-@");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::minus_minus);
    REQUIRE(spelling(token) == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-1");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus);
    REQUIRE(spelling(token) == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "1");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-.");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-.1");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "-1.0");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "@");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::minus);
    REQUIRE(spelling(token) == "-");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "@");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::word);
    REQUIRE(spelling(token) == "#");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::plus);
    REQUIRE(spelling(token) == "+");
    REQUIRE(scanner.next()->category == Category::whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::equal);
    REQUIRE(spelling(token) == "=");
    REQUIRE(scanner.next()->category == Category::end_of_line);

    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with invalid ASCII")
{
    build_scanner("HI \x01 BYE\n"
                  "\"HI \x02 BYE\"\n");

    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::whitespace);
    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(consume_diag().message == gta3sc::Diag::invalid_char);
    REQUIRE(scanner.next()->category == Category::whitespace);
    REQUIRE(scanner.next()->category == Category::word);
    REQUIRE(scanner.next()->category == Category::end_of_line);

    // TODO maybe we should emit an warning for this case
    REQUIRE(scanner.next()->category == Category::string);
    REQUIRE(scanner.next()->category == Category::end_of_line);

    REQUIRE(scanner.eof());
}

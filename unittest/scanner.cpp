#include <doctest.h>
#include <gta3sc/scanner.hpp>
#include "compiler-fixture.hpp"
#include <ostream>
using gta3sc::Category;

namespace
{
class ScannerFixture : public CompilerFixture
{
public:
    ScannerFixture() :
        scanner(gta3sc::Preprocessor(source_file, diagman))
    {}

protected:
    void build_scanner(std::string_view src)
    {
        this->build_source(src);
        auto pp = gta3sc::Preprocessor(source_file, diagman);
        this->scanner = gta3sc::Scanner(std::move(pp));
    }

protected:
    gta3sc::Scanner scanner;
};
}

namespace gta3sc
{
std::ostream& operator<<(std::ostream& os, const Category& category)
{
    os << "Category(" << static_cast<uint32_t>(category) << ")";
    return os;
}
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with empty stream")
{
    build_scanner("");

    REQUIRE(!scanner.eof());
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
    REQUIRE(scanner.next()->spelling() == "");
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with leading and trailing whitespaces")
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

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1234");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-.abc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "4x4.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "word");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "%$&~");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "AbC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "{}");
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

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.spelling() == "\"this\tI$ /* a // \\n (%1teral),\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(consume_diag().message == gta3sc::Diag::unterminated_string_literal);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.spelling() == "\"\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.spelling() == "\"string\"");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "abc"); // fine at scanning time
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "not_string");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE_FIXTURE(ScannerFixture, "scanner with filename")
{
    build_scanner(" .sc a.SC @.sc 1.sc 1.0sc SC   \n"
                              " b\"a\".sc                     \n"
                              " file-nam+@e.sc                \n"
                              " file-nam+@e.sc                \n");


    gta3sc::Token token;

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == ".sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "a.SC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "@.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1.sc");
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
    REQUIRE(token.spelling() == "\"a\"");
    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "file-nam+@e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "file");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "nam");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.spelling() == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

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

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.spelling() == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(token.spelling() == "*");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(token.spelling() == "/");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.spelling() == "+@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(token.spelling() == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqual);
    REQUIRE(token.spelling() == "+=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqual);
    REQUIRE(token.spelling() == "-=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::StarEqual);
    REQUIRE(token.spelling() == "*=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::SlashEqual);
    REQUIRE(token.spelling() == "/=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqualAt);
    REQUIRE(token.spelling() == "+=@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqualAt);
    REQUIRE(token.spelling() == "-=@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(token.spelling() == "<=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.spelling() == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.spelling() == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::EqualHash);
    REQUIRE(token.spelling() == "=#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(token.spelling() == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::GreaterEqual);
    REQUIRE(token.spelling() == ">=");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(token.spelling() == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusPlus);
    REQUIRE(token.spelling() == "++");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.spelling() == "<");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.spelling() == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(token.spelling() == "<=");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(token.spelling() == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.spelling() == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(token.spelling() == "*");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(token.spelling() == "/");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.spelling() == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(token.spelling() == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(token.spelling() == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.spelling() == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "1");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-.");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-.1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "-1.0");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.spelling() == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.spelling() == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.spelling() == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.spelling() == "#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.spelling() == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.spelling() == "=");
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

#include <doctest.h>
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
    os << "Category(" << static_cast<uint32_t>(category) << ")";
    return os;
}
}

TEST_CASE("scanner with empty stream")
{
    auto source = make_source("");
    auto scanner = make_scanner(source);

    REQUIRE(!scanner.eof());
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
    REQUIRE(scanner.next()->lexeme == "");
    REQUIRE(scanner.next()->category == Category::EndOfLine);
    REQUIRE(scanner.eof());
}

TEST_CASE("scanner with leading and trailing whitespaces")
{
    auto source = make_source("  , COMMAND  (\t)  \n");
    auto scanner = make_scanner(source);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::EndOfLine);
}

TEST_CASE("scanner with whitespaces in the middle")
{
    auto source = make_source("  , COMMAND  1,\t,2  (\t)  ");
    auto scanner = make_scanner(source);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::Whitespace);
    REQUIRE(scanner.next()->category == Category::Word);
    REQUIRE(scanner.next()->category == Category::EndOfLine);
}

TEST_CASE("scanner with word")
{
    auto source = make_source("1234 123a -123a -.abc         \n"
                              "4x4.sc .sc                    \n"
                              "word: word: word              \n"
                              "%$&~ AbC {}                   \n");
    auto scanner = make_scanner(source);

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1234");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-123a");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-.abc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "4x4.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "word:");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "word");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "%$&~");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "AbC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "{}");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE("scanner with string literal")
{
    auto source = make_source(" \"this\tI$ /* a // \\n (%1teral),\" \n"
                              " \"                                  \n"
                              " \"\"                                \n"
                              " \"string\"abc                       \n"
                              " not_string                          \n");
    auto scanner = make_scanner(source);

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.lexeme == "\"this\tI$ /* a // \\n (%1teral),\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.next() == std::nullopt);
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.lexeme == "\"\"");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.lexeme == "\"string\"");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "abc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "not_string");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE("scanner with filename")
{
    auto source = make_source(" .sc a.SC @.sc 1.sc 1.0sc SC   \n"
                              " b\"a\".sc                     \n"
                              " file-nam+@e.sc                \n"
                              " file-nam+@e.sc                \n");
    auto scanner = make_scanner(source);


    gta3sc::Token token;

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == ".sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "a.SC");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "@.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1.sc");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // 1.0sc
    REQUIRE(scanner.next()->category == Category::Whitespace);

    REQUIRE(scanner.next_filename() == std::nullopt); // SC
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.next_filename() == std::nullopt); // b
    token = scanner.next().value();
    REQUIRE(token.category == Category::String);
    REQUIRE(token.lexeme == "\"a\"");
    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == ".sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next_filename().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "file-nam+@e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "file");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "nam");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.lexeme == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "e.sc");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

TEST_CASE("scanner with operators")
{
    auto source = make_source("+ - * / +@ -@        \n"
                              "+= -= *= /= +=@ -=@  \n"
                              "<= < = =# > >=       \n"
                              "--++ - -             \n"
                              "<< <=> +-*/+@-@      \n"
                              "1--1 1- -1 +1        \n"
                              "-. -.1 -1.0          \n"
                              "+ @   - @   = #  + = \n");
    auto scanner = make_scanner(source);

    gta3sc::Token token;

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.lexeme == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(token.lexeme == "*");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(token.lexeme == "/");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.lexeme == "+@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(token.lexeme == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqual);
    REQUIRE(token.lexeme == "+=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqual);
    REQUIRE(token.lexeme == "-=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::StarEqual);
    REQUIRE(token.lexeme == "*=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::SlashEqual);
    REQUIRE(token.lexeme == "/=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusEqualAt);
    REQUIRE(token.lexeme == "+=@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusEqualAt);
    REQUIRE(token.lexeme == "-=@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(token.lexeme == "<=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.lexeme == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.lexeme == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::EqualHash);
    REQUIRE(token.lexeme == "=#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(token.lexeme == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::GreaterEqual);
    REQUIRE(token.lexeme == ">=");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(token.lexeme == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusPlus);
    REQUIRE(token.lexeme == "++");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.lexeme == "<");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Less);
    REQUIRE(token.lexeme == "<");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::LessEqual);
    REQUIRE(token.lexeme == "<=");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Greater);
    REQUIRE(token.lexeme == ">");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.lexeme == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Star);
    REQUIRE(token.lexeme == "*");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Slash);
    REQUIRE(token.lexeme == "/");
    token = scanner.next().value();
    REQUIRE(token.category == Category::PlusAt);
    REQUIRE(token.lexeme == "+@");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusAt);
    REQUIRE(token.lexeme == "-@");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::MinusMinus);
    REQUIRE(token.lexeme == "--");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.lexeme == "+");
    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "1");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-.");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-.1");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "-1.0");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.lexeme == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Minus);
    REQUIRE(token.lexeme == "-");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "@");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.lexeme == "=");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Word);
    REQUIRE(token.lexeme == "#");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Plus);
    REQUIRE(token.lexeme == "+");
    REQUIRE(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    REQUIRE(token.category == Category::Equal);
    REQUIRE(token.lexeme == "=");
    REQUIRE(scanner.next()->category == Category::EndOfLine);

    REQUIRE(scanner.eof());
}

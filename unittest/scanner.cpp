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
    os << "Category(" << static_cast<uint32_t>(category) << ")";
    return os;
}
}

TEST_CASE("scanner with empty stream")
{
    auto source = make_source("");
    auto scanner = make_scanner(source);

    CHECK(!scanner.eof());
    CHECK(scanner.next()->category == Category::EndOfLine);
    CHECK(scanner.eof());
    CHECK(scanner.next()->lexeme == "");
    CHECK(scanner.next()->category == Category::EndOfLine);
    CHECK(scanner.eof());
}

TEST_CASE("scanner with leading and trailing whitespaces")
{
    auto source = make_source("  , COMMAND  (\t)  \n");
    auto scanner = make_scanner(source);
    CHECK(scanner.next()->category == Category::Word);
    CHECK(scanner.next()->category == Category::EndOfLine);
}

TEST_CASE("scanner with whitespaces in the middle")
{
    auto source = make_source("  , COMMAND  1,\t,2  (\t)  ");
    auto scanner = make_scanner(source);
    CHECK(scanner.next()->category == Category::Word);
    CHECK(scanner.next()->category == Category::Whitespace);
    CHECK(scanner.next()->category == Category::Word);
    CHECK(scanner.next()->category == Category::Whitespace);
    CHECK(scanner.next()->category == Category::Word);
    CHECK(scanner.next()->category == Category::EndOfLine);
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
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1234");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "123a");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-123a");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-.abc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "4x4.sc");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == ".sc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "word:");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "word:");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "word");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "%$&~");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "AbC");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "{}");
    CHECK(scanner.next()->category == Category::EndOfLine);

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

    gta3sc::Token token;

    token = scanner.next().value();
    CHECK(token.category == Category::String);
    CHECK(token.lexeme == "\"this\tI$ /* a // \\n (%1teral),\"");
    CHECK(scanner.next()->category == Category::EndOfLine);

    CHECK(scanner.next() == std::nullopt);
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::String);
    CHECK(token.lexeme == "\"\"");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::String);
    CHECK(token.lexeme == "\"string\"");
    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "abc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "not_string");
    CHECK(scanner.next()->category == Category::EndOfLine);

    CHECK(scanner.eof());
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
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == ".sc");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "a.SC");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "@.sc");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next_filename().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1.sc");
    CHECK(scanner.next()->category == Category::Whitespace);

    CHECK(scanner.next_filename() == std::nullopt); // 1.0sc
    CHECK(scanner.next()->category == Category::Whitespace);

    CHECK(scanner.next_filename() == std::nullopt); // SC
    CHECK(scanner.next()->category == Category::EndOfLine);

    CHECK(scanner.next_filename() == std::nullopt); // b
    token = scanner.next().value();
    CHECK(token.category == Category::String);
    CHECK(token.lexeme == "\"a\"");
    token = scanner.next_filename().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == ".sc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next_filename().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "file-nam+@e.sc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "file");
    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "nam");
    token = scanner.next().value();
    CHECK(token.category == Category::PlusAt);
    CHECK(token.lexeme == "+@");
    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "e.sc");
    CHECK(scanner.next()->category == Category::EndOfLine);

    CHECK(scanner.eof());
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
    CHECK(token.category == Category::Plus);
    CHECK(token.lexeme == "+");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Star);
    CHECK(token.lexeme == "*");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Slash);
    CHECK(token.lexeme == "/");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::PlusAt);
    CHECK(token.lexeme == "+@");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::MinusAt);
    CHECK(token.lexeme == "-@");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::PlusEqual);
    CHECK(token.lexeme == "+=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::MinusEqual);
    CHECK(token.lexeme == "-=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::StarEqual);
    CHECK(token.lexeme == "*=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::SlashEqual);
    CHECK(token.lexeme == "/=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::PlusEqualAt);
    CHECK(token.lexeme == "+=@");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::MinusEqualAt);
    CHECK(token.lexeme == "-=@");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::LessEqual);
    CHECK(token.lexeme == "<=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Less);
    CHECK(token.lexeme == "<");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Equal);
    CHECK(token.lexeme == "=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::EqualHash);
    CHECK(token.lexeme == "=#");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Greater);
    CHECK(token.lexeme == ">");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::GreaterEqual);
    CHECK(token.lexeme == ">=");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::MinusMinus);
    CHECK(token.lexeme == "--");
    token = scanner.next().value();
    CHECK(token.category == Category::PlusPlus);
    CHECK(token.lexeme == "++");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Less);
    CHECK(token.lexeme == "<");
    token = scanner.next().value();
    CHECK(token.category == Category::Less);
    CHECK(token.lexeme == "<");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::LessEqual);
    CHECK(token.lexeme == "<=");
    token = scanner.next().value();
    CHECK(token.category == Category::Greater);
    CHECK(token.lexeme == ">");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Plus);
    CHECK(token.lexeme == "+");
    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    token = scanner.next().value();
    CHECK(token.category == Category::Star);
    CHECK(token.lexeme == "*");
    token = scanner.next().value();
    CHECK(token.category == Category::Slash);
    CHECK(token.lexeme == "/");
    token = scanner.next().value();
    CHECK(token.category == Category::PlusAt);
    CHECK(token.lexeme == "+@");
    token = scanner.next().value();
    CHECK(token.category == Category::MinusAt);
    CHECK(token.lexeme == "-@");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1");
    token = scanner.next().value();
    CHECK(token.category == Category::MinusMinus);
    CHECK(token.lexeme == "--");
    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1");
    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-1");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Plus);
    CHECK(token.lexeme == "+");
    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "1");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-.");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-.1");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "-1.0");
    CHECK(scanner.next()->category == Category::EndOfLine);

    token = scanner.next().value();
    CHECK(token.category == Category::Plus);
    CHECK(token.lexeme == "+");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "@");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Minus);
    CHECK(token.lexeme == "-");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "@");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Equal);
    CHECK(token.lexeme == "=");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Word);
    CHECK(token.lexeme == "#");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Plus);
    CHECK(token.lexeme == "+");
    CHECK(scanner.next()->category == Category::Whitespace);

    token = scanner.next().value();
    CHECK(token.category == Category::Equal);
    CHECK(token.lexeme == "=");
    CHECK(scanner.next()->category == Category::EndOfLine);

    CHECK(scanner.eof());
}

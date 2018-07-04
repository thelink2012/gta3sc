#include "doctest.h"
#include <gta3sc/preprocessor.hpp>
#include <cstring>

namespace
{
template<std::size_t N>
auto make_source(const char (&data)[N]) -> gta3sc::SourceFile
{
    auto ptr = std::make_unique<char[]>(N);
    std::memcpy(ptr.get(), data, N);
    return gta3sc::SourceFile(std::move(ptr), N-1);
}

auto drain(gta3sc::Preprocessor& pp) -> std::string
{
    std::string res;
    while(!pp.eof())
        res.push_back(pp.next());
    REQUIRE(res.back() == '\0');
    res.pop_back();
    return res;
}
}

TEST_CASE("simple character stream")
{
    auto source = make_source("foo");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo");
}

TEST_CASE("character stream EOF")
{
    auto source = make_source("foo");
    auto pp = gta3sc::Preprocessor(source);
    while(!pp.eof()) pp.next();
    REQUIRE(pp.eof() == true);
    CHECK(pp.next() == '\0');
    CHECK(pp.next() == '\0');
    CHECK(pp.next() == '\0');
    CHECK(pp.eof() == true);
}

TEST_CASE("character stream with CR")
{
    auto source = make_source("foo\r\nbar\rbaz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo\nbar\nbaz");
}

TEST_CASE("character stream with whitespaces")
{
    auto source = make_source("foo   (bar) ,\t\t baz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo   (bar) ,\t\t baz");
}

TEST_CASE("character stream with leading whitespaces")
{
    auto source = make_source("   (,)    \t\tfoo\n  ,\t)  bar\n\t\tbaz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo\nbar\nbaz");
}

TEST_CASE("character stream with trailing whitespaces")
{
    auto source = make_source("foo,\nbar  \t, \nbaz  ()");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo,\nbar  \t, \nbaz  ()");
}

TEST_CASE("character stream with line comment")
{
    auto source = make_source("foo // line comment\nbar\n  // more comment\nbaz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo \nbar\n\nbaz");
}

TEST_CASE("character stream with leading block comment")
{
    auto source = make_source("  /* block */ () /* more */ foo\nbar\n /**/, baz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo\nbar\nbaz");
}

TEST_CASE("character stream with trailing block comment")
{
    auto source = make_source("foo /* block */\nbar\nbaz/* block */");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo  \nbar\nbaz ");
}

TEST_CASE("character stream with block comment crossing lines")
{
    auto source = make_source("foo /* block \n   comment \n */ \nbar\nbaz");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo \n\n\nbar\nbaz");
}

TEST_CASE("character stream with nested block comment")
{
    auto source = make_source("foo/* this /* is a block \n /* nesting */\n */ */bar");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo\n\nbar");
    CHECK(pp.inside_block_comment() == false);
}

TEST_CASE("character stream with unclosed block comment")
{
    auto source = make_source("foo/*/ this is a block \n comment \n ");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == "foo\n\n");
    CHECK(pp.inside_block_comment() == true);
}

TEST_CASE("complicated character stream")
{
    auto source = make_source(R"__(   ,/**/ /**/) first line
/* second line has */ letter (/* and */) 74
third line    /* has
fourth line /* being
fifth line /**/
*/ */ , sixth line  ()
  final   (line   )
  /* lies */
)__");
    auto expected = std::string(R"__(first line
letter ( ) 74
third line    


sixth line  ()
final   (line   )

)__");
    auto pp = gta3sc::Preprocessor(source);
    CHECK(drain(pp) == expected);
}

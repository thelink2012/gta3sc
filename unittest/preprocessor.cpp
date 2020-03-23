#include <doctest.h>
#include "compiler-fixture.hpp"
#include <gta3sc/preprocessor.hpp>
using namespace gta3sc::test;

namespace gta3sc::test
{
class PreprocessorFixture : public CompilerFixture
{
public:
    PreprocessorFixture() :
        pp(make_source(""), diagman)
    {}

protected:
    void build_pp(std::string_view src)
    {
        this->pp = gta3sc::Preprocessor(make_source(src), diagman);
    }

    auto drain() -> std::string
    {
        std::string res;
        while(!pp.eof())
            res.push_back(pp.next());
        CHECK(res.back() == '\0');
        res.pop_back();
        return res;
    }

protected:
    gta3sc::Preprocessor pp;
};
}

TEST_CASE_FIXTURE(PreprocessorFixture, "simple character stream")
{
    build_pp("foo");
    REQUIRE(drain() == "foo");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream EOF")
{
    build_pp("foo");
    while(!pp.eof()) pp.next();
    CHECK(pp.eof() == true);
    REQUIRE(pp.next() == '\0');
    REQUIRE(pp.next() == '\0');
    REQUIRE(pp.next() == '\0');
    REQUIRE(pp.eof() == true);
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with CR")
{
    build_pp("foo\r\nbar\rbaz");
    REQUIRE(drain() == "foo\nbar\nbaz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with whitespaces")
{
    build_pp("foo   (bar) ,\t\t baz");
    REQUIRE(drain() == "foo   (bar) ,\t\t baz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with leading whitespaces")
{
    build_pp("   (,)    \t\tfoo\n  ,\t)  bar\n\t\tbaz");
    REQUIRE(drain() == "foo\nbar\nbaz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with trailing whitespaces")
{
    build_pp("foo,\nbar  \t, \nbaz  ()");
    REQUIRE(drain() == "foo,\nbar  \t, \nbaz  ()");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with line comment")
{
    build_pp("foo // line comment\nbar\n  // more comment\nbaz");
    REQUIRE(drain() == "foo \nbar\n\nbaz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with leading block comment")
{
    build_pp("  /* block */ () /* more */ foo\nbar\n /**/, baz");
    REQUIRE(drain() == "foo\nbar\nbaz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with trailing block comment")
{
    build_pp("foo /* block */\nbar\nbaz/* block */");
    REQUIRE(drain() == "foo  \nbar\nbaz ");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with block comment crossing lines")
{
    build_pp("foo /* block \n   comment \n */ \nbar\nbaz");
    REQUIRE(drain() == "foo \n\n\nbar\nbaz");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with nested block comment")
{
    build_pp("foo/* this /* is a block \n /* nesting */\n */ */bar");
    REQUIRE(drain() == "foo\n\nbar");
}

TEST_CASE_FIXTURE(PreprocessorFixture, "character stream with unclosed block comment")
{
    build_pp("foo/*/ this is a block \n comment \n ");
    REQUIRE(drain() == "foo\n\n");
    REQUIRE(consume_diag().message == gta3sc::Diag::unterminated_comment);
}

TEST_CASE_FIXTURE(PreprocessorFixture, "complicated character stream")
{
    build_pp(R"__(   ,/**/ /**/) first line
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
    REQUIRE(drain() == expected);
}

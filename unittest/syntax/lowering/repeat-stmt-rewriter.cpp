#include "../syntax-fixture.hpp"
#include <doctest.h>
#include <gta3sc/syntax/lowering/repeat-stmt-rewriter.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <string>
using namespace gta3sc::test::syntax;

namespace gta3sc::test::syntax
{
class RepeatStmtRewriterFixture : public SyntaxFixture
{
public:
    RepeatStmtRewriterFixture() : namegen("TEST_LABEL_") {}

protected:
    auto make_rewriter() -> gta3sc::syntax::RepeatStmtRewriter
    {
        return gta3sc::syntax::RepeatStmtRewriter(namegen, arena);
    }

protected:
    gta3sc::ArenaMemoryResource arena;
    gta3sc::util::NameGenerator namegen;
};
}

TEST_CASE_FIXTURE(RepeatStmtRewriterFixture, "no statement to rewrite")
{
    auto rewriter = make_rewriter();
    const auto rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                        .command("WAIT")
                                                        .arg_int(0)
                                                        .build());
    REQUIRE(!rewrite_result);
}

TEST_CASE_FIXTURE(RepeatStmtRewriterFixture, "rewriting REPEAT")
{
    auto rewriter = make_rewriter();

    SUBCASE("successfully rewrites REPEAT")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(arena)
                         .command("REPEAT")
                         .arg_int(10)
                         .arg_ident("somevar")
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("SET")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(0)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .label("TEST_LABEL_0")
                                 .build()}));
    }

    SUBCASE("does not rewrite REPEAT with too few args")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(arena)
                         .command("REPEAT")
                         .arg_int(10)
                         .build());
        REQUIRE(!rewrite_result);
    }

    SUBCASE("does not rewrite REPEAT with too many args")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(arena)
                         .command("REPEAT")
                         .arg_int(10)
                         .arg_ident("somevar")
                         .arg_ident("otherarg")
                         .build());
        REQUIRE(!rewrite_result);
    }

    SUBCASE("rewrite preserves label on REPEAT's line")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(arena)
                         .label("mylabel")
                         .command("REPEAT")
                         .arg_int(10)
                         .arg_ident("somevar")
                         .build());
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .label("MYLABEL")
                                 .command("SET")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(0)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .label("TEST_LABEL_0")
                                 .build()}));
    }

    SUBCASE("rewrite not affected by type of iter_var")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(arena)
                         .command("REPEAT")
                         .arg_int(10)
                         .arg_int(20)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("SET")
                                 .arg_int(20)
                                 .arg_int(0)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .label("TEST_LABEL_0")
                                 .build()}));
    }

    SUBCASE("REPEAT label names are unique")
    {
        gta3sc::syntax::RepeatStmtRewriter::Result rewrite_result;

        const auto repeat_10_somevar = gta3sc::ParserIR::Builder(arena)
                                               .command("REPEAT")
                                               .arg_int(10)
                                               .arg_ident("somevar")
                                               .build();

        auto set_somevar_0 = gta3sc::ParserIR::Builder(arena)
                                           .command("SET")
                                           .arg_ident("SOMEVAR")
                                           .arg_int(0)
                                           .build();

        rewrite_result = rewriter.visit(*repeat_10_somevar);
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {set_somevar_0, gta3sc::ParserIR::Builder(arena)
                                                .label("TEST_LABEL_0")
                                                .build()}));
        gta3sc::util::unlink_node(*set_somevar_0);

        rewrite_result = rewriter.visit(*repeat_10_somevar);
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {set_somevar_0, gta3sc::ParserIR::Builder(arena)
                                                .label("TEST_LABEL_1")
                                                .build()}));
    }
}

TEST_CASE_FIXTURE(RepeatStmtRewriterFixture, "rewriting ENDREPEAT")
{
    auto rewriter = make_rewriter();
    gta3sc::syntax::RepeatStmtRewriter::Result rewrite_result;

    SUBCASE("successfully rewrites ENDREPEAT")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(10)
                                                 .arg_ident("somevar")
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(10)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_0")
                                 .build()
                        }));
    }

    SUBCASE("does not rewrite ENDREPEAT if no REPEAT seen")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(!rewrite_result);
    }

    SUBCASE("does not rewrite ENDREPEAT if ENDREPEAT already seen")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(10)
                                                 .arg_ident("somevar")
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(!rewrite_result);
    }

    SUBCASE("rewrite preserves label on ENDREPEAT's line")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(10)
                                                 .arg_ident("somevar")
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .label("mylabel")
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .label("MYLABEL")
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(10)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_0")
                                 .build()
                        }));
    }

    SUBCASE("rewrite not affected by type of iter_var")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(10)
                                                 .arg_int(20)
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_int(20)
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_int(20)
                                 .arg_int(10)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_0")
                                 .build()
                        }));
    }

    SUBCASE("rewrite not affected by type of num_times")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_string("some string")
                                                 .arg_ident("somevar")
                                                 .build());
        CHECK(rewrite_result);
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR")
                                 .arg_string("some string")
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_0")
                                 .build()
                        }));
    }

    SUBCASE("successful nesting")
    {
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(10)
                                                 .arg_ident("somevar1")
                                                 .build());
        REQUIRE(rewrite_result);

        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(20)
                                                 .arg_ident("somevar2")
                                                 .build());
        REQUIRE(rewrite_result);

        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("REPEAT")
                                                 .arg_int(30)
                                                 .arg_ident("somevar3")
                                                 .build());
        REQUIRE(rewrite_result);

        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR3")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR3")
                                 .arg_int(30)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_2")
                                 .build()
                        }));
        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR2")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR2")
                                 .arg_int(20)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_1")
                                 .build()
                        }));

        rewrite_result = rewriter.visit(*gta3sc::ParserIR::Builder(arena)
                                                 .command("ENDREPEAT")
                                                 .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result.ir
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(arena)
                                 .command("ADD_THING_TO_THING")
                                 .arg_ident("SOMEVAR1")
                                 .arg_int(1)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("IS_THING_GREATER_OR_EQUAL_TO_THING")
                                 .arg_ident("SOMEVAR1")
                                 .arg_int(10)
                                 .build(),
                         gta3sc::ParserIR::Builder(arena)
                                 .command("GOTO_IF_FALSE")
                                 .arg_ident("TEST_LABEL_0")
                                 .build()
                        }));
    }
}

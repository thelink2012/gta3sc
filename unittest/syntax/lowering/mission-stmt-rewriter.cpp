#include "../syntax-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/syntax/lowering/mission-stmt-rewriter.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <string>
using namespace gta3sc::test::syntax;

namespace gta3sc::test::syntax
{
class MissionStmtRewriterFixture : public SyntaxFixture
{
public:
    MissionStmtRewriterFixture() = default;

protected:
    auto make_rewriter() -> gta3sc::syntax::MissionStmtRewriter
    {
        return gta3sc::syntax::MissionStmtRewriter(&arena);
    }

protected:
    gta3sc::ArenaMemoryResource arena;
};
} // namespace gta3sc::test::syntax

TEST_CASE_FIXTURE(MissionStmtRewriterFixture, "no statement to rewrite")
{
    auto rewriter = make_rewriter();
    const auto rewrite_result = rewriter.visit(
            *gta3sc::ParserIR::Builder(&arena)
                     .command("WAIT")
                     .arg_int(0)
                     .build());
    REQUIRE(!rewrite_result);
}

TEST_CASE_FIXTURE(MissionStmtRewriterFixture, "rewriting MISSION_START")
{
    auto rewriter = make_rewriter();

    SUBCASE("instruction is fully removed when there is no label")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(&arena)
                         .command("MISSION_START")
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->empty());
    }

    SUBCASE("label preserved on label-only instruction")
    {
        auto* const line = gta3sc::ParserIR::Builder(&arena)
                                   .label("mylabel")
                                   .command("MISSION_START")
                                   .build();

        const auto rewrite_result = rewriter.visit(*line);

        REQUIRE(rewrite_result);
        REQUIRE(*rewrite_result
                == gta3sc::LinkedIR<gta3sc::ParserIR>({gta3sc::ParserIR::create(
                        line->label_or_null(), nullptr, &arena)}));
    }
}

TEST_CASE_FIXTURE(MissionStmtRewriterFixture, "rewriting MISSION_END")
{
    auto rewriter = make_rewriter();

    SUBCASE("successfully rewrites MISSION_END")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(&arena)
                         .command("MISSION_END")
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(*rewrite_result
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(&arena)
                                 .command("TERMINATE_THIS_SCRIPT")
                                 .build()}));
    }

    SUBCASE("rewrite preserves label on MISSION_END's line")
    {
        const auto rewrite_result = rewriter.visit(
                *gta3sc::ParserIR::Builder(&arena)
                         .label("mylabel")
                         .command("MISSION_END")
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(*rewrite_result
                == gta3sc::LinkedIR<gta3sc::ParserIR>(
                        {gta3sc::ParserIR::Builder(&arena)
                                 .label("MYLABEL")
                                 .command("TERMINATE_THIS_SCRIPT")
                                 .build()}));
    }
}

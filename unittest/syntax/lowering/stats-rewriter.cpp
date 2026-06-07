#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/lowering/stats-rewriter.hpp>
#include <string_view>
using namespace gta3sc::test;
using namespace std::literals::string_view_literals;

namespace gta3sc::test
{
class StatsRewriterFixture : public CommandTableFixture
{
protected:
    ArenaMemoryResource arena{};
    SymbolTable symrepo{&arena};
};
} // namespace gta3sc::test

using gta3sc::SemaIR;
using gta3sc::syntax::StatsRewriter;
using gta3sc::test::StatsRewriterFixture;

TEST_CASE_FIXTURE(StatsRewriterFixture, "StatsRewriter")
{
    const auto total_count = 9;
    const auto total_cmd = cmdman.find_command("SET_COLLECTABLE1_TOTAL");
    REQUIRE(total_cmd != nullptr);

    SUBCASE("constructor from string rewrites total line")
    {
        auto rewriter = StatsRewriter(cmdman, "SET_COLLECTABLE1_TOTAL"sv,
                                      total_count, &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int()
                == total_count);
    }

    SUBCASE("constructor from CommandDef rewrites total line")
    {
        auto rewriter = StatsRewriter(total_cmd, total_count, &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int()
                == total_count);
    }

    SUBCASE("null command never rewrites")
    {
        auto rewriter = StatsRewriter(nullptr, total_count, &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("unknown command name never rewrites")
    {
        auto rewriter = StatsRewriter(cmdman, "NO_SUCH_CMD"sv, total_count,
                                      &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("does not rewrite unrelated command")
    {
        const auto wait_cmd = cmdman.find_command("WAIT");
        REQUIRE(wait_cmd != nullptr);
        auto rewriter = StatsRewriter(total_cmd, total_count, &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*wait_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("does not rewrite line with no arguments")
    {
        auto rewriter = StatsRewriter(total_cmd, 5, &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("non-zero first int does not rewrite")
    {
        auto rewriter = StatsRewriter(total_cmd, total_count, &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(7, SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("non-INT first arg does not rewrite")
    {
        auto rewriter = StatsRewriter(total_cmd, 5, &arena);
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_text_label("FOO", SemaIR::Builder::no_range)
                         .build()));
    }

    SUBCASE("label on total line preserved")
    {
        const auto label = symrepo.insert_label(
                                          "MYLABEL",
                                          gta3sc::SymbolTable::global_scope,
                                          SemaIR::Builder::no_range)
                                   .first;
        REQUIRE(label != nullptr);

        auto rewriter = StatsRewriter(total_cmd, total_count, &arena);

        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .label(label)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);

        const auto& rewritten = rewrite_result->front();
        REQUIRE(rewritten.has_label());
        REQUIRE(rewritten.label().name() == "MYLABEL");
    }

    SUBCASE("trailing args after first int are preserved")
    {
        auto rewriter = StatsRewriter(total_cmd, total_count, &arena);

        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .arg_int(99, SemaIR::Builder::no_range)
                         .arg_int(100, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);

        const auto& cmd = rewrite_result->front().command();
        REQUIRE(cmd.num_args() == 3);
        REQUIRE(cmd.arg(0).as_int() == total_count);
        REQUIRE(cmd.arg(1).as_int() == 99);
        REQUIRE(cmd.arg(2).as_int() == 100);
    }
}

TEST_CASE_FIXTURE(StatsRewriterFixture, "rewriting SET_COLLECTABLE1_TOTAL")
{
    const auto total_cmd = cmdman.find_command("SET_COLLECTABLE1_TOTAL");
    REQUIRE(total_cmd != nullptr);

    SUBCASE("initializer replaced with total from symbol table")
    {
        symrepo.add_collectable1(3);
        auto rewriter = StatsRewriter::for_collectable1_total(cmdman, symrepo,
                                                              &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int() == 3);
    }
}

TEST_CASE_FIXTURE(StatsRewriterFixture, "rewriting SET_PROGRESS_TOTAL")
{
    const auto total_cmd = cmdman.find_command("SET_PROGRESS_TOTAL");
    REQUIRE(total_cmd != nullptr);

    SUBCASE("initializer replaced with progress total")
    {
        symrepo.add_progress(8);
        auto rewriter = StatsRewriter::for_progress_total(cmdman, symrepo,
                                                          &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int() == 8);
    }
}

TEST_CASE_FIXTURE(StatsRewriterFixture,
                  "rewriting SET_TOTAL_NUMBER_OF_MISSIONS")
{
    const auto total_cmd = cmdman.find_command("SET_TOTAL_NUMBER_OF_MISSIONS");
    REQUIRE(total_cmd != nullptr);

    SUBCASE("initializer replaced with mission total")
    {
        symrepo.add_mission(2);
        auto rewriter = StatsRewriter::for_mission_total(cmdman, symrepo,
                                                         &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int() == 2);
    }
}

TEST_CASE_FIXTURE(StatsRewriterFixture, "rewriting SET_MISSION_RESPECT_TOTAL")
{
    const auto total_cmd = cmdman.find_command("SET_MISSION_RESPECT_TOTAL");
    REQUIRE(total_cmd != nullptr);

    SUBCASE("initializer replaced with respect total")
    {
        symrepo.add_mission_respect(8);
        auto rewriter = StatsRewriter::for_mission_respect_total(
                cmdman, symrepo, &arena);
        const auto rewrite_result = rewriter.visit(
                *SemaIR::Builder(&arena)
                         .command(*total_cmd, SemaIR::Builder::no_range)
                         .arg_int(0, SemaIR::Builder::no_range)
                         .build());
        REQUIRE(rewrite_result);
        REQUIRE(rewrite_result->front().command().arg(0).as_int() == 8);
    }
}

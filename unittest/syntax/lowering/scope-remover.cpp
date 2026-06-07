#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/syntax/lowering/scope-remover.hpp>

TEST_CASE_FIXTURE(gta3sc::test::CommandTableFixture, "scope braces are removed")
{
    gta3sc::ArenaMemoryResource arena;
    gta3sc::syntax::ScopeRemover rewriter(cmdman, &arena);

    SUBCASE("open brace")
    {
        const auto cmd = cmdman.find_command("{");
        REQUIRE(cmd != nullptr);

        const auto result = rewriter.visit(
                *gta3sc::SemaIR::Builder(&arena).command(*cmd).build());

        REQUIRE(result);
        REQUIRE(result->empty());
    }

    SUBCASE("close brace")
    {
        const auto cmd = cmdman.find_command("}");
        REQUIRE(cmd != nullptr);

        const auto result = rewriter.visit(
                *gta3sc::SemaIR::Builder(&arena).command(*cmd).build());

        REQUIRE(result);
        REQUIRE(result->empty());
    }
}

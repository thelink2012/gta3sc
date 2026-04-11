#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/syntax/lowering/load-and-launch-mission-rewriter.hpp>

class LoadAndLaunchMissionRewriterFixture
    : public gta3sc::test::CommandTableFixture
{
public:
    auto make_rewriter() -> gta3sc::syntax::LoadAndLaunchMissionRewriter
    {
        return gta3sc::syntax::LoadAndLaunchMissionRewriter(cmdman, &arena);
    }

    auto
    make_mission_file(std::string_view name) -> const gta3sc::SymbolTable::File&
    {
        const auto [file, inserted] = symtable.insert_file(
                name, gta3sc::SymbolTable::FileType::mission,
                gta3sc::SourceManager::no_source_range);
        REQUIRE(inserted);
        return *file;
    }

protected:
    gta3sc::ArenaMemoryResource arena;
    gta3sc::SymbolTable symtable{&arena};
};

TEST_CASE_FIXTURE(LoadAndLaunchMissionRewriterFixture,
                  "no statement to rewrite")
{
    const auto wait_cmd = cmdman.find_command("WAIT");
    REQUIRE(wait_cmd != nullptr);

    auto rewriter = make_rewriter();
    const auto rewrite_result = rewriter.visit(*gta3sc::SemaIR::Builder(&arena)
                                                        .command(*wait_cmd)
                                                        .arg_int(0)
                                                        .build());

    REQUIRE(!rewrite_result);
}

TEST_CASE_FIXTURE(LoadAndLaunchMissionRewriterFixture,
                  "rewriting LOAD_AND_LAUNCH_MISSION")
{
    const auto load_cmd = cmdman.find_command("LOAD_AND_LAUNCH_MISSION");
    const auto load_internal_cmd = cmdman.find_command(
            "LOAD_AND_LAUNCH_MISSION_INTERNAL");
    REQUIRE(load_cmd != nullptr);
    REQUIRE(load_internal_cmd != nullptr);

    auto rewriter = make_rewriter();

    SUBCASE("successfully rewrites LOAD_AND_LAUNCH_MISSION")
    {
        const auto& mission_file = make_mission_file("mission1.sc");
        const auto rewrite_result = rewriter.visit(
                *gta3sc::SemaIR::Builder(&arena)
                         .command(*load_cmd)
                         .arg_filename(mission_file)
                         .build());

        REQUIRE(rewrite_result);
        REQUIRE(*rewrite_result
                == gta3sc::LinkedIR<gta3sc::SemaIR>(
                        {gta3sc::SemaIR::Builder(&arena)
                                 .command(*load_internal_cmd)
                                 .arg_int(0)
                                 .build()}));
    }

    SUBCASE("mission_id follows insertion order")
    {
        const auto& m0 = make_mission_file("a.sc");
        const auto& m1 = make_mission_file("b.sc");
        const auto& m2 = make_mission_file("c.sc");

        const auto rewrite_m0 = rewriter.visit(*gta3sc::SemaIR::Builder(&arena)
                                                        .command(*load_cmd)
                                                        .arg_filename(m0)
                                                        .build());
        REQUIRE(rewrite_m0);
        REQUIRE(*rewrite_m0
                == gta3sc::LinkedIR<gta3sc::SemaIR>(
                        {gta3sc::SemaIR::Builder(&arena)
                                 .command(*load_internal_cmd)
                                 .arg_int(0)
                                 .build()}));

        const auto rewrite_m1 = rewriter.visit(*gta3sc::SemaIR::Builder(&arena)
                                                        .command(*load_cmd)
                                                        .arg_filename(m1)
                                                        .build());
        REQUIRE(rewrite_m1);
        REQUIRE(*rewrite_m1
                == gta3sc::LinkedIR<gta3sc::SemaIR>(
                        {gta3sc::SemaIR::Builder(&arena)
                                 .command(*load_internal_cmd)
                                 .arg_int(1)
                                 .build()}));

        const auto rewrite_m2 = rewriter.visit(*gta3sc::SemaIR::Builder(&arena)
                                                        .command(*load_cmd)
                                                        .arg_filename(m2)
                                                        .build());
        REQUIRE(rewrite_m2);
        REQUIRE(*rewrite_m2
                == gta3sc::LinkedIR<gta3sc::SemaIR>(
                        {gta3sc::SemaIR::Builder(&arena)
                                 .command(*load_internal_cmd)
                                 .arg_int(2)
                                 .build()}));
    }

    SUBCASE("rewrite preserves label on LOAD_AND_LAUNCH_MISSION's line")
    {
        const auto& mission_file = make_mission_file("mission1.sc");
        const auto [label, label_inserted] = symtable.insert_label(
                "MYLABEL", gta3sc::SymbolTable::global_scope,
                gta3sc::SourceManager::no_source_range);
        REQUIRE(label_inserted);

        const auto rewrite_result = rewriter.visit(
                *gta3sc::SemaIR::Builder(&arena)
                         .label(label)
                         .command(*load_cmd)
                         .arg_filename(mission_file)
                         .build());

        REQUIRE(rewrite_result);
        REQUIRE(*rewrite_result
                == gta3sc::LinkedIR<gta3sc::SemaIR>(
                        {gta3sc::SemaIR::Builder(&arena)
                                 .label(label)
                                 .command(*load_internal_cmd)
                                 .arg_int(0)
                                 .build()}));
    }
}

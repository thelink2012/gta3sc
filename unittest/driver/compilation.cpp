#include "../command-manager-fixture.hpp"
#include "../with-diagnostic-fixture.hpp"
#include "../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <functional>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/driver/compilation.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/syntax/sema.hpp>
#include <gta3sc/util/arena.hpp>
#include <vector>

using gta3sc::driver::Compilation;
using gta3sc::test::CommandTableFixture;
using gta3sc::test::WithDiagnosticFixture;
using gta3sc::test::WithTempDirFixture;

namespace
{
class CompilationFixture
    : public WithTempDirFixture
    , public WithDiagnosticFixture
    , public CommandTableFixture
{
protected:
    auto compile_main(std::vector<std::byte>& output) -> bool
    {
        REQUIRE(sourceman.scan_directory(root_test_dir));
        return Compilation(root_test_dir / "main.sc", cmdman, modelman,
                           sourceman, diagman)
                .compile({&output});
    }

    auto compile_main() -> bool
    {
        std::vector<std::byte> _;
        return compile_main(_);
    }

    auto compile_script(std::string_view main_content) -> bool
    {
        create_test_file("main.sc", main_content);
        return compile_main();
    }

    auto compile_script(std::string_view main_content,
                        std::vector<std::byte>& output) -> bool
    {
        create_test_file("main.sc", main_content);
        return compile_main(output);
    }

private:
    gta3sc::ArenaMemoryResource model_arena;
    gta3sc::ModelTable modelman{
            gta3sc::ModelTable::Builder(&model_arena).build()};
    gta3sc::SourceManager sourceman;
};

TEST_CASE_FIXTURE(CompilationFixture, "compile - empty main file succeeds")
{
    CHECK(compile_script(""));
}

TEST_CASE_FIXTURE(CompilationFixture, "compile - success writes output bytes")
{
    std::vector<std::byte> output;
    REQUIRE(compile_script("WAIT 0\n", output));
    CHECK_FALSE(output.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - multifile all file types succeeds")
{
    create_test_file("main.sc", "WAIT 0\n"
                                "GOSUB_FILE ext_label ext.sc\n"
                                "LAUNCH_MISSION sub.sc\n"
                                "LOAD_AND_LAUNCH_MISSION miss.sc\n");
    create_test_file("ext.sc", "ext_label:\nWAIT 0\n");
    create_test_file("sub.sc", "MISSION_START\nMISSION_END\n");
    create_test_file("miss.sc", "MISSION_START\nMISSION_END\n");

    std::vector<std::byte> output;
    REQUIRE(compile_main(output));
    CHECK_FALSE(output.empty());
}

TEST_CASE_FIXTURE(CompilationFixture, "compile - parse phase failure")
{
    std::vector<std::byte> output;
    CHECK_FALSE(compile_main(output));
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);
}

TEST_CASE_FIXTURE(CompilationFixture, "compile - sema phase failure")
{
    CHECK_FALSE(compile_script("UNDEFINED_CMD 1\n"));

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::syntax::diag::undefined_command);
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - pre-sema lowering - REPEAT stmt lowered")
{
    CHECK(compile_script("VAR_INT x\n"
                         "REPEAT 3 x\n"
                         "    WAIT 0\n"
                         "ENDREPEAT\n"));
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - post-sema lowering - IF stmt lowered")
{
    CHECK(compile_script("IF NOT WAIT 0\n"
                         "    WAIT 0\n"
                         "ENDIF\n"));
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - post-sema lowering - WHILE stmt lowered")
{
    CHECK(compile_script("WHILE NOT WAIT 0\n"
                         "    WAIT 0\n"
                         "ENDWHILE\n"));
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - post-sema lowering - scope blocks compile")
{
    CHECK(compile_script("{\n"
                         "    WAIT 0\n"
                         "}\n"));
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - post-sema lowering - var decls compile")
{
    CHECK(compile_script("VAR_INT x\n"
                         "WAIT 0\n"));
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - post-sema lowering - stats args rewritten")
{
    // Each subcase verifies that a StatsRewriter actually fires by compiling
    // two structurally identical scripts that differ only in the stats
    // initializer argument: one uses the placeholder 0 (which the rewriter
    // must replace with the real count) and one hardcodes the expected count
    // directly (no rewrite needed).  Equal output bytes prove the rewrite
    // substituted the correct value.

    SUBCASE("collectable1_total")
    {
        std::vector<std::byte> via_placeholder, via_direct;
        REQUIRE(compile_script("CREATE_COLLECTABLE1 1.0 2.0 3.0\n"
                               "SET_COLLECTABLE1_TOTAL 0\n"
                               "WAIT 0\n",
                               via_placeholder));
        REQUIRE(compile_script("CREATE_COLLECTABLE1 1.0 2.0 3.0\n"
                               "SET_COLLECTABLE1_TOTAL 1\n"
                               "WAIT 0\n",
                               via_direct));
        CHECK(via_placeholder == via_direct);
    }

    SUBCASE("progress_total")
    {
        std::vector<std::byte> via_placeholder, via_direct;
        REQUIRE(compile_script("PLAYER_MADE_PROGRESS 5\n"
                               "SET_PROGRESS_TOTAL 0\n"
                               "WAIT 0\n",
                               via_placeholder));
        REQUIRE(compile_script("PLAYER_MADE_PROGRESS 5\n"
                               "SET_PROGRESS_TOTAL 5\n"
                               "WAIT 0\n",
                               via_direct));
        CHECK(via_placeholder == via_direct);
    }

    SUBCASE("mission_total")
    {
        std::vector<std::byte> via_placeholder, via_direct;
        REQUIRE(compile_script("REGISTER_ODDJOB_MISSION_PASSED\n"
                               "SET_TOTAL_NUMBER_OF_MISSIONS 0\n"
                               "WAIT 0\n",
                               via_placeholder));
        REQUIRE(compile_script("REGISTER_ODDJOB_MISSION_PASSED\n"
                               "SET_TOTAL_NUMBER_OF_MISSIONS 1\n"
                               "WAIT 0\n",
                               via_direct));
        CHECK(via_placeholder == via_direct);
    }

    SUBCASE("mission_respect_total")
    {
        std::vector<std::byte> via_placeholder, via_direct;
        REQUIRE(compile_script("AWARD_PLAYER_MISSION_RESPECT 7\n"
                               "SET_MISSION_RESPECT_TOTAL 0\n"
                               "WAIT 0\n",
                               via_placeholder));
        REQUIRE(compile_script("AWARD_PLAYER_MISSION_RESPECT 7\n"
                               "SET_MISSION_RESPECT_TOTAL 7\n"
                               "WAIT 0\n",
                               via_direct));
        CHECK(via_placeholder == via_direct);
    }
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - codegen phase failure - local storage overflow")
{
    CHECK_FALSE(compile_script("{\n"
                               "LVAR_TEXT_LABEL a\n"
                               "LVAR_TEXT_LABEL b\n"
                               "LVAR_TEXT_LABEL c\n"
                               "LVAR_TEXT_LABEL d\n"
                               "LVAR_TEXT_LABEL e\n"
                               "LVAR_TEXT_LABEL f\n"
                               "LVAR_TEXT_LABEL g\n"
                               "LVAR_TEXT_LABEL h\n"
                               "LVAR_TEXT_LABEL i\n"
                               "}\n"));

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::codegen::diag::storage_overflow);
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - codegen phase failure - bytecode failure")
{
    const auto result = compile_script("COMMAND_WITHOUT_ID 1\n");

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::codegen::diag::target_does_not_support_command);

    CHECK_FALSE(result);
}

TEST_CASE_FIXTURE(CompilationFixture,
                  "compile - codegen phase failure - relocation failure")
{
    create_test_file("main.sc",
                     "GOTO mission_label\nLOAD_AND_LAUNCH_MISSION miss.sc\n");
    create_test_file("miss.sc",
                     "MISSION_START\nmission_label:\nWAIT 0\nMISSION_END\n");

    std::vector<std::byte> output;
    CHECK_FALSE(compile_main(output));
    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::codegen::diag::label_ref_across_segments);
}

} // namespace

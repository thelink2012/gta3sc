
#include "config-fixture.hpp"
#include "gta3sc/util/arena.hpp"
#include <filesystem>
#include <gta3sc/sourceman.hpp>
using namespace std::string_view_literals;
using namespace gta3sc::test::config;

TEST_CASE_FIXTURE(LoadConfigFixture,
                  "load_config with filesystem path and builder overload")
{
    create_test_file(root_test_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="TEST_CMD">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    auto builder = gta3sc::load_config(
            root_test_dir, root_test_dir / "config.xml", sourceman, diagman,
            gta3sc::CommandTable::Builder(&arena));
    builder.insert_command("TEST_CMD2");
    auto table = std::move(builder).build();

    CHECK(diags.empty());

    auto* cmd = table.find_command("TEST_CMD"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 1);

    auto* cmd2 = table.find_command("TEST_CMD2"sv);
    REQUIRE(cmd2 != nullptr);
    CHECK(cmd2->num_params() == 0);
}

TEST_CASE_FIXTURE(LoadConfigFixture,
                  "load_config with filesystem path and allocator overload")
{
    create_test_file(root_test_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="TEST_CMD"/>
    </Commands>
</GTA3Script>)");

    auto table = gta3sc::load_config(root_test_dir,
                                     root_test_dir / "config.xml", sourceman,
                                     diagman, gta3sc::ArenaAllocator<>(&arena));

    CHECK(diags.empty());

    auto* cmd = table.find_command("TEST_CMD"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 0);
}

TEST_CASE_FIXTURE(ConfigFixture, "load_config with in-memory file")
{
    auto source = make_source(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="TEST_CMD"/>
    </Commands>
</GTA3Script>)");

    auto builder = gta3sc::load_config(source, diagman,
                                       gta3sc::CommandTable::Builder(&arena));
    builder.insert_command("TEST_CMD2");
    auto table = std::move(builder).build();

    CHECK(diags.empty());

    auto* cmd = table.find_command("TEST_CMD"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 0);
}

TEST_CASE_FIXTURE(LoadConfigFixture, "load_config with non-existent file")
{
    auto table = gta3sc::load_config(root_test_dir,
                                     root_test_dir / "nonexistent.xml",
                                     sourceman, diagman,
                                     gta3sc::CommandTable::Builder(&arena))
                         .build();

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_could_not_open_file);
}

TEST_CASE_FIXTURE(LoadConfigFixture,
                  "load_config in allocator overload and non-existent file")
{
    auto table = gta3sc::load_config(
            root_test_dir, root_test_dir / "nonexistent.xml", sourceman,
            diagman, gta3sc::ArenaAllocator<>(&arena));

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_could_not_open_file);
}
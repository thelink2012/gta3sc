#include "config-fixture.hpp"
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty commands section")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown node in commands")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <UnknownNode/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_unknown_node);
}

TEST_CASE_FIXTURE(
        gta3sc::test::config::ConfigFixture,
        "config parse command id then command preserves id and handled")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="FOO" ID="99" Handled="false"/>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());
    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 99);
    CHECK_FALSE(cmd->target_handled());
    CHECK(cmd->num_params() == 1);
}

TEST_CASE_FIXTURE(
        gta3sc::test::config::ConfigFixture,
        "config parse command then command id preserves/replaces handled")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
                <Param Type="INPUT_INT"/>
            </Params>
        </Command>
        <CommandId Name="FOO" ID="99" Handled="false"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());
    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 99);
    CHECK_FALSE(cmd->target_handled());
    CHECK(cmd->num_params() == 2);
}
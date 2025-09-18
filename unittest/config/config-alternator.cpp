#include "config-fixture.hpp"
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty alternators section")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown node in alternators")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <UnknownNode/>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown node in alternator")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <UnknownNode/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty alternator")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator missing name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternative missing name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with empty name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="">
        </Alternator>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternative with empty name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name=""/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with alternatives")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="CMD1"/>
            <Alternative Name="CMD2"/>
            <Alternative Name="CMD3"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    auto* cmd2 = table.find_command("CMD2"sv);
    auto* cmd3 = table.find_command("CMD3"sv);
    REQUIRE(cmd1 != nullptr);
    REQUIRE(cmd2 != nullptr);
    REQUIRE(cmd3 != nullptr);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd2);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd3);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config extend alternator definitions")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="CMD1"/>
            <Alternative Name="CMD2"/>
        </Alternator>
        <Alternator Name="MYALT">
            <Alternative Name="CMD3"/>
            <Alternative Name="CMD4"/>
        </Alternator>
        <Alternator Name="myalt">
            <Alternative Name="CMD5"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    // Check that all commands were created
    auto* cmd1 = table.find_command("CMD1"sv);
    auto* cmd2 = table.find_command("CMD2"sv);
    auto* cmd3 = table.find_command("CMD3"sv);
    auto* cmd4 = table.find_command("CMD4"sv);
    auto* cmd5 = table.find_command("CMD5"sv);
    REQUIRE(cmd1 != nullptr);
    REQUIRE(cmd2 != nullptr);
    REQUIRE(cmd3 != nullptr);
    REQUIRE(cmd4 != nullptr);
    REQUIRE(cmd5 != nullptr);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd2);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd3);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd4);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd5);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with command defined later")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="CMD1"/>
        </Alternator>
    </Alternators>
    <Commands>
        <Command Name="CMD1">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);
    CHECK(cmd1->params().size() == 1);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with command defined before")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="CMD1">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="CMD1"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);
    CHECK(cmd1->params().size() == 1);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with case insensitive command name")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="CMD1">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="cmd1"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);
    CHECK(cmd1->params().size() == 1);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with duplicate command")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="CMD1"/>
            <Alternative Name="CMD1"/>
            <Alternative Name="CMD2"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    auto* cmd2 = table.find_command("CMD2"sv);
    REQUIRE(cmd1 != nullptr);
    REQUIRE(cmd2 != nullptr);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd2);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse alternator with case insensitive command names")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="MyAlt">
            <Alternative Name="Cmd1"/>
            <Alternative Name="CMD1"/>
            <Alternative Name="cmd1"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* alt = table.find_alternator("MYALT"sv);
    REQUIRE(alt != nullptr);

    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);

    auto it = alt->begin();
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    REQUIRE(it != alt->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt->end());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command in multiple alternators")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Alternators>
        <Alternator Name="Alt1">
            <Alternative Name="CMD1"/>
        </Alternator>
        <Alternator Name="Alt2">
            <Alternative Name="CMD1"/>
        </Alternator>
    </Alternators>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);

    auto* alt1 = table.find_alternator("ALT1"sv);
    REQUIRE(alt1 != nullptr);
    auto* alt2 = table.find_alternator("ALT2"sv);
    REQUIRE(alt2 != nullptr);

    auto it = alt1->begin();
    REQUIRE(it != alt1->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt1->end());

    it = alt2->begin();
    REQUIRE(it != alt2->end());
    CHECK(&it->command() == cmd1);

    ++it;
    CHECK(it == alt2->end());
}
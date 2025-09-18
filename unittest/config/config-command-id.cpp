#include "config-fixture.hpp"
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id missing name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId ID="1"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id empty name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="" ID="1"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id empty id")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID=""/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with decimal id")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="42"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 42);
    CHECK(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with hex id")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="0x2A"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 42);
    CHECK(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with invalid id")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="not a number"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with handled false")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="42" Handled="false"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 42);
    CHECK_FALSE(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with invalid handled value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="42" Handled="notbool"/>
        <CommandId Name="CMD2" ID="43" Handled=""/>
    </Commands>
</GTA3Script>)");

    REQUIRE(diags.size() == 2);
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_expected_boolean);
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_expected_boolean);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id without id")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    CHECK_FALSE(cmd->target_id().has_value());
    CHECK_FALSE(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id case insensitive name")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="cmd1" ID="42"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 42);
    CHECK(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id redefinition")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="42"/>
        <CommandId Name="cmd1" ID="99"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 99);
    CHECK(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id without handled when no id")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" Handled="true"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_handled_without_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with various decimal formats")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="0000123"/>
        <CommandId Name="CMD2" ID="00000000000000000000005"/>
        <CommandId Name="CMD3" ID="32767"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd1 = table.find_command("CMD1"sv);
    auto* cmd2 = table.find_command("CMD2"sv);
    auto* cmd3 = table.find_command("CMD3"sv);
    REQUIRE(cmd1 != nullptr);
    REQUIRE(cmd2 != nullptr);
    REQUIRE(cmd3 != nullptr);
    CHECK(cmd1->target_id().value() == 123);
    CHECK(cmd2->target_id().value() == 5);
    CHECK(cmd3->target_id().value() == 32767);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with various hex formats")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="0x00000123"/>
        <CommandId Name="CMD2" ID="0X123"/>
        <CommandId Name="CMD3" ID="0xbcd"/>
        <CommandId Name="CMD4" ID="0xBCD"/>
        <CommandId Name="CMD5" ID="0xbcD"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

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
    CHECK(cmd1->target_id().value() == 0x123);
    CHECK(cmd2->target_id().value() == 0x123);
    CHECK(cmd3->target_id().value() == 0xBCD);
    CHECK(cmd4->target_id().value() == 0xBCD);
    CHECK(cmd5->target_id().value() == 0xBCD);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with decimal overflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="65536"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with hex overflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="0x10000"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with invalid formats")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID=""/>
        <CommandId Name="CMD2" ID="+1"/>
        <CommandId Name="CMD3" ID="-1"/>
        <CommandId Name="CMD4" ID="+0x1"/>
        <CommandId Name="CMD5" ID="-0x1"/>
        <CommandId Name="CMD6" ID="0x"/>
        <CommandId Name="CMD7" ID="0xGHIJ"/>
        <CommandId Name="CMD8" ID="0.5"/>
        <CommandId Name="CMD9" ID="1e5"/>
        <CommandId Name="CMD10" ID="+-1"/>
        <CommandId Name="CMD11" ID="+"/>
        <CommandId Name="CMD12" ID="-"/>
        <CommandId Name="CMD13" ID=" "/>
        <CommandId Name="CMD14" ID="0x 1"/>
        <CommandId Name="CMD15" ID="0b1"/>
        <CommandId Name="CMD16" ID="0o1"/>
        <CommandId Name="CMD17" ID="1.0"/>
        <CommandId Name="CMD18" ID="000ZZ"/>
        <CommandId Name="CMD19" ID="0xZZ"/>
        <CommandId Name="CMD20" ID="12.34e5"/>
        <CommandId Name="CMD21" ID="0b1010"/>
        <CommandId Name="CMD22" ID="0o777"/>
        <CommandId Name="CMD23" ID="12.3"/>
        <CommandId Name="CMD24" ID="123.0"/>
        <CommandId Name="CMD25" ID="123."/>
        <CommandId Name="CMD26" ID=".123"/>
        <CommandId Name="CMD27" ID="0xNOTHEX"/>
        <CommandId Name="CMD28" ID="0x1.0"/>
        <CommandId Name="CMD29" ID="0x.1"/>
        <CommandId Name="CMD30" ID="0x1p1"/>
        <CommandId Name="CMD31" ID="-0x"/>
        <CommandId Name="CMD32" ID="+0x"/>
        <CommandId Name="CMD33" ID="+-123"/>
        <CommandId Name="CMD34" ID="-+123"/>
        <CommandId Name="CMD35" ID="+-0x123"/>
        <CommandId Name="CMD36" ID="-+0x123"/>
        <CommandId Name="CMD37" ID="++123"/>
        <CommandId Name="CMD38" ID="--123"/>
        <CommandId Name="CMD39" ID="0x"/>
        <CommandId Name="CMD40" ID="+"/>
        <CommandId Name="CMD41" ID="-"/>
        <CommandId Name="CMD42" ID="0x+1"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(diags.size() == 42);
    for(int i = 0; i < 42; ++i)
    {
        CHECK(consume_diag().message
              == gta3sc::Diag::config_xml_invalid_command_id);
    }
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with zero values")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="0"/>
        <CommandId Name="CMD2" ID="0x0"/>
        <CommandId Name="CMD3" ID="0X0"/>
        <CommandId Name="CMD4" ID="00000"/>
        <CommandId Name="CMD5" ID="0x00000"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

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
    CHECK(cmd1->target_id().value() == 0);
    CHECK(cmd2->target_id().value() == 0);
    CHECK(cmd3->target_id().value() == 0);
    CHECK(cmd4->target_id().value() == 0);
    CHECK(cmd5->target_id().value() == 0);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id handled override")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="1" Handled="true"/>
        <CommandId Name="CMD1" ID="1" Handled="false"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("CMD1"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->target_id().has_value());
    CHECK(cmd->target_id().value() == 1);
    CHECK_FALSE(cmd->target_handled());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command id with negative id")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="-1"/>
        <CommandId Name="CMD2" ID="32768"/>
        <CommandId Name="CMD3" ID="0x8000"/>
    </Commands>
</GTA3Script>)");

    REQUIRE(diags.size() == 3);
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_command_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse multiple command names with same id")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <CommandId Name="CMD1" ID="42"/>
        <CommandId Name="CMD2" ID="42" Handled="false"/>
        <CommandId Name="CMD3" ID="42"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd1 = table.find_command("CMD1"sv);
    auto* cmd2 = table.find_command("CMD2"sv);
    auto* cmd3 = table.find_command("CMD3"sv);
    REQUIRE(cmd1 != nullptr);
    REQUIRE(cmd2 != nullptr);
    REQUIRE(cmd3 != nullptr);
    CHECK(cmd1->target_id().value() == 42);
    CHECK(cmd2->target_id().value() == 42);
    CHECK(cmd3->target_id().value() == 42);
    CHECK(cmd1->target_handled());
    CHECK_FALSE(cmd2->target_handled());
    CHECK(cmd3->target_handled());
}
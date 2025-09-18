#include "config-fixture.hpp"
#include <filesystem>
#include <gta3sc/sourceman.hpp>
using namespace std::string_view_literals;

namespace gta3sc::test::config
{
class ImportConfigFixture : public LoadConfigFixture
{
public:
    ImportConfigFixture()
    {
        game0_dir = root_test_dir / "game0";
        std::filesystem::create_directories(game0_dir);
        std::filesystem::create_directories(root_test_dir / "game1");
    }

protected:
    auto
    load_test_config(const std::filesystem::path& config_path) -> CommandTable
    {
        return load_test_config(config_path, root_test_dir);
    }

    auto
    load_test_config(const std::filesystem::path& config_path,
                     const std::filesystem::path& root_path) -> CommandTable
    {
        return gta3sc::load_config(root_path, config_path, sourceman, diagman,
                                   gta3sc::CommandTable::Builder(&arena))
                .build();
    }

    // Overrides ConfigFixture::build_config to use create_test_file +
    // load_test_config
    auto build_config(std::string_view src) -> CommandTable
    {
        auto temp_config_file = game0_dir / "config.xml";
        create_test_file(temp_config_file, src);
        return load_test_config(temp_config_file);
    }

    std::filesystem::path game0_dir;
};
} // namespace gta3sc::test::config

using namespace gta3sc::test::config;

TEST_CASE_FIXTURE(ImportConfigFixture, "config import missing name attribute")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(ImportConfigFixture, "config import empty name attribute")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name=""/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import security validation - slash in From")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="config.xml" From="game1/other"/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_security_import_filesystem_traversal);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import security validation - backslash in From")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="config.xml" From="game1\\other"/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_security_import_filesystem_traversal);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import security validation - dot in From")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="config.xml" From="game1.other"/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_security_import_filesystem_traversal);
}

TEST_CASE_FIXTURE(ImportConfigFixture, "config import without from attribute")
{
    create_test_file(game0_dir / "imported.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="IMPORTED_CMD"/>
    </Commands>
</GTA3Script>)");

    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="imported.xml"/>
    <Commands>
        <Command Name="MAIN_CMD"/>
    </Commands>
</GTA3Script>)");

    auto table = load_test_config(game0_dir / "config.xml");

    CHECK(diags.empty());

    auto* main_cmd = table.find_command("MAIN_CMD"sv);
    REQUIRE(main_cmd != nullptr);

    auto* imported_cmd = table.find_command("IMPORTED_CMD"sv);
    REQUIRE(imported_cmd != nullptr);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import with from equals current config dir")
{
    // Create the imported file
    create_test_file(game0_dir / "imported.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="IMPORTED_CMD"/>
    </Commands>
</GTA3Script>)");

    // Create the main config file
    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="imported.xml" From="."/>
    <Commands>
        <Command Name="MAIN_CMD"/>
    </Commands>
</GTA3Script>)");

    auto table = load_test_config(game0_dir / "config.xml");

    CHECK(diags.empty());

    auto* main_cmd = table.find_command("MAIN_CMD"sv);
    REQUIRE(main_cmd != nullptr);

    auto* imported_cmd = table.find_command("IMPORTED_CMD"sv);
    REQUIRE(imported_cmd != nullptr);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import with from equals game config dir")
{
    create_test_file(root_test_dir / "game1" / "imported.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="GAME1_CMD"/>
    </Commands>
</GTA3Script>)");

    // Create the main config file
    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="imported.xml" From="game1"/>
    <Commands>
        <Command Name="MAIN_CMD"/>
    </Commands>
</GTA3Script>)");

    auto table = load_test_config(game0_dir / "config.xml");

    CHECK(diags.empty());

    auto* main_cmd = table.find_command("MAIN_CMD"sv);
    REQUIRE(main_cmd != nullptr);

    auto* game1_cmd = table.find_command("GAME1_CMD"sv);
    REQUIRE(game1_cmd != nullptr);
}

TEST_CASE_FIXTURE(ImportConfigFixture, "config import with non-existent file")
{
    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="nonexistent.xml" From="."/>
</GTA3Script>)");

    load_test_config(game0_dir / "config.xml");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_could_not_open_file);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import with invalid game config path")
{
    auto root_config_dir = root_test_dir / "root";
    std::filesystem::create_directories(root_config_dir);

    // To determine game config we must be somewhere inside root_config_dir
    // Let's create a directory that violates this.
    auto outside_root_dir = root_test_dir / "outside_root";
    std::filesystem::create_directories(outside_root_dir);

    create_test_file(outside_root_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
        <GTA3Script Version="2.0">
            <Import Name="imported.xml"/>
        </GTA3Script>)");

    load_test_config(outside_root_dir / "config.xml", root_config_dir);

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_import_failed_to_determine_game_config);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import with malformed XML in imported file")
{
    // Create a malformed XML file
    create_test_file(game0_dir / "malformed.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="TEST_CMD" <!-- missing closing tag -->
    </Commands>
</GTA3Script>)");

    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="malformed.xml" From="."/>
    <Commands>
        <Command Name="MAIN_CMD"/>
    </Commands>
</GTA3Script>)");

    load_test_config(game0_dir / "config.xml");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_parse_failed);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import depth limit - nested import")
{
    // Create a file that tries to import another file (depth 2)
    create_test_file(game0_dir / "level2.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="LEVEL2_CMD"/>
    </Commands>
</GTA3Script>)");

    create_test_file(game0_dir / "level1.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="level2.xml"/>
    <Commands>
        <Command Name="LEVEL1_CMD"/>
    </Commands>
</GTA3Script>)");

    // Create the main config file that imports level1 (depth 1)
    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="level1.xml"/>
    <Commands>
        <Command Name="MAIN_CMD"/>
    </Commands>
</GTA3Script>)");

    load_test_config(game0_dir / "config.xml");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_import_too_deep);
}

TEST_CASE_FIXTURE(ImportConfigFixture,
                  "config import with empty from attribute")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="config.xml" From=""/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_empty_attr);
}

TEST_CASE_FIXTURE(
        ImportConfigFixture,
        "config import with duplicate command names and multiple imports")
{
    create_test_file(game0_dir / "head.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="CMD1">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
        <Command Name="CMD2">
            <Params>
                <Param Type="FLOAT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    create_test_file(game0_dir / "tail.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="CMD3">
            <Params>
                <Param Type="STRING"/>
            </Params>
        </Command>
        <Command Name="CMD2">
            <Params>
                <Param Type="VAR_INT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    // Create main config file: import first, define CMD1 and CMD3, import
    // second
    create_test_file(game0_dir / "config.xml",
                     R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Import Name="head.xml" From="."/>
    <Commands>
        <Command Name="CMD1">
            <Params>
                <Param Type="VAR_FLOAT"/>
            </Params>
        </Command>
        <Command Name="CMD3">
            <Params>
                <Param Type="LABEL"/>
            </Params>
        </Command>
    </Commands>
    <Import Name="tail.xml" From="."/>
</GTA3Script>)");

    auto table = load_test_config(game0_dir / "config.xml");

    CHECK(diags.empty());

    // CMD1 should be from main config (overrides head.xml)
    auto* cmd1 = table.find_command("CMD1"sv);
    REQUIRE(cmd1 != nullptr);
    CHECK(cmd1->num_params() == 1);
    CHECK(cmd1->param(0).type == gta3sc::CommandTable::ParamType::VAR_FLOAT);

    // CMD2 should be from tail.xml (overrides head.xml)
    auto* cmd2 = table.find_command("CMD2"sv);
    REQUIRE(cmd2 != nullptr);
    CHECK(cmd2->num_params() == 1);
    CHECK(cmd2->param(0).type == gta3sc::CommandTable::ParamType::VAR_INT);

    // CMD3 should be from tail.xml (overrides main)
    auto* cmd3 = table.find_command("CMD3"sv);
    REQUIRE(cmd3 != nullptr);
    CHECK(cmd3->num_params() == 1);
    CHECK(cmd3->param(0).type == gta3sc::CommandTable::ParamType::STRING);
}
#include "config-fixture.hpp"
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command missing name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
<Commands>
<Command/>
</Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command empty name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
<Commands>
<Command Name=""/>
</Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty commands with command element")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 0);
    CHECK(cmd->num_min_params() == 0);
    CHECK_FALSE(cmd->has_optional_param());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command name case insensitive lookup")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="foo"/>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->name() == "FOO"sv);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse command unknown child rejected")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Bar/>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params list with one param of each kind basic")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
                <Param Type="FLOAT"/>
                <Param Type="INPUT_INT"/>
                <Param Type="OUTPUT_FLOAT"/>
                <Param Type="LABEL"/>
                <Param Type="TEXT_LABEL"/>
                <Param Type="STRING"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 7);
    CHECK(cmd->num_min_params() == 7);
    CHECK_FALSE(cmd->has_optional_param());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse param types conversion for all supported types")
{
    using ParamType = gta3sc::CommandTable::ParamType;

    const std::pair<const char*, ParamType> test_cases[] = {
            {"INT", ParamType::INT},
            {"FLOAT", ParamType::FLOAT},
            {"VAR_INT", ParamType::VAR_INT},
            {"LVAR_INT", ParamType::LVAR_INT},
            {"VAR_FLOAT", ParamType::VAR_FLOAT},
            {"LVAR_FLOAT", ParamType::LVAR_FLOAT},
            {"VAR_TEXT_LABEL", ParamType::VAR_TEXT_LABEL},
            {"LVAR_TEXT_LABEL", ParamType::LVAR_TEXT_LABEL},
            {"INPUT_INT", ParamType::INPUT_INT},
            {"INPUT_FLOAT", ParamType::INPUT_FLOAT},
            {"OUTPUT_INT", ParamType::OUTPUT_INT},
            {"OUTPUT_FLOAT", ParamType::OUTPUT_FLOAT},
            {"LABEL", ParamType::LABEL},
            {"TEXT_LABEL", ParamType::TEXT_LABEL},
            {"STRING", ParamType::STRING},
            {"VAR_INT_OPT", ParamType::VAR_INT_OPT},
            {"LVAR_INT_OPT", ParamType::LVAR_INT_OPT},
            {"VAR_FLOAT_OPT", ParamType::VAR_FLOAT_OPT},
            {"LVAR_FLOAT_OPT", ParamType::LVAR_FLOAT_OPT},
            {"VAR_TEXT_LABEL_OPT", ParamType::VAR_TEXT_LABEL_OPT},
            {"LVAR_TEXT_LABEL_OPT", ParamType::LVAR_TEXT_LABEL_OPT},
            {"INPUT_OPT", ParamType::INPUT_OPT},
    };

    for(auto [type_str, expected_type] : test_cases)
    {
        SUBCASE(type_str)
        {
            // TODO use std::format
            auto xml = std::string(
                               "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                               "<GTA3Script Version=\"2.0\">\n"
                               "    <Commands>\n"
                               "        <Command Name=\"FOO\">\n"
                               "            <Params>\n"
                               "                <Param Type=\"")
                       + type_str
                       + std::string("\"/>\n"
                                     "            </Params>\n"
                                     "        </Command>\n"
                                     "    </Commands>\n"
                                     "</GTA3Script>");

            auto table = build_config(xml);
            CHECK(diags.empty());

            auto* cmd = table.find_command("FOO"sv);
            REQUIRE(cmd != nullptr);
            CHECK(cmd->num_params() == 1);
            CHECK(cmd->param(0).type == expected_type);
        }
    }
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse *_OPT must be last parameter error")
{
    // TODO use std::format
    auto make_xml = [](std::string_view opt_type, std::string_view next_type) {
        return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                           "<GTA3Script Version=\"2.0\">\n"
                           "    <Commands>\n"
                           "        <Command Name=\"FOO\">\n"
                           "            <Params>\n"
                           "                <Param Type=\"")
               + std::string(opt_type) + std::string("\"/>\n")
               + std::string("                <Param Type=\"")
               + std::string(next_type)
               + std::string("\"/>\n"
                             "            </Params>\n"
                             "        </Command>\n"
                             "    </Commands>\n"
                             "</GTA3Script>");
    };

    const char* opt_types[] = {
            "VAR_INT_OPT",    "LVAR_INT_OPT",       "VAR_FLOAT_OPT",
            "LVAR_FLOAT_OPT", "VAR_TEXT_LABEL_OPT", "LVAR_TEXT_LABEL_OPT",
            "INPUT_OPT",
    };

    for(auto opt : opt_types)
    {
        SUBCASE(opt)
        {
            auto table = build_config(make_xml(opt, "INT"));
            REQUIRE(!diags.empty());
            CHECK(consume_diag().message
                  == gta3sc::Diag::config_xml_opt_must_be_last_param);
        }
    }
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse param missing type attribute")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse param with invalid Type value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="NOT_A_TYPE"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_param_type);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse param with empty Type attribute value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type=""/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message
          == gta3sc::Diag::config_xml_invalid_param_type);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params invalid child under Params")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
                <Unknown/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params must be only child of Command")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
            </Params>
            <AlsoBad/>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse multiple params not allowed")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
            </Params>
            <Params>
                <Param Type="FLOAT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params with Enum and Entity refs")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="VAR_INT" Enum="MyEnum"/>
                <Param Type="LVAR_INT" Entity="Car"/>
                <Param Type="INPUT_INT" Enum="MyEnum" Entity="SomeEntity"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 3);

    auto enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(enum_id.has_value());
    auto car_id = table.find_entity_type("CAR"sv);
    REQUIRE(car_id.has_value());
    auto sentity_id = table.find_entity_type("SOMEENTITY"sv);
    REQUIRE(sentity_id.has_value());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params empty")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());
    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 0);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse params enum and entity case-insensitive")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="VAR_INT" Enum="MyEnum"/>
                <Param Type="LVAR_INT" Entity="MyEntity"/>
                <Param Type="INPUT_INT" Enum="myenum" Entity="myentity"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto my_enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(my_enum_id.has_value());
    auto my_entity_id = table.find_entity_type("MYENTITY"sv);
    REQUIRE(my_entity_id.has_value());

    // check the three params point to enum_id and entity_id
    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->param(0).enum_type == *my_enum_id);
    CHECK(cmd->param(1).entity_type == *my_entity_id);
    CHECK(cmd->param(2).enum_type == *my_enum_id);
    CHECK(cmd->param(2).entity_type == *my_entity_id);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse param defaults to global enum and no entity")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="BAR">
            <Params>
                <Param Type="INT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("BAR"sv);
    REQUIRE(cmd != nullptr);
    REQUIRE(cmd->num_params() == 1);
    CHECK(cmd->param(0).enum_type == gta3sc::CommandTable::global_enum);
    CHECK(cmd->param(0).entity_type == gta3sc::CommandTable::no_entity_type);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty entity attribute value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT" Entity=""/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_empty_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty enum attribute value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT" Enum=""/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().message == gta3sc::Diag::config_xml_empty_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse optional parameter handling")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INPUT_INT"/>
                <Param Type="INPUT_OPT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 2);
    CHECK(cmd->num_min_params() == 1);
    CHECK(cmd->has_optional_param());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse redefinition replaces parameters")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Commands>
        <Command Name="FOO">
            <Params>
                <Param Type="INT"/>
                <Param Type="INT"/>
            </Params>
        </Command>
        <Command Name="foo">
            <Params>
                <Param Type="FLOAT"/>
            </Params>
        </Command>
    </Commands>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* cmd = table.find_command("FOO"sv);
    REQUIRE(cmd != nullptr);
    CHECK(cmd->num_params() == 1);
    CHECK(cmd->param(0).type == gta3sc::CommandTable::ParamType::FLOAT);
}
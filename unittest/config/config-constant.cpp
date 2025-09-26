#include "config-fixture.hpp"
#include <limits>
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty constants section")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown node in constants")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <UnknownNode/>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown node in enum")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <UnknownNode/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty enum")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="MyEnum">
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(enum_id.has_value());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unnamed enum")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="1"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* constant = table.find_constant(gta3sc::CommandTable::global_enum,
                                         "CONST1"sv);
    REQUIRE(constant != nullptr);
    CHECK(constant->value() == 1);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constants should auto-increment")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1"/>
            <Constant Name="CONST2"/>
            <Constant Name="CONST3"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    CHECK(const1->value() == 0);
    CHECK(const2->value() == 1);
    CHECK(const3->value() == 2);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constants auto-increment after value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1"/>
            <Constant Name="CONST2" Value="5"/>
            <Constant Name="CONST3"/>
            <Constant Name="CONST4"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    auto* const4 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST4"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    CHECK(const1->value() == 0);
    CHECK(const2->value() == 5);
    CHECK(const3->value() == 6);
    CHECK(const4->value() == 7);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constants auto-increment per enum")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="ENUM1">
            <Constant Name="CONST1"/>
            <Constant Name="CONST2"/>
        </Enum>
        <Enum Name="ENUM2">
            <Constant Name="CONST3"/>
            <Constant Name="CONST4"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    // Get enum IDs
    auto enum1_id = table.find_enumeration("ENUM1"sv);
    auto enum2_id = table.find_enumeration("ENUM2"sv);
    REQUIRE(enum1_id.has_value());
    REQUIRE(enum2_id.has_value());

    // Check constants in first enum
    auto* const1 = table.find_constant(*enum1_id, "CONST1"sv);
    auto* const2 = table.find_constant(*enum1_id, "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == 0);
    CHECK(const2->value() == 1);

    // Check constants in second enum (should start from 0 again)
    auto* const3 = table.find_constant(*enum2_id, "CONST3"sv);
    auto* const4 = table.find_constant(*enum2_id, "CONST4"sv);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    CHECK(const3->value() == 0);
    CHECK(const4->value() == 1);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config extend enum resets auto-increment")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="MYENUM">
            <Constant Name="CONST1"/>
            <Constant Name="CONST2"/>
        </Enum>
        <Enum Name="MYENUM">
            <Constant Name="CONST3"/>
            <Constant Name="CONST4"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(enum_id.has_value());

    // Check constants from first enum definition
    auto* const1 = table.find_constant(*enum_id, "CONST1"sv);
    auto* const2 = table.find_constant(*enum_id, "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == 0);
    CHECK(const2->value() == 1);

    // Check constants from second enum definition (should start from 0 again)
    auto* const3 = table.find_constant(*enum_id, "CONST3"sv);
    auto* const4 = table.find_constant(*enum_id, "CONST4"sv);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    CHECK(const3->value() == 0);
    CHECK(const4->value() == 1);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with decimal value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="42"/>
            <Constant Name="CONST2" Value="-1"/>
            <Constant Name="CONST3" Value="123456"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    CHECK(const1->value() == 42);
    CHECK(const2->value() == -1);
    CHECK(const3->value() == 123456);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with hex value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="0xA2"/>
            <Constant Name="CONST2" Value="0xFFFF"/>
            <Constant Name="CONST3" Value="0x1E240"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    // Check the actual values
    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    CHECK(const1->value() == 0xA2);
    CHECK(const2->value() == 0xFFFF);
    CHECK(const3->value() == 0x1E240);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with hex format variations")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="0xabcdef"/>
            <Constant Name="CONST2" Value="0XABCDEF"/>
            <Constant Name="CONST3" Value="0xABCDEF"/>
            <Constant Name="CONST4" Value="0xaBcDeF"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    auto* const4 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST4"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    CHECK(const1->value() == 0xABCDEF);
    CHECK(const2->value() == 0xABCDEF);
    CHECK(const3->value() == 0xABCDEF);
    CHECK(const4->value() == 0xABCDEF);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant missing name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Value="1"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with empty value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value=""/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with invalid value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="not a number"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse multiple enums")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="ENUM1">
            <Constant Name="CONST1" Value="1"/>
        </Enum>
        <Enum Name="ENUM2">
            <Constant Name="CONST2" Value="2"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto enum1_id = table.find_enumeration("ENUM1"sv);
    auto enum2_id = table.find_enumeration("ENUM2"sv);
    REQUIRE(enum1_id.has_value());
    REQUIRE(enum2_id.has_value());

    auto* const1 = table.find_constant(*enum1_id, "CONST1"sv);
    auto* const2 = table.find_constant(*enum2_id, "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == 1);
    CHECK(const2->value() == 2);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture, "config extend enum")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="MyEnum">
            <Constant Name="CONST1" Value="1"/>
        </Enum>
        <Enum Name="MYENUM">
            <Constant Name="CONST2" Value="5"/>
        </Enum>
        <Enum Name="myenum">
            <Constant Name="CONST3" Value="7"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(enum_id.has_value());

    auto* const1 = table.find_constant(*enum_id, "CONST1"sv);
    auto* const2 = table.find_constant(*enum_id, "CONST2"sv);
    auto* const3 = table.find_constant(*enum_id, "CONST3"sv);

    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);

    CHECK(const1->value() == 1);
    CHECK(const2->value() == 5);
    CHECK(const3->value() == 7);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture, "config extend constant")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="MYENUM">
            <Constant Name="MyConstant" Value="1"/>
            <Constant Name="MYCONSTANT" Value="5"/>
            <Constant Name="myconstant" Value="9"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto enum_id = table.find_enumeration("MYENUM"sv);
    REQUIRE(enum_id.has_value());

    auto* constant = table.find_constant(*enum_id, "MYCONSTANT"sv);
    REQUIRE(constant != nullptr);
    CHECK(constant->value() == 9);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config constants with same name in different enums")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="ENUM1">
            <Constant Name="CONST1" Value="1"/>
            <Constant Name="CONST2" Value="2"/>
        </Enum>
        <Enum Name="ENUM2">
            <Constant Name="CONST1" Value="100"/>
            <Constant Name="CONST2" Value="200"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    // Get enum IDs
    auto enum1_id = table.find_enumeration("ENUM1"sv);
    auto enum2_id = table.find_enumeration("ENUM2"sv);
    REQUIRE(enum1_id.has_value());
    REQUIRE(enum2_id.has_value());

    // Check constants in first enum
    auto* const1_enum1 = table.find_constant(*enum1_id, "CONST1"sv);
    auto* const2_enum1 = table.find_constant(*enum1_id, "CONST2"sv);
    REQUIRE(const1_enum1 != nullptr);
    REQUIRE(const2_enum1 != nullptr);
    CHECK(const1_enum1->value() == 1);
    CHECK(const2_enum1->value() == 2);

    // Check constants in second enum (should have different values)
    auto* const1_enum2 = table.find_constant(*enum2_id, "CONST1"sv);
    auto* const2_enum2 = table.find_constant(*enum2_id, "CONST2"sv);
    REQUIRE(const1_enum2 != nullptr);
    REQUIRE(const2_enum2 != nullptr);
    CHECK(const1_enum2->value() == 100);
    CHECK(const2_enum2->value() == 200);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty enum name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum Name="">
            <Constant Name="CONST1" Value="1"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_empty_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty constant name")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="" Value="1"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse duplicate constant names")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="1"/>
            <Constant Name="CONST1" Value="9"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* constant = table.find_constant(gta3sc::CommandTable::global_enum,
                                         "CONST1"sv);
    REQUIRE(constant != nullptr);
    CHECK(constant->value() == 9);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse invalid hex value with non-hex digits")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="0xGHIJ"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse decimal value overflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="2147483648"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with large positive hex value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="0x80000000"/>
            <Constant Name="CONST2" Value="0xFFFFFFFF"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == std::numeric_limits<int32_t>::min());
    CHECK(const2->value() == -1);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with negative hex value")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="-0x7FFFFFFF"/>
            <Constant Name="CONST2" Value="-0x1"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == -0x7FFFFFFF);
    CHECK(const2->value() == -1);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with decimal value limits")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="2147483647"/>
            <Constant Name="CONST2" Value="-2147483648"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    CHECK(const1->value() == std::numeric_limits<int32_t>::max());
    CHECK(const2->value() == std::numeric_limits<int32_t>::min());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with decimal value overflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="2147483648"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with decimal value underflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="-2147483649"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with negative hex overflow")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="-0x80000001"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with zero values")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="0"/>
            <Constant Name="CONST2" Value="-0"/>
            <Constant Name="CONST3" Value="0x0"/>
            <Constant Name="CONST4" Value="0X0"/>
            <Constant Name="CONST5" Value="-0x0"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    auto* const4 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST4"sv);
    auto* const5 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST5"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    REQUIRE(const5 != nullptr);
    CHECK(const1->value() == 0);
    CHECK(const2->value() == 0);
    CHECK(const3->value() == 0);
    CHECK(const4->value() == 0);
    CHECK(const5->value() == 0);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with leading zeros")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="000123"/>
            <Constant Name="CONST2" Value="-000000123"/>
            <Constant Name="CONST3" Value="0x00000123"/>
            <Constant Name="CONST4" Value="-0x00000123"/>
            <Constant Name="CONST5" Value="00000000000000000000005"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    auto* const4 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST4"sv);
    auto* const5 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST5"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    REQUIRE(const5 != nullptr);
    CHECK(const1->value() == 123);
    CHECK(const2->value() == -123);
    CHECK(const3->value() == 0x123);
    CHECK(const4->value() == -0x123);
    CHECK(const5->value() == 5);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse sign variations")
{
    // Valid signs
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value="+123"/>
            <Constant Name="CONST2" Value="-123"/>
            <Constant Name="CONST3" Value="+0x123"/>
            <Constant Name="CONST4" Value="-0x123"/>
            <Constant Name="CONST5" Value="+0"/>
            <Constant Name="CONST6" Value="-0"/>
            <Constant Name="CONST7" Value="+0x0"/>
            <Constant Name="CONST8" Value="-0x0"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    CHECK(diags.empty());

    auto* const1 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST1"sv);
    auto* const2 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST2"sv);
    auto* const3 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST3"sv);
    auto* const4 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST4"sv);
    auto* const5 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST5"sv);
    auto* const6 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST6"sv);
    auto* const7 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST7"sv);
    auto* const8 = table.find_constant(gta3sc::CommandTable::global_enum,
                                       "CONST8"sv);
    REQUIRE(const1 != nullptr);
    REQUIRE(const2 != nullptr);
    REQUIRE(const3 != nullptr);
    REQUIRE(const4 != nullptr);
    REQUIRE(const5 != nullptr);
    REQUIRE(const6 != nullptr);
    REQUIRE(const7 != nullptr);
    REQUIRE(const8 != nullptr);
    CHECK(const1->value() == 123);
    CHECK(const2->value() == -123);
    CHECK(const3->value() == 0x123);
    CHECK(const4->value() == -0x123);
    CHECK(const5->value() == 0);
    CHECK(const6->value() == 0);
    CHECK(const7->value() == 0);
    CHECK(const8->value() == 0);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with invalid formats")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value=""/>
            <Constant Name="CONST2" Value="000ZZ"/>
            <Constant Name="CONST3" Value="0xZZ"/>
            <Constant Name="CONST4" Value="12.34e5"/>
            <Constant Name="CONST5" Value="0b1010"/>
            <Constant Name="CONST6" Value="0o777"/>
            <Constant Name="CONST7" Value="12.3"/>
            <Constant Name="CONST8" Value="123.0"/>
            <Constant Name="CONST9" Value="123."/>
            <Constant Name="CONST10" Value=".123"/>
            <Constant Name="CONST11" Value="0xNOTHEX"/>
            <Constant Name="CONST12" Value="0x1.0"/>
            <Constant Name="CONST13" Value="0x.1"/>
            <Constant Name="CONST14" Value="0x1p1"/>
            <Constant Name="CONST15" Value="0x"/>
            <Constant Name="CONST16" Value="-0x"/>
            <Constant Name="CONST17" Value="+0x"/>
            <Constant Name="CONST18" Value="0x+1"/>
            <Constant Name="CONST19" Value="+"/>
            <Constant Name="CONST20" Value="-"/>
            <Constant Name="CONST21" Value="+-123"/>
            <Constant Name="CONST22" Value="-+123"/>
            <Constant Name="CONST23" Value="+-0x123"/>
            <Constant Name="CONST24" Value="-+0x123"/>
            <Constant Name="CONST25" Value="++123"/>
            <Constant Name="CONST26" Value="--123"/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(diags.size() == 26);
    for(int i = 0; i < 26; ++i)
        CHECK(consume_diag().descriptor
              == &gta3sc::config::diag::xml_invalid_constant_value);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse constant with whitespace")
{
    auto table = build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <Constants>
        <Enum>
            <Constant Name="CONST1" Value=" 123"/>
            <Constant Name="CONST2" Value="123 "/>
            <Constant Name="CONST3" Value=" 0x123"/>
            <Constant Name="CONST4" Value="0x123 "/>
            <Constant Name="CONST5" Value=" -123 "/>
            <Constant Name="CONST6" Value=" -0x123 "/>
        </Enum>
    </Constants>
</GTA3Script>)");

    REQUIRE(diags.size() == 6);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_constant_value);
}
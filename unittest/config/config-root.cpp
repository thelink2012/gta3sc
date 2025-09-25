#include "config-fixture.hpp"
using namespace std::string_view_literals;

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty file")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
</GTA3Script>)");

    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse invalid xml")
{
    build_config("this is not xml");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_parse_failed);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse missing root element")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_parse_failed);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse wrong root element")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<WrongRoot Version="2.0">
</WrongRoot>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_invalid_root_element);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse missing version")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse wrong version")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="1.0">
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse root with unknown attributes")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0" UnknownAttr="value">
</GTA3Script>)");

    CHECK(diags.empty()); // Unknown attributes should be ignored
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse empty version value")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="">
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::xml_missing_required_attr);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse unknown root child")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    <UnknownNode/>
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse root with text")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    Some text content
</GTA3Script>)");

    REQUIRE(!diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::config::diag::xml_unknown_node);
}

TEST_CASE_FIXTURE(gta3sc::test::config::ConfigFixture,
                  "config parse root with mixed content")
{
    build_config(R"(<?xml version="1.0" encoding="utf-8"?>
<GTA3Script Version="2.0">
    Some text
    <UnknownNode/>
    More text
</GTA3Script>)");

    REQUIRE(!diags.empty());
    auto diag1 = consume_diag();
    auto diag2 = consume_diag();
    auto diag3 = consume_diag();
    CHECK(diag1.descriptor == &gta3sc::config::diag::xml_unknown_node);
    CHECK(diag2.descriptor == &gta3sc::config::diag::xml_unknown_node);
    CHECK(diag3.descriptor == &gta3sc::config::diag::xml_unknown_node);
}
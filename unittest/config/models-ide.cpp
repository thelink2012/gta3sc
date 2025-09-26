#include "models-fixture.hpp"
#include <gta3sc/config/models.hpp>
#include <gta3sc/model-table.hpp>
using namespace std::string_view_literals;
using namespace gta3sc::test::config;

namespace gta3sc::test::config
{
class ModelsIdeTestFixture : public ModelsTestFixture
{
public:
    auto load_models(std::string_view ide_content,
                     bool objs_only = false) -> gta3sc::ModelTable
    {
        auto source = make_source(ide_content);
        return gta3sc::config::load_models_from_ide(
                       source, objs_only, diagman,
                       gta3sc::ModelTable::Builder(&arena))
                .build();
    }
};
} // namespace gta3sc::test::config

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_ide with file path could not open file")
{
    auto table = gta3sc::config::load_models_from_ide(
                         "nonexistent_file.ide", false, sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_ide with file path and with objs_only set")
{
    create_test_file("test_models.ide", R"(cars
400, landstal, ...
end
weap
321, gun_dildo1, ...
end
objs
14531, int_zerosrc01, ...
end)");

    auto table = gta3sc::config::load_models_from_ide(
                         root_test_dir / "test_models.ide", true, sourceman,
                         diagman, gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 1);

    auto* model = table.find_model("INT_ZEROSRC01"sv);
    REQUIRE(model != nullptr);
    CHECK(model->model_id() == 14531);
}

TEST_CASE_FIXTURE(
        ModelsTestFixture,
        "load_models_from_ide with file path and with objs_only unset")
{
    create_test_file("test_models.ide", R"(cars
400, landstal, ...
end
weap
321, gun_dildo1, ...
end
objs
14531, int_zerosrc01, ...
end)");

    auto table = gta3sc::config::load_models_from_ide(
                         root_test_dir / "test_models.ide", false, sourceman,
                         diagman, gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    auto* landstal = table.find_model("LANDSTAL"sv);
    REQUIRE(landstal != nullptr);
    CHECK(landstal->model_id() == 400);

    auto* gun_dildo1 = table.find_model("GUN_DILDO1"sv);
    REQUIRE(gun_dildo1 != nullptr);
    CHECK(gun_dildo1->model_id() == 321);

    auto* int_zerosrc01 = table.find_model("INT_ZEROSRC01"sv);
    REQUIRE(int_zerosrc01 != nullptr);
    CHECK(int_zerosrc01->model_id() == 14531);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with objs_only unset")
{
    auto table = load_models(R"(cars
400, landstal, ...
401, bravura, ...
end
weap
321, gun_dildo1, ...
end
objs
14531, int_zerosrc01, ...
end
)");

    CHECK(diags.empty());
    CHECK(table.size() == 4);

    expect_model(table, "LANDSTAL", 400, "LANDSTAL");
    expect_model(table, "BRAVURA", 401, "BRAVURA");
    expect_model(table, "GUN_DILDO1", 321, "GUN_DILDO1");
    expect_model(table, "INT_ZEROSRC01", 14531, "INT_ZEROSRC01");
    expect_no_model(table, "NONEXISTANT");
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with objs_only set")
{
    auto table = load_models(R"(cars
400, landstal, ...
end
weap
321, gun_dildo1, ...
end
hier
1000, csplay, ...
end
peds
1, player, ...
end
path
2000, path_node, ...
end
2dfx
3000, effect_light, ...
end
txdp
4000, texture_dict, ...
end
hand
5000, hand_model, ...
end
objs
14531, int_zerosrc01, ...
end
tobj
14541, driveschl_daylite, ...
end
anim
10000, foobar, ...
end)",
                             true);

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    // Should only have objs, tobj, and anim models, not other sections
    expect_no_model(table, "LANDSTAL");
    expect_no_model(table, "GUN_DILDO1");
    expect_no_model(table, "CSPLAY");
    expect_no_model(table, "PLAYER");
    expect_no_model(table, "PATH_NODE");
    expect_no_model(table, "EFFECT_LIGHT");
    expect_no_model(table, "TEXTURE_DICT");
    expect_no_model(table, "HAND_MODEL");

    // These should be present (objs, tobj, anim sections)
    expect_model(table, "INT_ZEROSRC01", 14531);
    expect_model(table, "DRIVESCHL_DAYLITE", 14541);
    expect_model(table, "FOOBAR", 10000, "FOOBAR");
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide converts case of model names")
{
    auto table = load_models(R"(cars
400, landstal, ...
401, BRAVURA, ...
402, FooBar, ...
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    expect_model(table, "LANDSTAL", 400, "LANDSTAL");
    expect_model(table, "BRAVURA", 401, "BRAVURA");
    expect_model(table, "FOOBAR", 402, "FOOBAR");
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with invalid lines")
{
    auto table = load_models(R"(cars
400, landstal, ...
invalid_line_without_id
401, bravura, ...
another_invalid_line
end)");

    // Should have 2 diagnostics for invalid lines
    REQUIRE(diags.size() == 2);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::models_invalid_ide_line);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::models_invalid_ide_line);

    CHECK(table.size() == 2);

    // Valid models should still be loaded
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "BRAVURA", 401);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture, "load_models_from_ide with empty file")
{
    auto table = load_models("");

    CHECK(diags.empty());
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with only comments and whitespace")
{
    auto table = load_models(R"(# This is a comment
   # Another comment with spaces
   
   # Empty lines above and below
   
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with malformed sections")
{
    auto table = load_models(R"(cars
400, landstal, ...
# Missing end statement - should continue processing
peds
98, pedname, ...
end
# Section without content
weap
end)");

    CHECK(diags.size() == 1);
    CHECK(consume_diag().descriptor
          == &gta3sc::config::diag::models_invalid_ide_line);

    CHECK(table.size() == 2);

    // All valid models should be loaded despite malformed sections
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "PEDNAME", 98);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with real GTA IDE format")
{
    auto table = load_models(R"(cars
400, landstal, landstal, car, LANDSTAL, LANDSTK, null, normal, 10, 0, 0, -1, 0.768, 0.768, 0
401, bravura, bravura, car, BRAVURA, BRAVURA, null, poorfamily, 10, 0, 0, -1, 0.74, 0.74, 0
end
weap
321, gun_dildo1, gun_dildo1, null, 1, 50, 0
322, gun_dildo2, gun_dildo2, null, 1, 50, 0
end
objs
14530, driveschl_main, estate2, 30, 0
14531, int_zerosrc01, int_zerosrcA, 30, 0
end
tobj
14541, driveschl_daylite, blindingLite, 100, 4, 6, 21
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 7);

    // Verify all model types are loaded
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "BRAVURA", 401);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "GUN_DILDO2", 322);
    expect_model(table, "DRIVESCHL_MAIN", 14530);
    expect_model(table, "INT_ZEROSRC01", 14531);
    expect_model(table, "DRIVESCHL_DAYLITE", 14541);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with buffer overflow edge cases")
{
    auto table = load_models(R"(cars
399, this_is_a_very_long_model_name_that_exceeds_the_63_character_limit_for_model_names_in_the_sscanf_buffer, ...
400, landstal, aaaaaaaaaaaaa, bbbbbbbbbbb, cccccccccc, ddddddddd, eeeeeeeee, extra, very, long, line, that, exceeds, the, character, buffer, limit, and, should, be, truncated, but, still, parse, the, first, two, fields, correctly
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 2);

    // First model should be parsed correctly despite long line
    expect_model(table, "LANDSTAL", 400);

    // Second model name should be truncated to 63 characters
    expect_no_model(table,
                    "THIS_IS_A_VERY_LONG_MODEL_NAME_THAT_EXCEEDS_THE_63_"
                    "CHARACTER_LIMIT_FOR_MODEL_NAMES_IN_THE_SSCANF_BUFFER");

    // Check if truncated version exists (first 63 chars)
    expect_model(
            table,
            "THIS_IS_A_VERY_LONG_MODEL_NAME_THAT_EXCEEDS_THE_63_CHARACTER_LI",
            399);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with duplicate models")
{
    auto table = load_models(R"(cars
400, landstal, ...
400, duplicate_id, ...
401, bravura, ...
402, bravura, ...
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "DUPLICATE_ID", 400);
    expect_model(table, "BRAVURA", 402); // overwritten from 401
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with numeric edge cases")
{
    auto table = load_models(R"(cars
0, zero_id, ...
4294967295, max_uint32, ...
999999999, large_id, ...
4294967297, over_max,...
18446744073709551615, huge_number, ...
end)");

    CHECK(diags.empty());
    CHECK(table.size() == 5);

    // Test zero ID
    expect_model(table, "ZERO_ID", 0);

    // Test maximum uint32 ID
    expect_model(table, "MAX_UINT32", 4294967295U);

    // Test large but reasonable ID
    expect_model(table, "LARGE_ID", 999999999U);

    // Values over max_uint32 should be truncated to fit in uint32_t (may change
    // in the future to error)
    expect_model(table, "OVER_MAX", 1); // 4294967297 % 2^32 = 1
    expect_model(table, "HUGE_NUMBER",
                 4294967295U); // 18446744073709551615 % 2^32 = 4294967295
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with line parsing edge cases")
{
    // Use string concatenation to include actual tab and carriage return
    // characters
    auto table = load_models(
            "cars\n"
            "# Comment line should be skipped\n"
            "400, landstal, landstal, car, LANDSTAL, LANDSTK, null, normal, "
            "10, 0, 0, -1, 0.768, 0.768, 0\n"
            "   # Comment with leading whitespace\n"
            "401, bravura, bravura, car, BRAVURA, BRAVURA, null, poorfamily, "
            "10, 0, 0, -1, 0.74, 0.74, 0\n"
            "# Multiple consecutive comments\n"
            "# Another comment\n"
            "402, esperant, esperant, car, ESPERANT, ESPERAN, null, normal, "
            "10, 0, 0, -1, 0.64, 0.64, 0\n"
            "   ,,,,   ,,,,   403, pony, pony, car, PONY, PONY, van, worker, "
            "10, 0, 0, -1, 0.72, 0.72, -1\n"
            "\t\t\t404, mule, mule, car, MULE, MULE, null, worker, 10, 0, 0, "
            "-1, 0.76, 0.76, -1\n"
            "405, cheetah, cheetah, car, CHEETAH, CHEETAH, null, executive, 5, "
            "0, 0, -1, 0.68, 0.68, 0   ,,,,   \n"
            "406, ambulan, ambulan, car, AMBULAN, AMBULAN, van, ignore, 10, 0, "
            "0, -1, 0.864, 0.864, -1\r\r\r\n"
            "407, leviathn, leviathn, heli, LEVIATHN, LEVIATH, null, ignore, "
            "5, 0, 0, -1, 0.54, 0.4, -1\n"
            "# Empty line above\n"
            "408    moonbeam,  moonbeam, car, MOONBEAM  MOONBM, null, normal, "
            "10, 0, 0, -1, 0.7, 0.7, 0\n"
            "   \n"
            "409,,,, othercar,,,, esperant, car, ESPERANT, ESPERAN, null, "
            "normal, 10, 0, 0, -1, 0.64, 0.64, 0\n"
            "end");

    CHECK(diags.empty());
    CHECK(table.size() == 10);

    // Test all models were parsed correctly despite whitespace/delimiter edge
    // cases
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "BRAVURA", 401);
    expect_model(table, "ESPERANT", 402);
    expect_model(table, "PONY", 403);
    expect_model(table, "MULE", 404);
    expect_model(table, "CHEETAH", 405);
    expect_model(table, "AMBULAN", 406);
    expect_model(table, "LEVIATHN", 407);
    expect_model(table, "MOONBEAM", 408);
    expect_model(table, "OTHERCAR", 409);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with builder chaining")
{
    auto source1 = make_source(R"(cars
400, landstal, ...
401, bravura, ...
end)");

    auto source2 = make_source(R"(weap
321, gun_dildo1, ...
322, gun_dildo2, ...
end)");

    auto builder = gta3sc::ModelTable::Builder(&arena);

    builder = gta3sc::config::load_models_from_ide(source1, false, diagman,
                                                   std::move(builder));

    builder = gta3sc::config::load_models_from_ide(source2, false, diagman,
                                                   std::move(builder));

    auto table = std::move(builder).build();

    CHECK(diags.empty());
    CHECK(table.size() == 4);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO2", 322);
}

TEST_CASE_FIXTURE(ModelsIdeTestFixture,
                  "load_models_from_ide with section edge cases")
{
    auto table = load_models(R"(cars
400, landstal, ...
end
end # Ends without section have no effect
end
end
weap
321, gun_dildo1, ...
end
end
objs
14531, int_zerosrc01, ...
end
9999, model_without_section, ...  # ignored
end
cars
401, bravura, ...
# missing end)");

    CHECK(diags.empty());
    CHECK(table.size() == 4);

    // All valid models should be loaded despite section state machine edge
    // cases
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "INT_ZEROSRC01", 14531);
    expect_model(table, "BRAVURA", 401);
    expect_no_model(table, "MODEL_WITHOUT_SECTION");
}

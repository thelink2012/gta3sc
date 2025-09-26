#include "models-fixture.hpp"
#include <gta3sc/config/models.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/util/arena.hpp>
using namespace std::string_view_literals;
using namespace gta3sc::test::config;

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with allocator overload")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
            root_test_dir, root_test_dir / "level.dat", false, sourceman,
            diagman, gta3sc::ArenaAllocator<>(&arena));

    CHECK(diags.empty());
    CHECK(table.size() == 1);
    expect_model(table, "LANDSTAL", 400);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with non existent file path")
{
    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, "nonexistent_level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with multiple IDE files")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE data/objects.ide
IDE data/weapons.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("data/weapons.ide", R"(weap
321, gun_dildo1, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level loads IDEs relative to root path")
{
    create_test_file("data/deep/level.dat", R"(IDE data/vehicles.ide
IDE something/objects.ide
IDE weapons.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("something/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("weapons.ide", R"(weap
321, gun_dildo1, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "data/deep/level.dat",
                         false, sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(
        ModelsTestFixture,
        "load_models_from_level with mixed valid and invalid IDE paths")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE data/nonexistent.ide
IDE data/objects.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    // Should have 1 diagnostic for the nonexistent IDE file
    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);

    // Should still load models from valid IDE files
    CHECK(table.size() == 2);
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with entries different from IDE")
{
    create_test_file("level.dat", R"(
IDE data/vehicles.ide

IPL data/something/else/placement.ipl
IMG example.img
COLFILE example.col

SPLASH loadsc2

IDE data/objects.ide)");

    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    REQUIRE(diags.empty());

    CHECK(table.size() == 2);
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(
        ModelsTestFixture,
        "load_models_from_level with file path - success with objs_only set")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE data/objects.ide
IDE data/weapons.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("data/weapons.ide", R"(weap
321, gun_dildo1, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", true,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 1);

    // Should only have objs models, not cars or weap
    expect_no_model(table, "LANDSTAL");
    expect_no_model(table, "GUN_DILDO1");
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(
        ModelsTestFixture,
        "load_models_from_level with file path - success with objs_only unset")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE data/objects.ide
IDE data/weapons.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("data/weapons.ide", R"(weap
321, gun_dildo1, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 3);

    // Should have all models from all sections
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with empty level file")
{
    create_test_file("level.dat", "");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with only comments and whitespace")
{
    create_test_file("level.dat", R"(# This is a comment
   # Another comment with leading whitespace
   
   # Multiple empty lines
   
   # More comments
   )");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 0);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with malformed IDE lines")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE
IDE 
IDE data/objects.ide
IDE   
IDE data/weapons.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("data/weapons.ide", R"(weap
321, gun_dildo1, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    // Skips empty IDE lines
    REQUIRE(diags.empty());

    // Should still load models from valid IDE files
    CHECK(table.size() == 3);
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with buffer overflow edge cases")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
IDE data/this_is_a_very_long_ide_file_name_that_exceeds_the_512_character_buffer_limit_for_level_file_parsing_but_should_still_be_processed_correctlyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.ide
IDE data/objects.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    // Should have 1 diagnostic for the long filename that doesn't exist
    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);

    // Should still load models from valid IDE files
    CHECK(table.size() == 2);
    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "INT_ZEROSRC01", 14531);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with line parsing edge cases")
{
    create_test_file("level.dat", R"(IDE data/vehicles.ide
   # Comment with leading whitespace
IDE data/objects.ide
# Multiple consecutive comments
# Another comment
IDE data/weapons.ide
   ,,,,   ,,,,   IDE data/peds.ide
		IDE data/hier.ide
IDE data/2dfx.ide   ,,,,   
IDE data/txdp.ide     

IDE data/hand.ide
   )");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");
    create_test_file("data/weapons.ide", R"(weap
321, gun_dildo1, ...
end)");
    create_test_file("data/peds.ide", R"(peds
1, player, ...
end)");
    create_test_file("data/hier.ide", R"(hier
1000, csplay, ...
end)");
    create_test_file("data/2dfx.ide", R"(2dfx
3000, effect_light, ...
end)");
    create_test_file("data/txdp.ide", R"(txdp
4000, texture_dict, ...
end)");
    create_test_file("data/hand.ide", R"(hand
5000, hand_model, ...
end)");

    auto table = gta3sc::config::load_models_from_level(
                         root_test_dir, root_test_dir / "level.dat", false,
                         sourceman, diagman,
                         gta3sc::ModelTable::Builder(&arena))
                         .build();

    CHECK(diags.empty());
    CHECK(table.size() == 8);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "INT_ZEROSRC01", 14531);
    expect_model(table, "GUN_DILDO1", 321);
    expect_model(table, "PLAYER", 1);
    expect_model(table, "CSPLAY", 1000);
    expect_model(table, "EFFECT_LIGHT", 3000);
    expect_model(table, "TEXTURE_DICT", 4000);
    expect_model(table, "HAND_MODEL", 5000);
}

TEST_CASE_FIXTURE(ModelsTestFixture,
                  "load_models_from_level with builder chaining")
{
    create_test_file("level1.dat", R"(IDE data/vehicles.ide)");
    create_test_file("level2.dat", R"(IDE data/objects.ide)");
    create_test_file("data/vehicles.ide", R"(cars
400, landstal, ...
end)");
    create_test_file("data/objects.ide", R"(objs
14531, int_zerosrc01, ...
end)");

    auto builder = gta3sc::ModelTable::Builder(&arena);

    builder = gta3sc::config::load_models_from_level(
            root_test_dir, root_test_dir / "level1.dat", false, sourceman,
            diagman, std::move(builder));

    builder = gta3sc::config::load_models_from_level(
            root_test_dir, root_test_dir / "level2.dat", false, sourceman,
            diagman, std::move(builder));

    auto table = std::move(builder).build();

    CHECK(diags.empty());
    CHECK(table.size() == 2);

    expect_model(table, "LANDSTAL", 400);
    expect_model(table, "INT_ZEROSRC01", 14531);
}
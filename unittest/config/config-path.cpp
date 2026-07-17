#include "../scoped-env-var.hpp"
#include "../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/config/config-path.hpp>

using gta3sc::test::ScopedEnvVar;
using gta3sc::test::WithTempDirFixture;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

namespace
{
constexpr auto config_root_env = "GTA3SC_CONFIG_ROOT"sv;
constexpr auto home_env = "HOME"sv;
} // namespace

TEST_SUITE("config_search_paths")
{
    TEST_CASE("config_search_paths returns exe-adjacent config/ as first "
              "candidate")
    {
        auto paths = gta3sc::config::config_search_paths("/opt/gta3sc",
                                                         "/home/alice");

        REQUIRE_FALSE(paths.empty());
        CHECK(paths[0] == fs::path("/opt/gta3sc/config"));
    }

#if !defined(_WIN32)
    TEST_CASE("config_search_paths returns home-local path as second candidate")
    {
        auto paths = gta3sc::config::config_search_paths("/opt/gta3sc",
                                                         "/home/alice");

        REQUIRE(paths.size() >= 2);
        CHECK(paths[1] == fs::path("/home/alice/.local/share/gta3sc/config"));
    }

    TEST_CASE("config_search_paths returns system path as third candidate")
    {
        auto paths = gta3sc::config::config_search_paths("/opt/gta3sc",
                                                         "/home/alice");

        REQUIRE(paths.size() >= 3);
        CHECK(paths[2] == fs::path("/usr/share/gta3sc/config"));
    }

    TEST_CASE("config_search_paths returns exactly three candidates on "
              "non-Windows")
    {
        auto paths = gta3sc::config::config_search_paths("/opt/gta3sc",
                                                         "/home/alice");
        CHECK(paths.size() == 3);
    }
#else
    TEST_CASE("config_search_paths returns exactly one candidate on Windows")
    {
        auto paths = gta3sc::config::config_search_paths("C:\\gta3sc",
                                                         "C:\\Users\\bob");
        CHECK(paths.size() == 1);
        CHECK(paths[0] == fs::path("C:\\gta3sc\\config"));
    }
#endif
}

#if !defined(_WIN32)
TEST_SUITE("find_config_root discovery")
{
    TEST_CASE_FIXTURE(
            WithTempDirFixture,
            "find_config_root discovers home-local config when HOME is set")
    {
        ScopedEnvVar unset_root(config_root_env, std::nullopt);
        ScopedEnvVar home(home_env, root_test_dir.string());

        const auto expected = root_test_dir / ".local/share/gta3sc/config";
        fs::create_directories(expected);

        auto root = gta3sc::config::find_config_root();
        REQUIRE(root.has_value());
        CHECK(*root == expected);
    }
}
#endif

TEST_SUITE("find_config_root with GTA3SC_CONFIG_ROOT")
{
    TEST_CASE_FIXTURE(
            WithTempDirFixture,
            "find_config_root returns env path when set to valid directory")
    {
        ScopedEnvVar env(config_root_env, root_test_dir.string());

        auto root = gta3sc::config::find_config_root();
        REQUIRE(root.has_value());
        CHECK(*root == root_test_dir);
    }

    TEST_CASE_FIXTURE(
            WithTempDirFixture,
            "find_config_root returns nullopt when env path does not exist")
    {
        ScopedEnvVar env(config_root_env, (root_test_dir / "missing").string());

        CHECK_FALSE(gta3sc::config::find_config_root().has_value());
    }

    TEST_CASE("find_config_root returns nullopt when env is empty string")
    {
        ScopedEnvVar env(config_root_env, "");

        CHECK_FALSE(gta3sc::config::find_config_root().has_value());
    }
}

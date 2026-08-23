#include "../scoped-env-var.hpp"
#include "../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <format>
#include <gta3sc/cli/run.hpp>
#include <gta3sc/cli/version.hpp>
#include <gta3sc/config/config-path.hpp>
#include <sstream>
#include <string_view>
#include <vector>

using gta3sc::test::ScopedEnvVar;
using gta3sc::test::WithTempDirFixture;

namespace
{
constexpr std::string_view config_root_env = "GTA3SC_CONFIG_ROOT";

auto call_run(std::vector<const char*> extra_args, std::ostream& out,
              std::ostream& err) -> int
{
    std::vector<const char*> argv = {"gta3sc"};
    argv.insert(argv.end(), extra_args.begin(), extra_args.end());
    auto argc = static_cast<int>(argv.size());
    return gta3sc::cli::run(argc, const_cast<char**>(argv.data()), out, err);
}
} // namespace

TEST_SUITE("run")
{
    TEST_CASE("run with no arguments prints help and returns EXIT_SUCCESS")
    {
        std::ostringstream out, err;
        int code = gta3sc::cli::run(0, nullptr, out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str().contains("Usage"));
        CHECK(err.str().empty());
    }
}

TEST_SUITE("split_action routing")
{
    TEST_CASE("--help at argv[1] returns help output and EXIT_SUCCESS")
    {
        std::ostringstream out, err;
        int code = call_run({"--help"}, out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str().contains("Usage"));
        CHECK(err.str().empty());
    }

    TEST_CASE("-h at argv[1] returns help output and EXIT_SUCCESS")
    {
        std::ostringstream out, err;
        int code = call_run({"-h"}, out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str().contains("Usage"));
    }

    TEST_CASE("--version at argv[1] returns version output and EXIT_SUCCESS")
    {
        std::ostringstream out, err;
        int code = call_run({"--version"}, out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str() == std::format("gta3sc {}\n", gta3sc::cli::version()));
        CHECK(err.str().empty());
    }

    TEST_CASE("-v at argv[1] returns version output and EXIT_SUCCESS")
    {
        std::ostringstream out, err;
        int code = call_run({"-v"}, out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str() == std::format("gta3sc {}\n", gta3sc::cli::version()));
    }

    TEST_CASE("compile subcommand at argv[1] routes to compile (fails with no "
              "input)")
    {
        std::ostringstream out, err;
        int code = call_run({"compile"}, out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("error"));
    }

    TEST_CASE("no subcommand defaults to compile (fails with no input)")
    {
        std::ostringstream out, err;
        int code = call_run({}, out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("error"));
    }

    TEST_CASE_FIXTURE(
            WithTempDirFixture,
            "leading flag at argv[1] defaults to compile without consuming it")
    {
        std::filesystem::create_directory(root_test_dir);
        ScopedEnvVar env(config_root_env, root_test_dir.string());

        // "--config=gta3" is not a subcommand token, so split_action routes it
        std::ostringstream out, err;
        int code = call_run({"--config=gta3"}, out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("no input file"));
    }

    TEST_CASE("run returns EXIT_FAILURE when config root is absent")
    {
        ScopedEnvVar env(config_root_env, "");

        std::ostringstream out, err;
        int code = call_run({"main.sc", "--config=gta3"}, out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("could not find config/ directory"));
    }
}

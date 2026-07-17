#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/cli/option-parser.hpp>
#include <gta3sc/cli/run-compile.hpp>
#include <sstream>
#include <vector>

using gta3sc::cli::CompileOptions;
using gta3sc::cli::OptionParser;
using gta3sc::cli::parse_compile_options;
using gta3sc::cli::run_compile;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

TEST_SUITE("parse_compile_options")
{
    TEST_CASE("sets input from positional argument")
    {
        std::string_view args[] = {"main.sc"};
        OptionParser parser{args};

        CompileOptions opts;
        auto m = parse_compile_options(parser, opts);

        CHECK(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(opts.input == "main.sc");
        CHECK_FALSE(parser.failed());
        CHECK(parser.eof());
    }

    TEST_CASE("sets output from -o")
    {
        auto args = GENERATE(std::vector{"-o"sv, "out.scm"sv},
                             std::vector{"-oout.scm"sv});
        OptionParser parser{args};

        CompileOptions opts;
        auto m = parse_compile_options(parser, opts);

        CHECK(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(opts.output == "out.scm");
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("sets config name from --config")
    {
        auto args = GENERATE(std::vector{"--config=gta3"sv},
                             std::vector{"--config"sv, "gta3"sv});
        OptionParser parser{args};

        CompileOptions opts;
        auto m = parse_compile_options(parser, opts);

        CHECK(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(opts.config.name == "gta3");
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("leaves unrecognized argument unconsumed")
    {
        std::string_view args[] = {"--unknown-flag"};
        OptionParser parser{args};

        CompileOptions opts;
        auto m = parse_compile_options(parser, opts);

        CHECK_FALSE(static_cast<bool>(m));
        CHECK_FALSE(parser.failed());
        CHECK(parser.peek() == "--unknown-flag"sv);
    }

    TEST_CASE("parse_compile_options records failure when valued option has "
              "no following value")
    {
        auto args = GENERATE(std::vector{"-o"sv}, std::vector{"--config"sv});
        OptionParser parser{args};

        CompileOptions opts;
        auto m = parse_compile_options(parser, opts);

        CHECK(static_cast<bool>(m));
        CHECK_FALSE(m.value.has_value());
        CHECK(parser.failed());
    }
}

TEST_SUITE("run_compile validation")
{
    TEST_CASE("run_compile returns EXIT_FAILURE when no input file given")
    {
        std::ostringstream out, err;
        int code = run_compile(std::span<const std::string_view>{},
                               fs::path("/some/root"), out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("no input file"));
    }

    TEST_CASE("run_compile returns EXIT_FAILURE when --config not given")
    {
        std::string_view args[] = {"main.sc"};
        std::ostringstream out, err;
        int code = run_compile(std::span<const std::string_view>(args),
                               fs::path("/some/root"), out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("no game config"));
    }

    TEST_CASE("run_compile --help short-circuits to help output")
    {
        std::string_view args[] = {"--help"};
        std::ostringstream out, err;
        int code = run_compile(std::span<const std::string_view>(args),
                               fs::path("/some/root"), out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str().contains("Usage"));
        CHECK(err.str().empty());
    }

    TEST_CASE("run_compile --version short-circuits to version output")
    {
        std::string_view args[] = {"--version"};
        std::ostringstream out, err;
        int code = run_compile(std::span<const std::string_view>(args),
                               fs::path("/some/root"), out, err);

        CHECK(code == EXIT_SUCCESS);
        CHECK(out.str().contains("gta3sc"));
        CHECK(err.str().empty());
    }

    TEST_CASE("run_compile rejects unrecognized argument")
    {
        std::string_view args[] = {"--bogus"};
        std::ostringstream out, err;
        int code = run_compile(std::span<const std::string_view>(args),
                               fs::path("/some/root"), out, err);

        CHECK(code == EXIT_FAILURE);
        CHECK(err.str().contains("unrecognized"));
    }
}

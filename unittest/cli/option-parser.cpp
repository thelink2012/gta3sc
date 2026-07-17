#include <doctest/doctest.h>
#include <gta3sc/cli/option-parser.hpp>

using gta3sc::cli::OptionMatch;
using gta3sc::cli::OptionParser;
using namespace std::string_view_literals;

TEST_SUITE("OptionParser cursor basics")
{
    TEST_CASE("eof is true on empty args")
    {
        OptionParser parser{std::span<const std::string_view>{}};
        CHECK(parser.eof());
    }

    TEST_CASE("eof is false when args remain")
    {
        std::string_view args[] = {"foo"sv};
        OptionParser parser{args};
        CHECK_FALSE(parser.eof());
    }

    TEST_CASE("peek returns next token without consuming")
    {
        std::string_view args[] = {"foo"sv, "bar"sv};
        OptionParser parser{args};

        CHECK(parser.peek() == "foo"sv);
        CHECK(parser.peek() == "foo"sv); // second peek unchanged
    }

    TEST_CASE("peek returns nullopt when eof")
    {
        OptionParser parser{std::span<const std::string_view>{}};
        CHECK(parser.peek() == std::nullopt);
    }

    TEST_CASE("next consumes tokens in order")
    {
        std::string_view args[] = {"foo"sv, "bar"sv, "foobar"sv};
        OptionParser parser{args};

        CHECK(parser.next() == "foo"sv);
        CHECK(parser.next() == "bar"sv);
        CHECK(parser.next() == "foobar"sv);
        CHECK(parser.next() == std::nullopt);
        CHECK(parser.eof());
    }
}

TEST_SUITE("OptionParser::positional")
{
    TEST_CASE("positional consumes non-flag token")
    {
        std::string_view args[] = {"main.sc"sv};
        OptionParser parser{args};

        auto tok = parser.positional();
        REQUIRE(tok.has_value());
        CHECK(*tok == "main.sc"sv);
        CHECK(parser.eof());
    }

    TEST_CASE("positional returns nullopt when next token starts with dash")
    {
        std::string_view args[] = {"-o"sv, "out.scm"sv};
        OptionParser parser{args};

        CHECK(parser.positional() == std::nullopt);
        CHECK(parser.peek() == "-o"sv); // not consumed
    }

    TEST_CASE("positional returns nullopt when eof")
    {
        OptionParser parser{std::span<const std::string_view>{}};
        CHECK(parser.positional() == std::nullopt);
    }
}

TEST_SUITE("OptionParser::option")
{
    TEST_CASE("option matches short form")
    {
        std::string_view args[] = {"-h"sv};
        OptionParser parser{args};

        auto m = parser.option("-h"sv, "--help"sv);
        REQUIRE(m.has_value());
        CHECK(*m == true);
        CHECK(parser.eof());
    }

    TEST_CASE("option matches long form")
    {
        std::string_view args[] = {"--help"sv};
        OptionParser parser{args};

        auto m = parser.option("-h"sv, "--help"sv);
        REQUIRE(m.has_value());
        CHECK(*m == true);
        CHECK(parser.eof());
    }

    TEST_CASE("option does not match unrelated token")
    {
        std::string_view args[] = {"--version"sv};
        OptionParser parser{args};

        auto m = parser.option("-h"sv, "--help"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "--version"sv); // not consumed
    }

    TEST_CASE("option with empty short does not match short token")
    {
        std::string_view args[] = {"-h"sv};
        OptionParser parser{args};

        auto m = parser.option(""sv, "--help"sv);
        CHECK(m == std::nullopt);
    }

    TEST_CASE("option with empty long does not match long token")
    {
        std::string_view args[] = {"--help"sv};
        OptionParser parser{args};

        auto m = parser.option("-h"sv, ""sv);
        CHECK(m == std::nullopt);
    }
}

TEST_SUITE("OptionParser::toggle")
{
    TEST_CASE("toggle matches positive -f form")
    {
        std::string_view args[] = {"-fdebug"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        REQUIRE(m.has_value());
        CHECK(*m == true);
        CHECK(parser.eof());
    }

    TEST_CASE("toggle matches negative -fno- form")
    {
        std::string_view args[] = {"-fno-debug"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        REQUIRE(m.has_value());
        CHECK(*m == false);
        CHECK(parser.eof());
    }

    TEST_CASE("toggle matches positive -m form")
    {
        std::string_view args[] = {"-march"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-march"sv);
        REQUIRE(m.has_value());
        CHECK(*m == true);
        CHECK(parser.eof());
    }

    TEST_CASE("toggle matches negative -mno- form")
    {
        std::string_view args[] = {"-mno-arch"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-march"sv);
        REQUIRE(m.has_value());
        CHECK(*m == false);
        CHECK(parser.eof());
    }

    TEST_CASE("toggle does not match partial name")
    {
        std::string_view args[] = {"-fdebugger"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "-fdebugger"sv); // not consumed
    }

    TEST_CASE("toggle does not match unrelated flag name")
    {
        std::string_view args[] = {"-fother"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "-fother"sv); // not consumed
    }

    TEST_CASE("toggle does not match different letter prefix")
    {
        std::string_view args[] = {"-xdebug"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "-xdebug"sv); // not consumed
    }

    TEST_CASE("toggle does not match token without letter prefix")
    {
        std::string_view args[] = {"--fdebug"sv};
        OptionParser parser{args};

        auto m = parser.toggle("-fdebug"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "--fdebug"sv); // not consumed
    }
}

TEST_SUITE("OptionParser::flag")
{
    TEST_CASE("flag matches set-only -f form")
    {
        std::string_view args[] = {"-fverbose"sv};
        OptionParser parser{args};

        auto m = parser.flag("-fverbose"sv);
        REQUIRE(m.has_value());
        CHECK(*m == true);
        CHECK(parser.eof());
    }

    TEST_CASE("flag does not match -fno- form")
    {
        std::string_view args[] = {"-fno-verbose"sv};
        OptionParser parser{args};

        auto m = parser.flag("-fverbose"sv);
        CHECK(m == std::nullopt);
        CHECK(parser.peek() == "-fno-verbose"sv); // not consumed
    }
}

TEST_SUITE("OptionParser::value")
{
    TEST_CASE("value option accepts short form with space")
    {
        std::string_view args[] = {"-o"sv, "out.scm"sv};
        OptionParser parser{args};

        auto m = parser.value("-o"sv, ""sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == "out.scm"sv);
        CHECK(parser.eof());
    }

    TEST_CASE("value option accepts short form concatenated")
    {
        std::string_view args[] = {"-oout.scm"sv};
        OptionParser parser{args};

        auto m = parser.value("-o"sv, ""sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == "out.scm"sv);
        CHECK(parser.eof());
    }

    TEST_CASE("value option accepts long form with equals")
    {
        std::string_view args[] = {"--config=gta3"sv};
        OptionParser parser{args};

        auto m = parser.value(""sv, "--config"sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == "gta3"sv);
        CHECK(parser.eof());
    }

    TEST_CASE("value option accepts long form with space")
    {
        std::string_view args[] = {"--config"sv, "gta3"sv};
        OptionParser parser{args};

        auto m = parser.value(""sv, "--config"sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == "gta3"sv);
        CHECK(parser.eof());
    }

    TEST_CASE("value option does not consume on no match")
    {
        std::string_view args[] = {"--other=val"sv};
        OptionParser parser{args};

        auto m = parser.value(""sv, "--config"sv);

        CHECK_FALSE(static_cast<bool>(m));
        CHECK(parser.peek() == "--other=val"sv);
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("value option records failure when short form has no following "
              "value")
    {
        std::string_view args[] = {"-o"sv};
        OptionParser parser{args};

        auto m = parser.value("-o"sv, ""sv);

        CHECK(static_cast<bool>(m));      // name matched
        CHECK_FALSE(m.value.has_value()); // but no value
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
    }

    TEST_CASE("value option records failure when long form has no following "
              "value")
    {
        std::string_view args[] = {"--config"sv};
        OptionParser parser{args};

        auto m = parser.value(""sv, "--config"sv);

        CHECK(static_cast<bool>(m));      // name matched
        CHECK_FALSE(m.value.has_value()); // but no value
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
    }
}

TEST_SUITE("OptionParser::value_int")
{
    TEST_CASE("value_int parses integer via --opt=N form")
    {
        std::string_view args[] = {"--count=42"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--count"sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == 42);
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("value_int parses integer via --opt N form")
    {
        std::string_view args[] = {"--count"sv, "7"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--count"sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == 7);
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("value_int fails on non-integer value")
    {
        std::string_view args[] = {"--count=abc"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--count"sv);

        CHECK(static_cast<bool>(m));      // name matched
        CHECK_FALSE(m.value.has_value()); // but parsing failed
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
    }

    TEST_CASE("value_int fails on missing value")
    {
        std::string_view args[] = {"--count"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--count"sv);

        CHECK(static_cast<bool>(m));
        CHECK_FALSE(m.value.has_value());
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
    }

    TEST_CASE("value_int fails on overflow for int")
    {
        // One more than INT_MAX
        std::string_view args[] = {"--n=2147483648"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--n"sv);

        CHECK(static_cast<bool>(m));
        CHECK_FALSE(m.value.has_value());
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
    }

    TEST_CASE("value_int does not consume on no match")
    {
        std::string_view args[] = {"--other=5"sv};
        OptionParser parser{args};

        auto m = parser.value_int<int>("--count"sv);

        CHECK_FALSE(static_cast<bool>(m));
        CHECK(parser.peek() == "--other=5"sv);
        CHECK_FALSE(parser.failed());
    }
}

TEST_SUITE("OptionParser fail and error state")
{
    TEST_CASE("failed is false by default")
    {
        std::string_view args[] = {"a"sv};
        OptionParser parser{args};
        CHECK_FALSE(parser.failed());
    }

    TEST_CASE("fail sets error message and failed flag")
    {
        std::string_view args[] = {"a"sv};
        OptionParser parser{args};
        parser.fail("something went wrong");

        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
        CHECK(parser.error() == "something went wrong"sv);
    }

    TEST_CASE("fail overwrites previous error")
    {
        std::string_view args[] = {"a"sv};
        OptionParser parser{args};
        parser.fail("first error");
        parser.fail("second error");

        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
        CHECK(parser.error() == "second error"sv);
    }

    TEST_CASE("matchers keep working after a prior failure")
    {
        std::string_view args[] = {"-o"sv, "out.scm"sv};
        OptionParser parser{args};
        parser.fail("earlier error");

        auto m = parser.value("-o"sv, ""sv);

        REQUIRE(static_cast<bool>(m));
        REQUIRE(m.value.has_value());
        CHECK(*m.value == "out.scm"sv);
        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
        CHECK(parser.error() == "earlier error"sv);
    }

    TEST_CASE("fail formats message from arguments")
    {
        std::string_view args[] = {"a"sv};
        OptionParser parser{args};
        parser.fail("option '{}' requires an argument", "--config"sv);

        CHECK(parser.failed());
        CHECK_FALSE(parser.error().empty());
        CHECK(parser.error() == "option '--config' requires an argument"sv);
    }
}

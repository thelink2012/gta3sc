#include "../../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>

using namespace gta3sc::test;
using gta3sc::filesystem::RelativeInsensitivePathResolver;

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "case mismatch resolves to actual on-disk path")
{
    create_test_file("data/maps/foo.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("DATA/MAPS/foo.IDE");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/maps/foo.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "backslash separators are accepted")
{
    create_test_file("data/maps/bar.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto result = resolver.resolve(R"(DATA\MAPS\bar.IDE)");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/maps/bar.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "mixed backslash and forward slash both work")
{
    create_test_file("data/maps/mixed.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto result = resolver.resolve(R"(DATA\maps/mixed.IDE)");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/maps/mixed.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "root path dot component is canonicalized")
{
    create_test_file("data/test.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir / ".");
    auto result = resolver.resolve("data/test.ide");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/test.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "missing component returns nullopt")
{
    std::filesystem::create_directories(root_test_dir / "data");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    CHECK(!resolver.resolve("data/maps/foo.ide").has_value());
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "resolves multiple files in the same directory")
{
    create_test_file("data/maps/a.ide", "");
    create_test_file("data/maps/b.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);

    auto r1 = resolver.resolve("DATA/MAPS/a.IDE");
    auto r2 = resolver.resolve("DATA/MAPS/b.IDE");

    REQUIRE(r1.has_value());
    REQUIRE(r2.has_value());
    CHECK(*r1 == root_test_dir / "data/maps/a.ide");
    CHECK(*r2 == root_test_dir / "data/maps/b.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "single component resolves to on-disk name")
{
    create_test_file("solo.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("Solo.IDE");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "solo.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "casing at each segment resolves to on-disk path")
{
    create_test_file("A/b/C/d/File.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("a/B/c/D/file.IDE");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "A/b/C/d/File.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "failed lookup does not corrupt cache")
{
    create_test_file("data/maps/exists.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);

    CHECK(!resolver.resolve("DATA/MAPS/missing.ide").has_value());

    auto result = resolver.resolve("DATA/MAPS/exists.IDE");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/maps/exists.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "nonexistent root returns nullopt")
{
    RelativeInsensitivePathResolver resolver(root_test_dir / "no_such_subdir");

    CHECK(!resolver.resolve("anything").has_value());
}

TEST_CASE_FIXTURE(WithTempDirFixture, "double slash skips empty segments")
{
    create_test_file("data/maps/foo.ide", "");

    RelativeInsensitivePathResolver resolver(root_test_dir);
    auto doubled = resolver.resolve("DATA//MAPS/foo.ide");
    auto plain = resolver.resolve("DATA/MAPS/foo.ide");

    REQUIRE(doubled.has_value());
    REQUIRE(plain.has_value());
    CHECK(*doubled == *plain);
}

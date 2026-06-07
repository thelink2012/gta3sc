#include "../../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>

using namespace gta3sc::test;
using gta3sc::filesystem::RelativePathResolver;

TEST_CASE_FIXTURE(WithTempDirFixture, "joins relative path under root")
{
    RelativePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("data/maps/foo.ide");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/maps/foo.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "result need not exist on disk")
{
    RelativePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("does/not/exist.ide");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "does/not/exist.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "empty string returns root")
{
    RelativePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "root path dot component is canonicalized")
{
    RelativePathResolver resolver(root_test_dir / ".");
    auto result = resolver.resolve("data/foo.ide");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "data/foo.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "path is joined lexically, case is preserved")
{
    RelativePathResolver resolver(root_test_dir);
    auto result = resolver.resolve("Data/CASEd.ide");

    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "Data/CASEd.ide");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "nonexistent root still joins lexically")
{
    const auto bogus_root = root_test_dir / "no_such_subdir";

    RelativePathResolver resolver(bogus_root);
    auto result = resolver.resolve("any/file.ide");

    REQUIRE(result.has_value());
    CHECK(*result == bogus_root / "any/file.ide");
}

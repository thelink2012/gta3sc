#include "../../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>

using namespace gta3sc::test;
using gta3sc::filesystem::ScriptPathResolver;

TEST_CASE_FIXTURE(WithTempDirFixture, "scan_directory return value")
{
    ScriptPathResolver resolver;

    CHECK(!resolver.scan_directory(root_test_dir / "nonexistent"));
    CHECK(resolver.scan_directory(root_test_dir));
}

TEST_CASE_FIXTURE(WithTempDirFixture, "follows directory symlinks")
{
    // real/ is outside scripts/ so mission.sc is only reachable via the
    // symlink.
    const auto scripts_dir = root_test_dir / "scripts";
    const auto real_dir = root_test_dir / "real";
    const auto link_dir = scripts_dir / "link";

    std::filesystem::create_directories(scripts_dir);
    std::filesystem::create_directories(real_dir);
    create_test_file("real/mission.sc", "");

    std::error_code ec;
    std::filesystem::create_directory_symlink(real_dir, link_dir, ec);
    if(ec)
    {
        // TODO: Revisit. Consider Windows directory junctions (no privilege
        // needed) as a cross-platform alternative.
        WARN("directory symlink creation failed; test skipped"
             " (enable Developer Mode on Windows)");
        return;
    }

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(scripts_dir));

    const auto result = resolver.resolve("mission.sc");
    REQUIRE(result.has_value());
    CHECK(*result == link_dir / "mission.sc");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "case-insensitive lookup")
{
    create_test_file("MiSSioN.SC", "");

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(root_test_dir));

    CHECK(resolver.resolve("mission.sc") == root_test_dir / "MiSSioN.SC");
    CHECK(resolver.resolve("MISSION.SC") == root_test_dir / "MiSSioN.SC");
}

TEST_CASE_FIXTURE(WithTempDirFixture, "extension filtering")
{
    create_test_file("script0.sc", "");
    create_test_file("script1.Sc", "");
    create_test_file("script2.sC", "");
    create_test_file("script3.SC", "");
    create_test_file("excluded.s", "");
    create_test_file("excluded.scm", "");
    create_test_file("excluded.scx", "");
    create_test_file("excluded.txt", "");
    create_test_file("excluded.sc.bak", "");

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(root_test_dir));

    CHECK(resolver.resolve("script0.sc") == root_test_dir / "script0.sc");
    CHECK(resolver.resolve("script1.Sc") == root_test_dir / "script1.Sc");
    CHECK(resolver.resolve("script2.sC") == root_test_dir / "script2.sC");
    CHECK(resolver.resolve("script3.SC") == root_test_dir / "script3.SC");

    CHECK(!resolver.resolve("excluded.s").has_value());
    CHECK(!resolver.resolve("excluded.scm").has_value());
    CHECK(!resolver.resolve("excluded.scx").has_value());
    CHECK(!resolver.resolve("excluded.txt").has_value());
    CHECK(!resolver.resolve("excluded.sc.bak").has_value());
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "scan_directory recurses into subdirectories")
{
    create_test_file("sub/deep/mission.sc", "");

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(root_test_dir));

    CHECK(resolver.resolve("mission.sc")
          == root_test_dir / "sub" / "deep" / "mission.sc");
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "accumulates across multiple scan_directory calls")
{
    create_test_file("a/foo.sc", "");
    create_test_file("b/bar.sc", "");

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(root_test_dir / "a"));
    REQUIRE(resolver.scan_directory(root_test_dir / "b"));

    CHECK(resolver.resolve("foo.sc") == root_test_dir / "a" / "foo.sc");
    CHECK(resolver.resolve("bar.sc") == root_test_dir / "b" / "bar.sc");
}

TEST_CASE("resolve returns nullopt for unknown file")
{
    ScriptPathResolver resolver;
    CHECK(!resolver.resolve("not_indexed.sc").has_value());
}

TEST_CASE_FIXTURE(WithTempDirFixture,
                  "scan_directory on file path returns false")
{
    create_test_file("plain.txt", "");

    ScriptPathResolver resolver;
    CHECK(!resolver.scan_directory(root_test_dir / "plain.txt"));
}

// TODO verify which file wins in the original miss2.exe
TEST_CASE_FIXTURE(WithTempDirFixture, "duplicate basename first scan wins")
{
    create_test_file("a/foo.sc", "");
    create_test_file("b/foo.sc", "");

    ScriptPathResolver resolver;
    REQUIRE(resolver.scan_directory(root_test_dir / "a"));
    REQUIRE(resolver.scan_directory(root_test_dir / "b"));

    const auto result = resolver.resolve("foo.sc");
    REQUIRE(result.has_value());
    CHECK(*result == root_test_dir / "a" / "foo.sc");
}

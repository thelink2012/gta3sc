#include "../with-diagnostic-fixture.hpp"
#include "../with-temp-dir-fixture.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/util/arena.hpp>
#include <iterator>
#include <vector>
using namespace gta3sc::test;
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto subscript_shell = "MISSION_START\nMISSION_END\n"sv;

auto collect_file_labels(const gta3sc::LinkedIR<gta3sc::ParserIR>& ir)
        -> std::vector<std::string>
{
    std::vector<std::string> labels;
    for(const auto& line : ir)
    {
        if(line.has_label() && line.label().name().starts_with("@@"))
            labels.emplace_back(line.label().name());
    }
    return labels;
}

auto size(const gta3sc::LinkedIR<gta3sc::ParserIR>& ir)
{
    return std::distance(ir.begin(), ir.end());
}
} // namespace

namespace gta3sc::test::syntax
{
class MultifileParserFixture
    : public WithTempDirFixture
    , public WithDiagnosticFixture
{
public:
    gta3sc::ArenaMemoryResource arena;

protected:
    gta3sc::SourceManager sourceman;
    gta3sc::SymbolTable symrepo{gta3sc::ArenaAllocator<>(&arena)};

    auto make_parser(const std::filesystem::path& main_path)
            -> gta3sc::syntax::MultifileParser
    {
        return gta3sc::syntax::MultifileParser(
                main_path, symrepo, sourceman, diagman,
                gta3sc::ArenaAllocator<>(&arena));
    }
};
} // namespace gta3sc::test::syntax

using namespace gta3sc::test::syntax;
using FileType = gta3sc::SymbolTable::FileType;

TEST_CASE_FIXTURE(MultifileParserFixture, "has_next_file initially true")
{
    create_test_file("main.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");

    CHECK(parser.has_next_file());
}

TEST_CASE_FIXTURE(MultifileParserFixture, "has_next_file false after parse")
{
    create_test_file("main.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");

    CHECK(parser.parse() != std::nullopt);
    CHECK_FALSE(parser.has_next_file());
}

TEST_CASE_FIXTURE(MultifileParserFixture, "empty main file")
{
    create_test_file("main.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().has_label());
    CHECK(ir->front().label().name() == "@@MAIN"sv);
    CHECK_FALSE(ir->front().has_command());

    const auto main_file = symrepo.lookup_file("MAIN");
    REQUIRE(main_file != nullptr);
    CHECK(main_file->type() == FileType::main);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "main file missing")
{
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");

    CHECK(parser.parse() == std::nullopt);
    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_open_file);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "ancillary file missing")
{
    create_test_file("main.sc", "LAUNCH_MISSION sub.sc\n");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");

    CHECK(parser.parse() == std::nullopt);
    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_load_file);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "GOSUB_FILE extension")
{
    create_test_file("main.sc", "GOSUB_FILE label ext.sc\n");
    create_test_file("ext.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@EXT.SC");

    const auto ext_file = symrepo.lookup_file("EXT.SC");
    REQUIRE(ext_file != nullptr);
    CHECK(ext_file->type() == FileType::main_extension);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "LAUNCH_MISSION subscript")
{
    create_test_file("main.sc", "LAUNCH_MISSION sub.sc\n");
    create_test_file("sub.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@SUB.SC");

    const auto sub_file = symrepo.lookup_file("SUB.SC");
    REQUIRE(sub_file != nullptr);
    CHECK(sub_file->type() == FileType::subscript);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "LOAD_AND_LAUNCH_MISSION mission")
{
    create_test_file("main.sc", "LOAD_AND_LAUNCH_MISSION miss.sc\n");
    create_test_file("miss.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@MISS.SC");

    const auto mission_file = symrepo.lookup_file("MISS.SC");
    REQUIRE(mission_file != nullptr);
    CHECK(mission_file->type() == FileType::mission);
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "output order - same type discovery order")
{
    create_test_file("main.sc", "LAUNCH_MISSION a.sc\nLAUNCH_MISSION b.sc\n");
    create_test_file("a.sc", subscript_shell);
    create_test_file("b.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 3);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@A.SC");
    CHECK(labels[2] == "@@B.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "output order - script lists subscript before extension")
{
    create_test_file("main.sc",
                     "LAUNCH_MISSION sub.sc\nGOSUB_FILE label ext.sc\n");
    create_test_file("sub.sc", subscript_shell);
    create_test_file("ext.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 3);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@EXT.SC");
    CHECK(labels[2] == "@@SUB.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "output order - script lists extension before subscript")
{
    create_test_file("main.sc",
                     "GOSUB_FILE label ext.sc\nLAUNCH_MISSION sub.sc\n");
    create_test_file("sub.sc", subscript_shell);
    create_test_file("ext.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 3);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@EXT.SC");
    CHECK(labels[2] == "@@SUB.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "output order - subscript before mission")
{
    create_test_file(
            "main.sc",
            "LAUNCH_MISSION sub.sc\nLOAD_AND_LAUNCH_MISSION miss.sc\n");
    create_test_file("sub.sc", subscript_shell);
    create_test_file("miss.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 3);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@SUB.SC");
    CHECK(labels[2] == "@@MISS.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "unsupported main extension import order")
{
    create_test_file("main.sc", "LAUNCH_MISSION sub.sc\n");
    create_test_file("sub.sc",
                     "MISSION_START\nGOSUB_FILE lbl ext.sc\nMISSION_END\n"sv);
    create_test_file("ext.sc", "");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    CHECK(parser.parse() == std::nullopt);

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::syntax::diag::unsupported_main_extension_import_order);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "unsupported subscript import order")
{
    create_test_file("main.sc", "LOAD_AND_LAUNCH_MISSION miss.sc\n");
    create_test_file("miss.sc",
                     "MISSION_START\nLAUNCH_MISSION sub.sc\nMISSION_END\n"sv);
    create_test_file("sub.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    CHECK(parser.parse() == std::nullopt);

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::syntax::diag::unsupported_subscript_import_order);
}

// TODO: when FileType gains streamed_script (ordered after mission), add a test
// that imports a mission from a streamed script and expect
// diag::unsupported_mission_import_order.

TEST_CASE_FIXTURE(MultifileParserFixture, "duplicate import same type ignored")
{
    create_test_file("main.sc",
                     "LAUNCH_MISSION sub.sc\nLAUNCH_MISSION sub.sc\n");
    create_test_file("sub.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@SUB.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture, "duplicate import different type")
{
    create_test_file("main.sc",
                     "LAUNCH_MISSION dup.sc\nLOAD_AND_LAUNCH_MISSION dup.sc\n");
    create_test_file("dup.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);
    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::syntax::diag::file_already_imported_as_different_type);

    const auto dup_file = symrepo.lookup_file("DUP.SC");
    REQUIRE(dup_file != nullptr);
    CHECK(dup_file->type() == FileType::subscript);

    auto labels = collect_file_labels(*ir);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0] == "@@MAIN");
    CHECK(labels[1] == "@@DUP.SC");
}

TEST_CASE_FIXTURE(MultifileParserFixture, "main parse error")
{
    create_test_file("main.sc", "\"x\"\n");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    CHECK(parser.parse() == std::nullopt);

    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::syntax::diag::expected_command);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "ancillary parse error")
{
    create_test_file("main.sc", "LAUNCH_MISSION sub.sc\n");
    create_test_file("sub.sc", "WAIT 0\n");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    CHECK(parser.parse() == std::nullopt);

    REQUIRE_FALSE(diags.empty());
    CHECK(consume_diag().descriptor
          == &gta3sc::syntax::diag::expected_mission_start_at_top);
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "parse continues after one load failure")
{
    create_test_file("main.sc", "LAUNCH_MISSION a.sc\nLAUNCH_MISSION b.sc\n");
    create_test_file("b.sc", subscript_shell);
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    CHECK(parser.parse() == std::nullopt);

    REQUIRE(diags.size() == 1);
    CHECK(consume_diag().descriptor == &gta3sc::diag::could_not_load_file);
}

TEST_CASE_FIXTURE(MultifileParserFixture, "main with commands")
{
    create_test_file("main.sc", "WAIT 0\n");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");
    auto ir = parser.parse();

    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) >= 2);
    CHECK(ir->front().label().name() == "@@MAIN"sv);

    auto it = ir->begin();
    CHECK_FALSE(it->has_command());
    ++it;
    REQUIRE(it->has_command());
    CHECK(it->command().name() == "WAIT"sv);
}

TEST_CASE_FIXTURE(MultifileParserFixture,
                  "parse_next_file incrementally matches full parse")
{
    create_test_file("main.sc", "WAIT 0\n"
                                "GOSUB_FILE lbl ext_a.sc\n"
                                "LAUNCH_MISSION sub_a.sc\n"
                                "LOAD_AND_LAUNCH_MISSION miss_a.sc\n");
    create_test_file("ext_a.sc", "WAIT 1\n"
                                 "GOSUB_FILE lbl ext_b.sc\n"
                                 "LAUNCH_MISSION sub_b.sc\n");
    create_test_file("ext_b.sc", "WAIT 2\n");
    create_test_file("sub_a.sc", "MISSION_START\n"
                                 "WAIT 3\n"
                                 "LOAD_AND_LAUNCH_MISSION miss_b.sc\n"
                                 "MISSION_END\n");
    create_test_file("sub_b.sc", "MISSION_START\n"
                                 "WAIT 4\n"
                                 "MISSION_END\n");
    create_test_file("miss_a.sc", "MISSION_START\n"
                                  "WAIT 5\n"
                                  "MISSION_END\n");
    create_test_file("miss_b.sc", "MISSION_START\n"
                                  "WAIT 6\n"
                                  "MISSION_END\n");
    sourceman.scan_directory(root_test_dir);

    auto parser = make_parser(root_test_dir / "main.sc");

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@MAIN"sv);

        auto wait = std::next(chunk->begin(), 1);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 0);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@EXT_A.SC"sv);

        auto wait = std::next(chunk->begin(), 1);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 1);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@EXT_B.SC"sv);

        auto wait = std::next(chunk->begin(), 1);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 2);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@SUB_A.SC"sv);

        auto wait = std::next(chunk->begin(), 2);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 3);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@SUB_B.SC"sv);

        auto wait = std::next(chunk->begin(), 2);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 4);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@MISS_A.SC"sv);

        auto wait = std::next(chunk->begin(), 2);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 5);
    }

    REQUIRE(parser.has_next_file());
    {
        auto chunk = parser.parse_next_file();
        REQUIRE(chunk != std::nullopt);

        REQUIRE(chunk->front().has_label());
        CHECK(chunk->front().label().name() == "@@MISS_B.SC"sv);

        auto wait = std::next(chunk->begin(), 2);
        REQUIRE(wait->has_command());
        CHECK(wait->command().name() == "WAIT"sv);
        CHECK(*wait->command().arg(0).as_int() == 6);
    }

    CHECK_FALSE(parser.has_next_file());
}

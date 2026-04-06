#include <doctest/doctest.h>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/visitor/required-files-visitor.hpp>
#include <gta3sc/util/arena.hpp>
#include <vector>
using namespace std::literals::string_view_literals;

using FileType = gta3sc::SymbolTable::FileType;

namespace gta3sc::test::syntax
{
class RequiredFilesVisitorFixture
{
protected:
    struct RequiredFileCallbackRecord
    {
        std::string_view command;
        std::string_view filename;
        gta3sc::SymbolTable::FileType type;
    };

    auto visit_each(const gta3sc::LinkedIR<gta3sc::ParserIR>& ir)
            -> std::vector<RequiredFileCallbackRecord>
    {
        std::vector<RequiredFileCallbackRecord> records;

        gta3sc::syntax::CallbackRequiredFilesVisitor visitor(
                [&](const gta3sc::ParserIR::Command& command,
                    const gta3sc::ParserIR::Argument& arg, FileType type) {
                    records.push_back(
                            {command.name(), *arg.as_filename(), type});
                });

        visitor.visit_each(ir);

        return records;
    }

    gta3sc::ArenaMemoryResource arena;
};
} // namespace gta3sc::test::syntax

using namespace gta3sc::test::syntax;

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "visits empty LinkedIR")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir;
    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "ignores unrelated command")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("WAIT"sv)
                    .arg_int(0)
                    .build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "label-only instruction")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena).label("LABEL"sv).build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "label plus command still visits")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .label("START"sv)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("x.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 1);
    CHECK(records[0].filename == "X.SC"sv);
    CHECK(records[0].type == FileType::subscript);
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "GOSUB_FILE with filename")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("GOSUB_FILE"sv)
                    .arg_ident("LABEL"sv)
                    .arg_filename("file.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 1);
    CHECK(records[0].command == "GOSUB_FILE"sv);
    CHECK(records[0].filename == "FILE.SC"sv);
    CHECK(records[0].type == FileType::main_extension);
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "LAUNCH_MISSION with filename")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("sub.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 1);
    CHECK(records[0].command == "LAUNCH_MISSION"sv);
    CHECK(records[0].filename == "SUB.SC"sv);
    CHECK(records[0].type == FileType::subscript);
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "LOAD_AND_LAUNCH_MISSION with filename")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("LOAD_AND_LAUNCH_MISSION"sv)
                    .arg_filename("miss.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 1);
    CHECK(records[0].command == "LOAD_AND_LAUNCH_MISSION"sv);
    CHECK(records[0].filename == "MISS.SC"sv);
    CHECK(records[0].type == FileType::mission);
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "multiple file commands in order")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("GOSUB_FILE"sv)
                    .arg_ident("LABEL"sv)
                    .arg_filename("a.sc"sv)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("b.sc"sv)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("LOAD_AND_LAUNCH_MISSION"sv)
                    .arg_filename("c.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 3);
    CHECK(records[0].filename == "A.SC"sv);
    CHECK(records[0].type == FileType::main_extension);
    CHECK(records[1].filename == "B.SC"sv);
    CHECK(records[1].type == FileType::subscript);
    CHECK(records[2].filename == "C.SC"sv);
    CHECK(records[2].type == FileType::mission);
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "GOSUB_FILE missing filename argument")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("GOSUB_FILE"sv)
                    .arg_ident("LABEL"sv)
                    .build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "GOSUB_FILE non-filename second argument")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("GOSUB_FILE"sv)
                    .arg_ident("LABEL"sv)
                    .arg_ident("NOT_A_FILE"sv)
                    .build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "LAUNCH_MISSION non-filename argument")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_ident("NOT_A_FILE"sv)
                    .build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture,
                  "LOAD_AND_LAUNCH_MISSION non-filename argument")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("LOAD_AND_LAUNCH_MISSION"sv)
                    .arg_ident("NOT_A_FILE"sv)
                    .build()};

    const auto records = visit_each(ir);

    CHECK(records.empty());
}

TEST_CASE_FIXTURE(RequiredFilesVisitorFixture, "visit with multiple commands")
{
    const gta3sc::LinkedIR<gta3sc::ParserIR> ir{
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("one.sc"sv)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("WAIT"sv)
                    .arg_int(0)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("WAIT"sv)
                    .arg_int(1)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("two.sc"sv)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("WAIT"sv)
                    .arg_int(2)
                    .build(),
            gta3sc::ParserIR::Builder(&arena)
                    .command("LAUNCH_MISSION"sv)
                    .arg_filename("three.sc"sv)
                    .build()};

    const auto records = visit_each(ir);

    REQUIRE(records.size() == 3);
    CHECK(records[0].filename == "ONE.SC"sv);
    CHECK(records[0].type == FileType::subscript);
    CHECK(records[1].filename == "TWO.SC"sv);
    CHECK(records[1].type == FileType::subscript);
    CHECK(records[2].filename == "THREE.SC"sv);
    CHECK(records[2].type == FileType::subscript);
}

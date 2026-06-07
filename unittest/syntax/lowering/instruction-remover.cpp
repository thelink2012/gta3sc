#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/syntax/lowering/instruction-remover.hpp>

using gta3sc::ArenaMemoryResource;
using gta3sc::LinkedIR;
using gta3sc::no_file_range;
using gta3sc::SemaIR;
using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using gta3sc::syntax::InstructionRemover;

namespace
{
class InstructionRemoverExample final : public InstructionRemover
{
public:
    InstructionRemoverExample(const gta3sc::CommandTable& cmdtable,
                              gta3sc::ArenaAllocator<> allocator) noexcept :
        InstructionRemover(allocator)
    {
        add_command(cmdtable.find_command("{"));
        add_command(cmdtable.find_command("}"));
    }
};

class InstructionRemoverFixture : public gta3sc::test::CommandTableFixture
{
public:
    auto make_rewriter() -> InstructionRemoverExample
    {
        return InstructionRemoverExample(cmdman, &arena);
    }

protected:
    ArenaMemoryResource arena;
    SymbolTable symtable{&arena};
};
} // namespace

TEST_CASE_FIXTURE(InstructionRemoverFixture, "unrelated command is not removed")
{
    const auto wait_cmd = cmdman.find_command("WAIT");
    REQUIRE(wait_cmd != nullptr);

    auto rewriter = make_rewriter();
    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).command(*wait_cmd).arg_int(0).build());

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(InstructionRemoverFixture, "label-only line is not removed")
{
    const auto [label, inserted] = symtable.insert_label(
            "LABEL", SymbolTable::global_scope, no_file_range);
    REQUIRE(inserted);

    auto rewriter = make_rewriter();
    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).label(label).build());

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(InstructionRemoverFixture,
                  "unrelated command with label is not removed")
{
    const auto [label, inserted] = symtable.insert_label(
            "LABEL", SymbolTable::global_scope, no_file_range);
    REQUIRE(inserted);

    const auto wait_cmd = cmdman.find_command("WAIT");
    REQUIRE(wait_cmd != nullptr);

    auto rewriter = make_rewriter();
    const auto result = rewriter.visit(*SemaIR::Builder(&arena)
                                                .label(label)
                                                .command(*wait_cmd)
                                                .arg_int(0)
                                                .build());

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(InstructionRemoverFixture,
                  "removed command with label preserves label")
{
    const auto [label, inserted] = symtable.insert_label(
            "LABEL", SymbolTable::global_scope, no_file_range);
    REQUIRE(inserted);

    const auto open_cmd = cmdman.find_command("{");
    REQUIRE(open_cmd != nullptr);

    auto rewriter = make_rewriter();
    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).label(label).command(*open_cmd).build());

    const auto expected = LinkedIR<SemaIR>(
            {SemaIR::create(label, nullptr, &arena)});

    REQUIRE(result);
    REQUIRE(*result == expected);
}

TEST_CASE_FIXTURE(InstructionRemoverFixture,
                  "command in removal set is removed")
{
    const auto open_cmd = cmdman.find_command("{");
    REQUIRE(open_cmd != nullptr);

    auto rewriter = make_rewriter();
    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).command(*open_cmd).build());

    REQUIRE(result);
    REQUIRE(result->empty());
}

TEST_CASE_FIXTURE(InstructionRemoverFixture,
                  "multiple commands in removal set are removed")
{
    auto rewriter = make_rewriter();

    // TODO migrate to GENERATE
    for(const auto cmd_name : {std::string_view{"{"}, std::string_view{"}"}})
    {
        SUBCASE(cmd_name.data())
        {
            const auto cmd = cmdman.find_command(cmd_name);
            REQUIRE(cmd != nullptr);

            const auto result = rewriter.visit(
                    *SemaIR::Builder(&arena).command(*cmd).build());

            REQUIRE(result);
            REQUIRE(result->empty());
        }
    }
}

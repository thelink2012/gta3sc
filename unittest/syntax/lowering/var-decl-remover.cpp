#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/syntax/lowering/var-decl-remover.hpp>

using gta3sc::ArenaMemoryResource;
using gta3sc::no_file_range;
using gta3sc::SemaIR;
using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using gta3sc::syntax::VarDeclRemover;

TEST_CASE_FIXTURE(gta3sc::test::CommandTableFixture,
                  "all var-decl commands are removed")
{
    ArenaMemoryResource arena;
    SymbolTable symtable{&arena};
    
    const auto [dummy_var, inserted] = symtable.insert_var(
            "DUMMY", SymbolTable::global_scope, SymbolTable::VarType::INT,
            std::nullopt, no_file_range);
    REQUIRE(inserted);

    VarDeclRemover rewriter(cmdman, &arena);

    // TODO migrate to GENERATE
    for(const auto cmd_name :
        {std::string_view{"VAR_INT"}, std::string_view{"LVAR_INT"},
         std::string_view{"VAR_FLOAT"}, std::string_view{"LVAR_FLOAT"},
         std::string_view{"VAR_TEXT_LABEL"},
         std::string_view{"LVAR_TEXT_LABEL"}})
    {
        SUBCASE(cmd_name.data())
        {
            const auto cmd = cmdman.find_command(cmd_name);
            REQUIRE(cmd != nullptr);

            const auto result = rewriter.visit(*SemaIR::Builder(&arena)
                                                        .command(*cmd)
                                                        .arg_var(*dummy_var)
                                                        .build());

            REQUIRE(result);
            REQUIRE(result->empty());
        }
    }
}

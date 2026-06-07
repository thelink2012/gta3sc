#pragma once
#include "../../command-manager-fixture.hpp"
#include "../../with-diagnostic-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/util/arena.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace gta3sc::test::codegen::trilogy
{
/// Shared fixture base for Trilogy codegen unit tests.
///
/// Provides a SymbolTable, arena, relocation table, and the helpers common to
/// both `CodeGenFixture` and `MultifileCodeGenFixture`.
class CommonCodeGenFixture
    : public CommandTableFixture
    , public WithDiagnosticFixture
{
public:
    CommonCodeGenFixture() : symtable(&arena) {}

    CommonCodeGenFixture(const CommonCodeGenFixture&) = delete;
    auto
    operator=(const CommonCodeGenFixture&) -> CommonCodeGenFixture& = delete;

    CommonCodeGenFixture(CommonCodeGenFixture&&) = delete;
    auto operator=(CommonCodeGenFixture&&) -> CommonCodeGenFixture& = delete;

protected:
    auto make_lvar(SymbolTable::VarType var_type, SymbolTable::ScopeId scope_id)
            -> const SymbolTable::Variable&
    {
        const auto [var, inserted] = symtable.insert_var(
                std::to_string(next_symbol_id++), scope_id, var_type,
                std::nullopt, no_file_range);
        REQUIRE(inserted);
        return *var;
    }

    auto make_var(SymbolTable::VarType var_type) -> const SymbolTable::Variable&
    {
        return make_lvar(var_type, SymbolTable::global_scope);
    }

    auto make_label() -> const SymbolTable::Label&
    {
        const auto [label, inserted] = symtable.insert_label(
                std::to_string(next_symbol_id++), SymbolTable::global_scope,
                no_file_range);
        REQUIRE(inserted);
        return *label;
    }

    auto make_file(SymbolTable::FileType file_type) -> const SymbolTable::File&
    {
        const auto [file, inserted] = symtable.insert_file(
                std::to_string(next_symbol_id++), file_type, no_file_range);
        REQUIRE(inserted);
        return *file;
    }

    auto make_used_object() -> const SymbolTable::UsedObject&
    {
        const auto [uobj, inserted] = symtable.insert_used_object(
                std::to_string(next_symbol_id++), no_file_range);
        REQUIRE(inserted);
        return *uobj;
    }

    auto make_storage_table() -> gta3sc::codegen::StorageTable
    {
        return gta3sc::codegen::StorageTable::from_symbols(
                       symtable, gta3sc::codegen::StorageTable::Options())
                .value();
    }

    auto find_command(std::string_view name) -> const CommandTable::CommandDef&
    {
        const auto* command = cmdman.find_command(name);
        REQUIRE(command != nullptr);
        return *command;
    }

    auto find_constant(std::string_view enum_name,
                       std::string_view constant_name)
            -> const CommandTable::ConstantDef&
    {
        const auto enum_id = cmdman.find_enumeration(enum_name);
        REQUIRE(enum_id != std::nullopt);
        const auto* constant = cmdman.find_constant(*enum_id, constant_name);
        REQUIRE(constant != nullptr);
        return *constant;
    }

    ArenaMemoryResource arena;
    gta3sc::codegen::RelocationTable reloc_table;
    SymbolTable symtable; // initialized after arena
    uint32_t next_symbol_id{};
};
} // namespace gta3sc::test::codegen::trilogy

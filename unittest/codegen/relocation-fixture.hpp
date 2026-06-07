#pragma once
#include "../with-diagnostic-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/symbol-table.hpp>

using gta3sc::ArenaMemoryResource;
using ::gta3sc::no_file_loc;
using ::gta3sc::no_file_range;
using gta3sc::SymbolTable;
using gta3sc::codegen::RelocationTable;
using gta3sc::test::WithDiagnosticFixture;
using FileType = gta3sc::SymbolTable::FileType;

namespace gta3sc::test::codegen
{
class RelocationFixture : public WithDiagnosticFixture
{
public:
    RelocationFixture() : symtable(&arena) {}

    RelocationFixture(const RelocationFixture&) = delete;
    auto operator=(const RelocationFixture&) -> RelocationFixture& = delete;

    RelocationFixture(RelocationFixture&&) = delete;
    auto operator=(RelocationFixture&&) -> RelocationFixture& = delete;

    auto make_label() -> const SymbolTable::Label&
    {
        const auto [label, inserted] = symtable.insert_label(
                std::to_string(next_label_id++), SymbolTable::global_scope,
                no_file_range);
        REQUIRE(inserted);
        return *label;
    }

    auto make_file(FileType type) -> const SymbolTable::File&
    {
        const auto [file, inserted] = symtable.insert_file(
                std::to_string(next_file_id++), type, no_file_range);
        REQUIRE(inserted);
        return *file;
    }

    void insert_label_loc(const SymbolTable::Label& label,
                          const SymbolTable::File& file,
                          RelocationTable::AbsoluteOffset offset)
    {
        REQUIRE(reloc_table.insert_label_loc(label, file, offset));
    }

    void insert_file_loc(const SymbolTable::File& file,
                         RelocationTable::AbsoluteOffset offset)
    {
        REQUIRE(reloc_table.insert_file_loc(file, offset));
    }

    void insert_fixup_entry(const SymbolTable::Label& label,
                            const SymbolTable::File& origin,
                            RelocationTable::AbsoluteOffset offset)
    {
        reloc_table.insert_fixup_entry(label, origin, offset);
    }

    void insert_fixup_entry(const SymbolTable::File& file,
                            RelocationTable::AbsoluteOffset offset)
    {
        reloc_table.insert_fixup_entry(file, offset);
    }

protected:
    ArenaMemoryResource arena;
    SymbolTable symtable;
    RelocationTable reloc_table;
    uint32_t next_label_id{};
    uint32_t next_file_id{};
};
} // namespace gta3sc::test::codegen

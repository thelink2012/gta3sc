#pragma once
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/trilogy/emitter.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>

namespace gta3sc::codegen
{
class RelocationTable;
class StorageTable;
} // namespace gta3sc::codegen

namespace gta3sc::codegen::trilogy
{
class MultifileCodeGen
{
public:
    MultifileCodeGen(const SymbolTable& symbol_table,
                     const StorageTable& storage,
                     DiagnosticHandler& diag) noexcept :
        symbol_table(&symbol_table), storage(&storage), diag(&diag)
    {}

    MultifileCodeGen(const MultifileCodeGen&) = delete;
    auto operator=(const MultifileCodeGen&) -> MultifileCodeGen& = delete;

    MultifileCodeGen(MultifileCodeGen&&) noexcept = default;
    auto operator=(MultifileCodeGen&&) noexcept -> MultifileCodeGen& = default;

    ~MultifileCodeGen() noexcept = default;

    // TODO better define the output interface (consider script.img etc)
    bool generate(const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
                  std::vector<std::byte>& output);

private:
    const SymbolTable* symbol_table;
    const StorageTable* storage;
    DiagnosticHandler* diag;
};
} // namespace gta3sc::codegen::trilogy
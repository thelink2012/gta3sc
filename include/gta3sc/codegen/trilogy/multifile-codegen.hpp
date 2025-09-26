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

// TODO doc comments

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

    struct NextFile
    {
        const SymbolTable::File* file; // can be nullptr if we're done
        LinkedIR<SemaIR>::const_iterator next_ir;
    };

    /// Determines header sizes and skips that much space in the output.
    ///
    /// This must be called before any file is generated. And after all
    /// files have been generated, \ref generate_headers must be called
    /// to actually fill the header data.
    ///
    /// Do note that in terms of output this function just clears and resizes
    /// the output to be the same size as the header segment.
    ///
    /// \param output the output vector to clear and resize.
    bool generate_header_stubs(std::vector<std::byte>& output);

    /// Fills the header data in the output.
    ///
    /// This should only be called after all files have been generated.
    ///
    /// \param output the output vector to fill with header data.
    bool generate_headers(const RelocationTable& reloc_table,
                          std::vector<std::byte>& output);

    // TODO better define the output interface (consider script.img etc)
    bool generate(const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
                  std::vector<std::byte>& output);

    auto generate_first_file(
            const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
            std::vector<std::byte>& output) -> std::optional<NextFile>;

    auto generate_next_file(LinkedIR<SemaIR>::const_iterator next_ir,
                            LinkedIR<SemaIR>::const_iterator max_ir,
                            RelocationTable& reloc_table,
                            std::vector<std::byte>& output)
            -> std::optional<NextFile>;

    auto generate_next_file(const SymbolTable::File& file,
                            LinkedIR<SemaIR>::const_iterator next_ir,
                            LinkedIR<SemaIR>::const_iterator max_ir,
                            RelocationTable& reloc_table,
                            std::vector<std::byte>& output)
            -> std::optional<NextFile>;

private:
    auto detect_file_label(const SemaIR& line) -> const SymbolTable::File*;

    auto global_var_header_size() const -> uint32_t;
    auto used_object_header_size() const -> uint32_t;
    auto mission_header_size() const -> uint32_t;

private:
    const SymbolTable* symbol_table;
    const StorageTable* storage;
    DiagnosticHandler* diag;

    uint32_t num_globals{};
    uint32_t num_used_objects{};
    uint32_t num_missions{};
    RelocationTable::AbsoluteOffset header_size{};
    RelocationTable::AbsoluteOffset current_multifile_offset{};
};
} // namespace gta3sc::codegen::trilogy
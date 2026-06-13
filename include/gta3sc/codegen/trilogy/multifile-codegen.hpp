#pragma once
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/trilogy/emitter.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <optional>

namespace gta3sc::codegen
{
class RelocationTable;
class StorageTable;
} // namespace gta3sc::codegen

// TODO doc comments

namespace gta3sc::codegen::trilogy
{
/// Generates multifile bytecode for GTA III, Vice City and San Andreas.
///
/// This class is responsible for generating the complete `MAIN.SCM` file
/// from semantically valid IR. It takes the IR for each multiscript
/// and generates bytecode and relocation information, as well as the
/// SCM header.
///
/// \see CodeGen for more details on code generation.
///
/// \see MultifileParser and \see Sema for details on parsing multiple files
/// into semantically valid IR.
///
/// All file labels must have a corresponding symbol table entry or behavior
/// is undefined. \ref MultifileParser IR satisfies this requirement.
class MultifileCodeGen
{
public:
    struct NextFile;

public:
    /// Constructs a multifile code generator.
    ///
    /// \param symbol_table the symbol table used to extract information about
    /// script files and used objects.
    /// \param storage the storage table used to extract information about
    /// the storage of variables.
    /// \param diag where diagnostics will be reported to.
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

    /// Returns true if any error was reported during generation.
    [[nodiscard]] auto has_error() const noexcept -> bool;

    /// Generates headers and bytecode for a list of instructions.
    ///
    /// Behaves as if by calling \ref compute_header_stubs, followed by
    /// \ref generate_next_file for each file in the IR, and finally
    /// \ref generate_headers to fill the header data.
    ///
    /// All errors are reported to the diagnostic handler. Generation
    /// continues after recoverable errors to collect as many diagnostics
    /// as possible. Returns false if any error was reported.
    bool generate(const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
                  std::vector<std::byte>& output);

    /// Computes initial header information and determines header sizes.
    ///
    /// This **must** be called before any file is generated. After all
    /// files have been generated, \ref generate_headers must be called
    /// to actually fill the header data.
    ///
    /// Returns the size of the header segment. In general you should reserve
    /// this much space in the output before appending any other data
    /// (i.e. before using it with any other method of this class).
    ///
    /// \returns the size of the header segment or `std::nullopt` in case of an
    /// error. Errors are reported to the diagnostic handler.
    auto
    compute_header_stubs() -> std::optional<RelocationTable::AbsoluteOffset>;

    /// Fills the header data in the output.
    ///
    /// This should only be called after both \ref compute_header_stubs and
    /// all files have been generated through \ref generate_next_file.
    ///
    /// \param output the output vector to fill with header data.
    bool generate_headers(const RelocationTable& reloc_table,
                          std::vector<std::byte>& output);

    /// Behaves as if calling \ref generate_next_file with `next_ir` and
    /// `max_ir` set to `ir.begin()` and `ir.end()`, respectively.
    auto generate_first_file(
            const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
            std::vector<std::byte>& output) -> std::optional<NextFile>;

    /// Generates bytecode for instructions starting at `next_ir` and
    /// ending at the next file label (unless `max_ir` is reached first).
    ///
    /// \see MultifileParser for details on file labels. Essentially it's
    /// a special label that starts with `@@` followed by the filename.
    ///
    /// Once the next file is reached, the method returns a `NextFile`
    /// describing the next file to be generated. This method can then
    /// be called again with the parameters provided by `NextFile`.
    ///
    /// \note This method is stateful, it accumulates the bytecode size into an
    /// internal variable keeping track of the current multifile offset.
    ///
    /// \param next_ir the iterator to the first instruction of the file to be
    /// generated. Must be pointing to one instruction after the file label.
    /// \param max_ir the iterator to the end of the IR.
    /// \param reloc_table where to output relocation information to.
    /// \param output the output vector to fill with bytecode.
    ///
    /// \returns a `NextFile` describing the next file to be generated, or
    /// `std::nullopt` in case of an unrecoverable error. Use \ref has_error
    /// to check whether any recoverable error was reported during generation.
    auto generate_next_file(const SymbolTable::File& file,
                            LinkedIR<SemaIR>::const_iterator next_ir,
                            LinkedIR<SemaIR>::const_iterator max_ir,
                            RelocationTable& reloc_table,
                            std::vector<std::byte>& output)
            -> std::optional<NextFile>;

    /// Behaves as if calling \ref generate_next_file with `file` set
    /// to the file information extracted from the label at `next_ir`.
    ///
    /// Unlike the other overload, this method requires `next_ir` to be
    /// pointing to the file label.
    auto generate_next_file(LinkedIR<SemaIR>::const_iterator next_ir,
                            LinkedIR<SemaIR>::const_iterator max_ir,
                            RelocationTable& reloc_table,
                            std::vector<std::byte>& output)
            -> std::optional<NextFile>;

public:
    /// Describes the next file to be generated.
    struct NextFile
    {
        /// The next file to be generated.
        /// Set to `nullptr` if there's no more files to generate.
        const SymbolTable::File* file;
        /// The iterator to the first instruction of the next file,
        /// pointing to one instruction after the file label.
        LinkedIR<SemaIR>::const_iterator next_ir;
    };

private:
    auto report(FileRange source,
                const DiagnosticDescriptor& message) -> Diagnostic::Builder;

    auto detect_file_label(const SemaIR& line) -> const SymbolTable::File*;

    auto global_var_header_size() const -> uint32_t;
    auto used_object_header_size() const -> uint32_t;
    auto mission_header_size() const -> uint32_t;

    bool generate_global_var_header(
            RelocationTable::AbsoluteOffset& next_header_offset,
            CodeEmitter& emitter);
    bool generate_used_object_header(
            RelocationTable::AbsoluteOffset& next_header_offset,
            CodeEmitter& emitter);
    bool
    generate_mission_header(RelocationTable::AbsoluteOffset& next_header_offset,
                            CodeEmitter& emitter,
                            const RelocationTable& reloc_table);

private:
    const SymbolTable* symbol_table;
    const StorageTable* storage;
    DiagnosticHandler* diag;

    uint32_t globals_top_index{};
    uint32_t num_used_objects{};
    uint32_t num_missions{};
    RelocationTable::AbsoluteOffset header_size{};
    RelocationTable::AbsoluteOffset current_multifile_offset{};
    RelocationTable::AbsoluteOffset main_segment_size{};
    RelocationTable::AbsoluteOffset largest_mission_script_size{};
    bool computed_header_size{};
    bool m_has_error{};
};
} // namespace gta3sc::codegen::trilogy

namespace gta3sc::codegen::trilogy::diag
{
extern const DiagnosticDescriptor used_object_name_too_long;
} // namespace gta3sc::codegen::trilogy::diag
#pragma once
#include <filesystem>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <optional>
#include <vector>

namespace gta3sc
{
class CommandTable;
class ModelTable;
class SourceManager;
class DiagnosticHandler;
} // namespace gta3sc

namespace gta3sc::driver
{
/// Compiles a multifile script (main.sc and ancillary script files) into a
/// binary stream.
///
/// It's recommended to create one \ref SourceManager per use of this class to
/// avoid source location exhaustion issues. Additionally, \ref
/// SourceManager::scan_directory is of responsability of the caller.
///
/// See `DESIGN.adoc` for more details on the whole compilation pipeline.
class Compilation
{
public:
    struct Result;

public:
    /// \param input_file The input `main.sc` file to compile.
    /// \param command_table The command table to use for command resolution.
    /// \param model_table The model table to use for model resolution.
    /// \param source_manager The source manager to use for file loading
    /// and source location tracking.
    /// \param diag_manager where to report diagnostics to.
    Compilation(const std::filesystem::path& input_file,
                CommandTable& command_table, ModelTable& model_table,
                SourceManager& source_manager, DiagnosticHandler& diag_manager);

    Compilation(Compilation&&) noexcept = default;
    auto operator=(Compilation&&) noexcept -> Compilation& = default;

    Compilation(const Compilation&) = delete;
    auto operator=(const Compilation&) -> Compilation& = delete;

    ~Compilation() noexcept = default;

    /// Compiles the input multifile given in the constructor into a binary
    /// stream given in \ref result.
    ///
    /// This method can only be called once for an instance of \ref Compilation.
    ///
    /// \returns whether compilation succeeds, otherwise false and diagnostics
    /// are produced.
    bool compile(Result result);

private:
    /// Parses the input multifile into a parser IR.
    ///
    /// Constructs all IR objects in \ref parser_ir_arena and the symbol table
    /// objects in \ref symbol_arena.
    ///
    /// Returns a parser IR if parsing succeeds, otherwise `std::nullopt`
    /// and a diagnostic is produced.
    auto parse() -> std::optional<LinkedIR<ParserIR>>;

    /// Lowers high-level constructs into low-level ones.
    ///
    /// Constructs all IR objects in \ref parser_ir_arena and the symbol table
    /// objects in \ref symbol_arena.
    ///
    /// Returns a parser IR if lowering succeeds, otherwise `std::nullopt`
    /// and a diagnostic is produced.
    auto lower(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<ParserIR>>;

    /// Validates the semantics of the parser IR and produces a sema IR.
    ///
    /// Constructs all IR objects in \ref sema_ir_arena and the symbol table
    /// objects in \ref symbol_arena.
    ///
    /// After this pass, the parser IR allocations are released in
    /// favor of the sema IR allocations..
    ///
    /// Returns a sema IR if semantics validation succeeds, otherwise
    /// `std::nullopt` and a diagnostic is produced.
    auto sema(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<SemaIR>>;

    /// Code generates the sema IR into a binary stream.
    ///
    /// After this pass, the sema IR and symbol table allocations are released.
    ///
    /// Returns true if code generation succeeds, otherwise false and a
    /// diagnostic is produced.
    auto codegen(LinkedIR<SemaIR> ir, Result result) -> bool;

protected:
    // Using std::unique_ptr such that \ref Compilation is moveable.
    std::unique_ptr<ArenaMemoryResource> symbol_arena;
    std::unique_ptr<ArenaMemoryResource> parser_ir_arena;
    std::unique_ptr<ArenaMemoryResource> sema_ir_arena;
    SymbolTable symbol_table;

private:
    std::filesystem::path input_file;
    CommandTable* command_table;
    ModelTable* model_table;
    SourceManager* source_manager;
    DiagnosticHandler* diag_manager;
};

struct Compilation::Result
{
    /// The output stream for `main.scm`.
    ///
    /// Must **NOT** be nullptr.
    std::vector<std::byte>* target_main_scm;
};
} // namespace gta3sc::driver

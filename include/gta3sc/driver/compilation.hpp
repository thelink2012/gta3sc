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
class Compilation
{
public:
    // TODO improve ctor definition
    Compilation(const std::filesystem::path& input_file,
                CommandTable& command_table, ModelTable& model_table,
                SourceManager& source_manager, DiagnosticHandler& diag_manager);

    Compilation(Compilation&&) noexcept = default;
    auto operator=(Compilation&&) noexcept -> Compilation& = default;

    Compilation(const Compilation&) = delete;
    auto operator=(const Compilation&) -> Compilation& = delete;

    // TODO virtual
    ~Compilation() noexcept = default;

    bool compile();

    // TODO output interface
    std::vector<std::byte> output;

protected:
    auto parse() -> std::optional<LinkedIR<ParserIR>>;
    auto lower(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<ParserIR>>;
    auto sema(LinkedIR<ParserIR> ir) -> std::optional<LinkedIR<SemaIR>>;
    auto codegen(const LinkedIR<SemaIR>& ir) -> bool; // TODO output interface

protected:
    std::unique_ptr<ArenaMemoryResource> symbol_arena;
    std::unique_ptr<ArenaMemoryResource> parser_ir_arena;
    std::unique_ptr<ArenaMemoryResource> sema_ir_arena;
    SymbolTable symbol_table;

private: // TODO move some to protected storage maybe?
    std::filesystem::path input_file;
    CommandTable* command_table;
    ModelTable* model_table;
    SourceManager* source_manager;
    DiagnosticHandler* diag_manager;
};
} // namespace gta3sc::driver
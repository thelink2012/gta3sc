#pragma once

#include <filesystem>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena.hpp>
#include <optional>
#include <queue>

namespace gta3sc::syntax
{
/// The multifile parser takes a main script file and parses it alongside its
/// subscript files, including mission scripts and streaming scripts.
///
/// See \ref Preprocessor, \ref Scanner and \ref Parser for details on parsing.
///
/// Outputs whether the stream is valid and an intermediate representation for
/// further processing. Also populates the symbol repository with file
/// definitions.
///
/// The main file is populated into the symbol table with the name `MAIN`. The
/// other files are inserted with their respective filename e.g. `FILENAME.SC`.
///
/// The representation for all scripts are concatenated together in an order
/// equivalent to the tuple `(script_type, script_seen_order)`. The beginning of
/// each script file in the IR is marked with a special label. The main script
/// is marked with a label `@@MAIN` and the other files with a label
/// `@@FILENAME.SC`. Notice the suffix following `@@` is the unique identifier
/// of the file in the symbol table.
class MultifileParser
{
public:
    /// \param main_script_path the path to the main script file.
    /// \param symbol_table the repository that symbols should be inserted in.
    /// \param source_manager the source manager to load files from.
    /// \param diag the diagnostic handler to report errors to.
    /// \param allocator the arena that should be used to allocate IR in.
    MultifileParser(std::filesystem::path main_script_path,
                    SymbolTable& symbol_table, SourceManager& source_manager,
                    DiagnosticHandler& diag, ArenaAllocator<> allocator);

    MultifileParser(const MultifileParser&) = delete;
    auto operator=(const MultifileParser&) -> MultifileParser& = delete;

    MultifileParser(MultifileParser&&) noexcept = default;
    auto operator=(MultifileParser&&) noexcept -> MultifileParser& = default;

    ~MultifileParser() noexcept = default;

    /// Parses the main script file and all its ancillary script files.
    [[nodiscard]] auto parse() -> std::optional<LinkedIR<ParserIR>>;

private:
    auto parse(SourceFile source_file,
               SymbolTable::FileType type) -> std::optional<LinkedIR<ParserIR>>;
    auto load_file(const SymbolTable::File& file) -> std::optional<SourceFile>;

    void analyze_required_files(const LinkedIR<ParserIR>& ir);
    void analyze_required_file(const ParserIR::Command& command,
                               uint32_t arg_index, SymbolTable::FileType type);

private:
    struct ParseQueueItem
    {
        const gta3sc::SymbolTable::File* file;
        bool operator<(const ParseQueueItem& other) const;
    };

private:
    std::priority_queue<ParseQueueItem> parse_queue;
    SymbolTable::FileType parsing_file_type{SymbolTable::FileType::main};

    std::filesystem::path main_script_path;
    SymbolTable* symbol_table;
    SourceManager* source_manager;
    DiagnosticHandler* diag;
    ArenaAllocator<> ir_allocator;
};
} // namespace gta3sc::syntax
#pragma once

#include <filesystem>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/source-manager.hpp>
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
/// \ref SourceManager must provide the ability to load files by filename,
/// meaning the caller is responsible for calling \ref
/// SourceManager::scan_directory if necessary.
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
///
/// Example:
/// \code{.cpp}
/// MultifileParser parser("path/to/main.sc", ...);
/// auto ir = parser.parse();
/// // ir contains the IR for the main script file and all its ancillary script
/// files \endcode
///
/// You can also use \ref has_next_file and \ref parse_next_file to parse one
/// file at a time.
class MultifileParser
{
public:
    /// Constructs a multifile parser with a main file in the next file queue.
    ///
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

    /// Checks whether there is a next file to parse.
    ///
    /// The initial state of the MultifileParser is such that there is a main
    /// file in the next file queue, meaning the first time this function is
    /// called, it will return true.
    [[nodiscard]] auto has_next_file() -> bool;

    /// Parses the next file in the queue.
    ///
    /// The file is removed from the queue after parsing.
    ///
    /// It's forbidden to call this function if \ref has_next_file returns
    /// false.
    ///
    /// If parsing fails, `std::nullopt` is returned and a diagnostic is
    /// produced.
    [[nodiscard]] auto parse_next_file() -> std::optional<LinkedIR<ParserIR>>;

    /// Parses all remaining files in the queue.
    ///
    /// An recently initialized MultifileParser followed by this call parses
    /// the main script file and all its ancillary script files.
    ///
    /// After this function returns, \ref has_next_file will return false.
    ///
    /// If parsing fails, `std::nullopt` is returned and a diagnostic is
    /// produced.
    [[nodiscard]] auto parse() -> std::optional<LinkedIR<ParserIR>>;

private:
    auto parse(FileEntryRef source_file,
               SymbolTable::FileType type) -> std::optional<LinkedIR<ParserIR>>;
    auto load_file(const SymbolTable::File& file) -> std::optional<FileEntryRef>;

    void push_required_files(const LinkedIR<ParserIR>& ir);
    void analyze_required_file(const ParserIR::Command& command,
                               uint32_t arg_index, SymbolTable::FileType type);

    void
    report_unsupported_import_order(const SymbolTable::File& imported_file);

private:
    struct ParseQueueItem;

private:
    std::priority_queue<ParseQueueItem> parse_queue;
    SymbolTable::FileType parsing_file_type{SymbolTable::FileType::main};
    std::filesystem::path main_script_path;
    SymbolTable* symbol_table;
    SourceManager* source_manager;
    DiagnosticHandler* diag;
    ArenaAllocator<> ir_allocator;
};

struct MultifileParser::ParseQueueItem
{
    const gta3sc::SymbolTable::File* file;

    /// Inverted `(type, type_id)` ordering for \c std::priority_queue.
    bool operator<(const ParseQueueItem& other) const;
};
} // namespace gta3sc::syntax

namespace gta3sc::syntax::diag
{
extern const DiagnosticDescriptor unsupported_main_extension_import_order;
extern const DiagnosticDescriptor unsupported_subscript_import_order;
extern const DiagnosticDescriptor unsupported_mission_import_order;
extern const DiagnosticDescriptor
        file_already_imported_as_different_type; // %0 => FileType/int (first
                                                 // time), %1 => FileType/int
                                                 // (this time)
} // namespace gta3sc::syntax::diag
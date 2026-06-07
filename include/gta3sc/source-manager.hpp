#pragma once
#include <gta3sc/filesystem/file-location.hpp>
#include <gta3sc/filesystem/file-pool.hpp>
#include <gta3sc/filesystem/path-resolver.hpp>

namespace gta3sc
{
/// Manages source files, their locations and ranges.
///
/// \see FilePool
///
/// \code{.cpp}
/// SourceManager source_manager;
/// source_manager.scan_directory("/liberty/main");
///
/// MultifileParser parser("/liberty/main.sc",
///                        symbol_table, source_manager,
///                        diag, allocator);
///
/// auto ir = parser.parse(); // parses main.sc and subscripts
/// \endcode
class SourceManager : public filesystem::FilePool
{
public:
    SourceManager() = default;

    SourceManager(SourceManager&&) noexcept = default;
    auto operator=(SourceManager&&) noexcept -> SourceManager& = default;

    // New load_file overload would otherwise hide parent overloads.
    using filesystem::FilePool::load_file;

    /// Recursively scans a directory, registering `.sc` files for later lookup.
    ///
    /// Subsequent calls accumulate. Follows directory symlinks.
    ///
    /// Returns true on success, false on I/O error.
    ///
    /// \see ScriptPathResolver
    [[nodiscard]] auto scan_directory(const std::filesystem::path& dir) -> bool;

    /// Loads a file by filename.
    ///
    /// The file path is resolved by looking at the files seen during \ref
    /// scan_directory.
    [[nodiscard]] auto load_file(std::string_view filename)
            -> std::optional<filesystem::FileEntryRef>;

private:
    gta3sc::filesystem::ScriptPathResolver script_resolver;
};
} // namespace gta3sc

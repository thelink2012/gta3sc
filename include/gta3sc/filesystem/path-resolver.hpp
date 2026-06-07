#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gta3sc::filesystem
{
/// Abstract interface for path resolution strategies.
///
/// Used by name-based loaders (e.g. \ref SourceManager::load_file) to map
/// logical names to on-disk paths.
class PathResolver
{
public:
    virtual ~PathResolver() = default;

    /// Maps a logical name (e.g. script filename) to a real on-disk path.
    ///
    /// Returns `std::nullopt` if resolution fails, otherwise path to file.
    virtual auto
    resolve(std::string_view name) const -> std::optional<std::filesystem::path>
                                            = 0;
};

/// Path resolver that maps script filenames to paths.
///
/// Populated by repeated `scan_directory` calls. When multiple files share
/// the same filename, the first registered entry wins.
///
/// \code{.cpp}
/// ScriptPathResolver r;
/// r.scan_directory("/game/main");  // Finds missions/staunton/ASUKA1.sc
/// auto p = r.resolve("ASUKA1.sc"); // Returns
/// /game/main/missions/staunton/asuka1.sc \endcode
class ScriptPathResolver : public PathResolver
{
public:
    ScriptPathResolver() = default;

    ScriptPathResolver(const ScriptPathResolver&) = delete;
    auto operator=(const ScriptPathResolver&) -> ScriptPathResolver& = delete;

    ScriptPathResolver(ScriptPathResolver&&) noexcept = default;
    auto
    operator=(ScriptPathResolver&&) noexcept -> ScriptPathResolver& = default;

    /// Recursively scans a directory, registering `.sc` files for later lookup.
    ///
    /// Subsequent calls accumulate. Follows directory symlinks.
    ///
    /// Returns true on success, false on I/O error.
    auto scan_directory(const std::filesystem::path& dir) -> bool;

    /// Case-insensitive lookup by script basename.
    ///
    /// Returns `nullopt` if not found, path otherwise.
    auto resolve(std::string_view filename) const
            -> std::optional<std::filesystem::path> override;

private:
    struct FilenamePath
    {
        std::string filename;
        std::filesystem::path path;
    };

private:
    /// Pairs gathered by scan_directory(), in scan order.
    std::vector<FilenamePath> filename_to_path;
};

/// Resolver for relative paths anchored at a root directory.
///
/// \code{.cpp}
/// RelativePathResolver r("/game");
///
/// auto p = r.resolve("DATA/MAPS/foo.IDE");
/// => /game/DATA/MAPS/foo.IDE
/// \endcode
///
/// \see RelativeInsensitivePathResolver for case-insensitive version.
class RelativePathResolver : public PathResolver
{
public:
    explicit RelativePathResolver(std::filesystem::path root_path);

    RelativePathResolver(const RelativePathResolver&) = delete;
    auto
    operator=(const RelativePathResolver&) -> RelativePathResolver& = delete;

    RelativePathResolver(RelativePathResolver&&) noexcept = default;
    auto operator=(RelativePathResolver&&) noexcept
            -> RelativePathResolver& = default;

    ~RelativePathResolver() override = default;

    /// Lexically joins \p relative_path under the stored root anchor using the
    /// host `std::filesystem::path` concatenation rules.
    ///
    /// \returns the resolved path or `std::nullopt` on failure.
    auto resolve(std::string_view relative_path) const
            -> std::optional<std::filesystem::path> override;

private:
    std::filesystem::path root;
};

/// Relative path resolver with per-segment ASCII case-insensitive matching.
///
/// Used specially for DAT/IDE relative path resolution, since those files
/// were authored on Windows and embed paths with arbitrary casing and
/// backslash separators that may not match a case-sensitive filesystem.
///
/// Resolves multi-segment relative paths (e.g. "DATA\MAPS\foo.IDE") against
/// a root-path anchor using per-directory case-insensitive matching.
///
/// \code{.cpp}
/// RelativeInsensitivePathResolver r("/game");
///
/// auto p = r.resolve("DATA\\MAPS\\foo.IDE");
/// // => /game/data/maps/foo.ide  (actual casing on disk)
///
/// auto q = r.resolve("data/maps/foo.ide");
/// // same result — slashes are interchangeable
/// \endcode
class RelativeInsensitivePathResolver : public PathResolver
{
public:
    /// Constructs the resolver anchored at the given root path.
    explicit RelativeInsensitivePathResolver(std::filesystem::path root_path);

    RelativeInsensitivePathResolver(const RelativeInsensitivePathResolver&)
            = delete;
    auto operator=(const RelativeInsensitivePathResolver&)
            -> RelativeInsensitivePathResolver& = delete;

    RelativeInsensitivePathResolver(RelativeInsensitivePathResolver&&) noexcept
            = default;
    auto operator=(RelativeInsensitivePathResolver&&) noexcept
            -> RelativeInsensitivePathResolver& = default;

    /// Joins \p relative_path under the stored root anchor using
    /// case-insensitive path search.
    ///
    /// \returns the resolved path or `std::nullopt` when any segment has no
    /// case-insensitive match on disk.
    auto resolve(std::string_view relative_path) const
            -> std::optional<std::filesystem::path> override;

private:
    /// \returns the cached directory listing for \p dir, populating it on first
    /// access or `nullptr` on I/O failure.
    auto list_directory(const std::filesystem::path& dir) const
            -> const std::vector<std::string>*;

    /// \returns a pointer to the first entry in \p entries that matches \p name
    /// case-insensitively, or nullptr if none found.
    static auto find_entry(const std::vector<std::string>& entries,
                           std::string_view name) -> const std::string*;

    /// Lists \p dir and returns a pointer to the first entry matching \p name
    /// case-insensitively. Returns nullptr on I/O failure or no match.
    auto resolve_component(const std::filesystem::path& dir,
                           std::string_view name) const -> const std::string*;

private:
    /// Canonicalized root path anchor.
    std::filesystem::path root;

    /// Directory cache.
    ///
    /// Mutable for lazy population inside const resolve().
    mutable std::map<std::filesystem::path, std::vector<std::string>> dir_cache;
};

// TODO move the comment below to .cpp
// TODO check if first-scanned entry win behavior is compatible with miss2.
// TODO implement thread safety in `mutable dir_cache`
} // namespace gta3sc::filesystem

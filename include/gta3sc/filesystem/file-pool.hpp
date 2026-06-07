#pragma once
#include <cassert>
#include <filesystem>
#include <gta3sc/filesystem/file-location.hpp>
#include <gta3sc/fwd.hpp>
#include <gta3sc/util/arena.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace gta3sc::filesystem
{
struct FileEntry;

/// RAII handle to a loaded file content.
///
/// File content is keptin memory as long as at least one handle exists.
///
/// \see FilePool
class FileEntryRef
{
public:
    ~FileEntryRef();

    // TODO probably should be copyable (and increment refcount)
    FileEntryRef(const FileEntryRef&) = delete;
    auto operator=(const FileEntryRef&) -> FileEntryRef& = delete;

    FileEntryRef(FileEntryRef&&) noexcept;
    auto operator=(FileEntryRef&&) noexcept -> FileEntryRef&;

    /// Returns the raw bytes of the loaded file.
    ///
    /// The returned buffer has a trailing null terminator, helpful for
    /// handling source files.
    [[nodiscard]] auto data() const -> const char*;

    /// Returns the size in bytes of the file, excluding the null terminator.
    [[nodiscard]] auto size() const -> size_t;

    /// Returns a string view over the file content.
    [[nodiscard]] auto view() const -> std::string_view;

    /// Returns the \ref FileLoc of the byte at `p`.
    ///
    /// \pre `p` must point into `data()[0..size()-1]`.
    [[nodiscard]] auto location_of(const char* p) const -> FileLoc;

    /// Returns `location_of(view.data())`.
    [[nodiscard]] auto location_of(std::string_view view) const -> FileLoc;

    /// Returns a string view over the bytes in `range`.
    ///
    /// The view is valid only while there is a open \ref FileEntryRef handle
    /// to the file.
    ///
    /// \pre `range` must fall within this file's span.
    [[nodiscard]] auto view_of(FileRange range) const -> std::string_view;

    /// Returns path this file was loaded from, or empty for ephemeral buffers.
    [[nodiscard]] auto path() const -> const std::filesystem::path&;

private:
    /// Only \ref FilePool may wrap an internal \ref FileEntry in a handle.
    friend class FilePool;

    explicit FileEntryRef(FileEntry& entry);

private:
    FileEntry* entry{};
};

/// Internal per-file record. Do not hold directly; use \ref FileEntryRef.
///
/// \see FilePool
struct FileEntry
{
    /// Path to the file in the filesystem.
    ///
    /// Empty for ephemeral (in-memory) buffers.
    std::filesystem::path path;

    /// Start of the location range used by this file.
    FileLoc start_loc{};

    /// The length of the file in bytes.
    uint32_t file_length{};

    /// Number of live \ref FileEntryRef handles.
    size_t ref_count{0};

    /// Heap buffer holding the file bytes plus a trailing '\0'.
    ///
    /// May be null if the file content was paged out.
    std::unique_ptr<char[]> data;

    /// File modification time recorded at first page in.
    ///
    /// May be zero if no modification time is available.
    std::filesystem::file_time_type file_mtime{};
};

/// Manages files, their locations and ranges.
///
/// Loads and manages file bytes, assigning each file a unique \ref FileLoc
/// range. Files content may be paged in/out of memory as needed.
///
/// Each instance owns an independent \ref FileLoc address space. A \ref FileLoc
/// from one instance must not be used with another.
///
/// This can be seen as a kind of arena for files.
class FilePool
{
public:
    FilePool() noexcept = default;

    FilePool(const FilePool&) = delete;
    auto operator=(const FilePool&) -> FilePool& = delete;

    FilePool(FilePool&&) noexcept;
    auto operator=(FilePool&&) noexcept -> FilePool&;

    /// Loads the file at `path`. Returns `nullopt` on I/O error.
    [[nodiscard]] auto
    load_file(const std::filesystem::path& path) -> std::optional<FileEntryRef>;

    /// Loads from an in-memory buffer.
    ///
    /// `size` is the total buffer allocation in bytes, **including** the null
    /// terminator. The byte at `data[size - 1]` must be `'\0'`.
    ///
    /// Ephemeral: content cannot be reloaded after page eviction.
    [[nodiscard]] auto load_buffer(std::unique_ptr<char[]> data,
                                   size_t size) -> std::optional<FileEntryRef>;

    /// Returns a copy of the bytes in `range`, or `nullopt` if unavailable.
    ///
    /// Prefer to use \ref FileEntryRef::view_of since it returns a view instead
    /// of a copy.
    [[nodiscard]] auto
    string_copy_of(FileRange range) const -> std::optional<std::string>;

    /// Same as \ref string_copy_of(FileRange) but allocates in an arena.
    [[nodiscard]] auto string_copy_of(FileRange range, ArenaAllocator<> alloc)
            const -> std::optional<std::string_view>;

private:
    [[nodiscard]] auto
    load_file_impl(const std::filesystem::path& path,
                   std::unique_ptr<char[]> data,
                   size_t size) -> std::optional<FileEntryRef>;

    /// \returns the \ref FileEntry containing `range`, or `nullptr` if no entry
    /// spans `range`.
    [[nodiscard]] auto find_entry(FileRange range) const -> const FileEntry*;

private:
    /// Container for file entries.
    ///
    /// Allows efficient lookup by locations and ranges.
    std::map<FileLoc, FileEntry> files;

    /// Next free location in the file address space.
    ///
    /// Starts from `1` because of `0` is reserved for \ref no_file_loc.
    FileLoc next_loc{1};
};
} // namespace gta3sc::filesystem
#pragma once
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace gta3sc
{
/// Handle to a location in the source file.
enum class SourceLocation : uint32_t
{ // strong typedef
};

inline bool operator==(SourceLocation lhs, SourceLocation rhs) noexcept
{
    return static_cast<uint32_t>(lhs) == static_cast<uint32_t>(rhs);
}

inline bool operator!=(SourceLocation lhs, SourceLocation rhs) noexcept
{
    return !(lhs == rhs);
}

inline auto operator+(SourceLocation lhs, std::ptrdiff_t rhs) noexcept
        -> SourceLocation
{
    return SourceLocation{
            static_cast<uint32_t>(static_cast<uint32_t>(lhs) + rhs)};
}

inline auto operator+=(SourceLocation& lhs, std::ptrdiff_t rhs) noexcept
        -> SourceLocation&
{
    lhs = lhs + rhs;
    return lhs;
}

inline auto operator-(SourceLocation lhs, std::ptrdiff_t rhs) noexcept
        -> SourceLocation
{
    return SourceLocation{
            static_cast<uint32_t>(static_cast<uint32_t>(lhs) - rhs)};
}

inline auto operator-=(SourceLocation& lhs, std::ptrdiff_t rhs) noexcept
        -> SourceLocation&
{
    lhs = lhs - rhs;
    return lhs;
}

inline auto operator-(SourceLocation lhs, SourceLocation rhs) noexcept
        -> std::ptrdiff_t
{
    return static_cast<std::ptrdiff_t>(lhs) - static_cast<std::ptrdiff_t>(rhs);
}

inline auto operator++(SourceLocation& lhs) noexcept -> SourceLocation&
{
    lhs += 1;
    return lhs;
}

inline auto operator--(SourceLocation& lhs) noexcept -> SourceLocation&
{
    lhs -= 1;
    return lhs;
}

inline auto operator++(const SourceLocation& lhs, int) noexcept
        -> SourceLocation
{
    return lhs + 1;
}

inline auto operator--(const SourceLocation& lhs, int) noexcept
        -> SourceLocation
{
    return lhs - 1;
}

/// Handle to a range of characters in the source file.
struct SourceRange
{
    SourceLocation begin{};
    SourceLocation end{};

    SourceRange() noexcept = default;

    SourceRange(SourceLocation begin, SourceLocation end) noexcept :
        begin(begin), end(end)
    {}

    SourceRange(SourceLocation begin, std::ptrdiff_t len) noexcept :
        SourceRange(begin, begin + len)
    {}

    /// Returns the number of characters in this range.
    auto size() const -> size_t { return end - begin; }

    /// Returns a subrange of this range.
    ///
    /// \param offset the position to start the new range from this range.
    /// \param count the length of the new range.
    auto subrange(size_t offset, size_t count = -1) const -> SourceRange
    {
        offset = std::min(offset, this->size());
        count = std::min(count, this->size() - offset);
        return SourceRange(this->begin + offset, count);
    }

    bool operator==(const SourceRange& rhs) const
    {
        return begin == rhs.begin && end == rhs.end;
    }

    bool operator!=(const SourceRange& rhs) const { return !(*this == rhs); }
};

class SourceFile;

static_assert(std::is_trivially_copyable_v<SourceLocation>);
static_assert(std::is_trivially_copyable_v<SourceRange>);

/// Manages source files, locations and ranges.
///
/// This object manages the paging of source files in and out of memory.
///
/// Use this manager to load source files and query characters or strings
/// based (purely) on SourceLocation or SourceRange of the characters.
class SourceManager
{
public:
    /// Represents no source location.
    ///
    /// Guaranteed to be the default construction of `SourceLocation`.
    static constexpr SourceLocation no_source_loc{};

    /// Represents no source range.
    ///
    /// Guaranteed to be the default construction of `SourceRange`.
    static constexpr SourceRange no_source_range{};

public:
    SourceManager() noexcept = default;

    SourceManager(const SourceManager&) = delete;
    SourceManager& operator=(const SourceManager&) = delete;

    SourceManager(SourceManager&&) noexcept = default;
    SourceManager& operator=(SourceManager&&) noexcept = default;

    ~SourceManager() noexcept = default;

    /// Keeps track of all filenames in the given directory (recursively).
    ///
    /// Files found in this search can be loaded by filename in the `load_file`
    /// method.
    ///
    /// If multiple files are found with the same name, it's unspecified which
    /// file takes precedence to the other.
    ///
    /// If multiple calls to this method are made, it behaves as if the search
    /// continues from the previous method call.
    ///
    /// Returns whether it was possible to scan the given directory.
    bool scan_directory(const std::filesystem::path&);

    /// Loads a source file given its filename.
    ///
    /// For this method to work, `scan_directory` must have found the given
    /// file during its search.
    auto load_file(std::string_view filename) -> std::optional<SourceFile>;

    /// Loads a source file given its path.
    auto load_file(const std::filesystem::path&) -> std::optional<SourceFile>;

    /// Loads a source file given a null-terminated sequence of characters.
    /// \param data the sequence of null-terminated characters.
    /// \param size the size of the sequence not including the null-terminator.
    auto load_file(std::unique_ptr<char[]> data, size_t size)
            -> std::optional<SourceFile>;

protected:
    friend class SourceFile;

    /// Internal information about a source file.
    struct SourceInfo
    {
        /// Path to the source file.
        std::filesystem::path path;
        /// Start of the location range used by this source file.
        SourceLocation start_loc;
        /// The length of the source file in bytes.
        uint32_t file_length{};

        /// Number of SourceFile handles pointing to this structure.
        size_t refcount = 0;
        /// Characters of the source file. May be null if paged out.
        std::unique_ptr<char[]> data;
    };

private:
    /// Checks whether two strings are equal (ignoring casing).
    bool iequal(std::string_view, std::string_view) const;

    auto load_file(const std::filesystem::path&, std::FILE*,
                   size_t hint_size = -1) -> std::optional<SourceFile>;

    auto load_file(const std::filesystem::path&, std::unique_ptr<char[]> data,
                   size_t size) -> std::optional<SourceFile>;

    auto load_file(std::FILE*, size_t hint_size = -1)
            -> std::optional<SourceFile>;

private:
    struct FilenamePathPair
    {
        std::string filename;
        std::filesystem::path path;
    };

    /// Maps filenames and paths.
    ///
    /// This can benefit from the cache locality of a linear search in a
    /// flat array since the number of files to be compiled is usually small.
    std::vector<FilenamePathPair> filename_to_path;

    /// Search tree for a source information given its start location range.
    std::map<SourceLocation, SourceInfo> source_infos;

    /// The starting source location of the next `load_file` method call.
    SourceLocation next_source_loc{1};
};

/// Handle to a source file.
class SourceFile
{
public:
    ~SourceFile()
    {
        if(this->info)
            --this->info->refcount;
    }

    SourceFile(const SourceFile&) = delete;
    SourceFile& operator=(const SourceFile&) = delete;

    SourceFile(SourceFile&& rhs) noexcept
    {
        this->info = std::exchange(rhs.info, nullptr);
    }

    SourceFile& operator=(SourceFile&& rhs) noexcept
    {
        this->info = std::exchange(rhs.info, nullptr);
        return *this;
    }

    /// Gets the null-terminated sequence of characters of the source file.
    auto code_data() const -> const char* { return info->data.get(); }

    /// Returns the size (in bytes) of the source code.
    auto code_size() const -> size_t { return info->file_length; }

    /// Returns a string view to the source code.
    auto code_view() const -> std::string_view
    {
        return std::string_view(code_data(), code_size());
    }

    /// Gets the source location of a given character.
    auto location_of(const char* cp) const -> SourceLocation
    {
        return info->start_loc + (cp - code_data());
    }

    /// Gets the source location of a given character view.
    auto location_of(std::string_view view) const -> SourceLocation
    {
        return location_of(view.data());
    }

    /// Gets a string view to a source range.
    auto view_of(SourceRange range) const -> std::string_view
    {
        const auto* const begin = code_data() + (range.begin - info->start_loc);
        return std::string_view(begin, range.end - range.begin);
    }

protected:
    friend class SourceManager;

    explicit SourceFile(SourceManager::SourceInfo& info) : info(&info)
    {
        ++this->info->refcount;
    }

private:
    SourceManager::SourceInfo* info;
};

// TODO this manager does not perform any paging at the present moment.
//      first we have to analyze the access patterns of the source data
//      to define a proper paging scheme.

// TODO given the usage of this for things other than source code, it
// should be renamed to something else like FileManager (maybe even
// split into a SourceManager class that takes FileManager as input)
// remember to rename methods such as code_data to e.g. data
} // namespace gta3sc

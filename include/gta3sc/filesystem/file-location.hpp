#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>

// TODO remove dependency of <algorithm> in this header

namespace gta3sc::filesystem
{
/// Opaque handle to a location in a file.
///
/// Used most of the time to represent locations in source files.
///
/// Valid only for the \ref FilePool that issued it; values from
/// one instance are meaningless when presented to another.
///
/// Zero value is reserved as \ref no_file_loc. No loaded file range includes
/// it.
enum class FileLoc : uint32_t
{
};

/// Opaque handle to a range of bytes / characters in a file.
struct FileRange
{
public:
    /// First location in the range.
    FileLoc begin{};
    /// One location past the last in the range.
    FileLoc end{};

public:
    FileRange() noexcept = default;

    /// Constructs the range [begin, end).
    FileRange(FileLoc begin, FileLoc end) noexcept;

    /// Constructs the range starting at begin with the given byte length.
    FileRange(FileLoc begin, std::ptrdiff_t len) noexcept;

    /// Returns the number of bytes in this range.
    [[nodiscard]] auto size() const noexcept -> size_t;

    /// Returns a subrange of this range.
    ///
    /// \param offset the position to start the new range from this range.
    /// \param count the length of the new range.
    [[nodiscard]] auto subrange(
            size_t offset,
            size_t count = static_cast<size_t>(-1)) const noexcept -> FileRange;

    friend auto operator==(const FileRange&,
                           const FileRange&) noexcept -> bool = default;
};

static_assert(std::is_trivially_copyable_v<FileLoc>);
static_assert(std::is_trivially_copyable_v<FileRange>);

/// Represents no file location.
///
/// Guaranteed to equal default construction of \ref FileLoc.
inline constexpr FileLoc no_file_loc{};

/// Represents no file range.
///
/// Guaranteed to equal default construction of \ref FileRange.
inline constexpr FileRange no_file_range{};

/// Advances lhs by rhs bytes.
inline auto operator+(FileLoc lhs, std::ptrdiff_t rhs) noexcept -> FileLoc
{
    return FileLoc{static_cast<uint32_t>(static_cast<uint32_t>(lhs) + rhs)};
}

/// Advances lhs in place by rhs bytes.
inline auto operator+=(FileLoc& lhs, std::ptrdiff_t rhs) noexcept -> FileLoc&
{
    lhs = lhs + rhs;
    return lhs;
}

/// Retreats lhs by rhs bytes.
inline auto operator-(FileLoc lhs, std::ptrdiff_t rhs) noexcept -> FileLoc
{
    return FileLoc{static_cast<uint32_t>(static_cast<uint32_t>(lhs) - rhs)};
}

/// Retreats lhs in place by rhs bytes.
inline auto operator-=(FileLoc& lhs, std::ptrdiff_t rhs) noexcept -> FileLoc&
{
    lhs = lhs - rhs;
    return lhs;
}

/// Signed byte distance from rhs to lhs.
inline auto operator-(FileLoc lhs, FileLoc rhs) noexcept -> std::ptrdiff_t
{
    return static_cast<std::ptrdiff_t>(static_cast<uint32_t>(lhs))
           - static_cast<std::ptrdiff_t>(static_cast<uint32_t>(rhs));
}

/// Pre-increments lhs by one byte.
inline auto operator++(FileLoc& lhs) noexcept -> FileLoc&
{
    lhs += 1;
    return lhs;
}

/// Pre-decrements lhs by one byte.
inline auto operator--(FileLoc& lhs) noexcept -> FileLoc&
{
    lhs -= 1;
    return lhs;
}

/// Post-increments lhs by one byte; returns the prior value.
inline auto operator++(FileLoc& lhs, int) noexcept -> FileLoc
{
    const auto temp = lhs;
    ++lhs;
    return temp;
}

/// Post-decrements lhs by one byte; returns the prior value.
inline auto operator--(FileLoc& lhs, int) noexcept -> FileLoc
{
    const auto temp = lhs;
    --lhs;
    return temp;
}

inline FileRange::FileRange(FileLoc begin, FileLoc end) noexcept :
    begin(begin), end(end)
{}

inline FileRange::FileRange(FileLoc begin, std::ptrdiff_t len) noexcept :
    FileRange(begin, begin + len)
{}

inline auto FileRange::size() const noexcept -> size_t
{
    return end - begin;
}

inline auto FileRange::subrange(size_t offset,
                                size_t count) const noexcept -> FileRange
{
    offset = std::min(offset, this->size());
    count = std::min(count, this->size() - offset);
    return {this->begin + static_cast<std::ptrdiff_t>(offset),
            static_cast<std::ptrdiff_t>(count)};
}

} // namespace gta3sc::filesystem

namespace gta3sc
{
using filesystem::FileLoc;
using filesystem::FileRange;
using filesystem::no_file_loc;
using filesystem::no_file_range;
} // namespace gta3sc

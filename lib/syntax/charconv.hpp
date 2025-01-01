#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <system_error>

namespace gta3sc::util
{
// This is a placeholder implementation of std::from_chars.
//
// It is very limited in capability (limited to our requirements), and is
// intended to be replaced directly with <charconv> once it is fully available.
//
// See https://en.cppreference.com/w/cpp/utility/from_chars for details.

// NOLINTNEXTLINE(readability-identifier-naming): `std::` clone.
struct from_chars_result
{
    const char* ptr;
    std::errc ec;
};

// NOLINTNEXTLINE(readability-identifier-naming): `std::` clone.
enum class chars_format : uint8_t
{
    fixed = 1,
};

inline auto from_chars(const char* first, const char* last, int32_t& value,
                       int base = 10) -> from_chars_result
{
    // We assume `[first, last)` is a valid integer literal. This is true
    // for our use case which happens after the compiler performs lexing.

    // We also do not support (nor require) other radices.
    assert(base == 10);

    // NOLINTNEXTLINE(google-runtime-int): Allow long because of strtol
    constexpr long min_value = (-2147483647 - 1);
    constexpr long max_value = 2147483647; // NOLINT

    static_assert(std::numeric_limits<long>::min() <= min_value      // NOLINT
                  && std::numeric_limits<long>::max() >= max_value); // NOLINT

    char buffer[64];
    const auto length = std::min<size_t>(last - first, std::size(buffer) - 1);

    std::memcpy(buffer, first, length);
    buffer[length] = '\0';

    errno = 0;
    char* endptr = nullptr;
    const auto result = std::strtol(buffer, &endptr, base);

    if(errno == ERANGE || result < min_value || result > max_value)
        return from_chars_result{endptr, std::errc::result_out_of_range};
    else if(errno == EINVAL)
        return from_chars_result{first, std::errc::invalid_argument};
    else
        assert(errno == 0);

    value = result;
    return from_chars_result{endptr, std::errc()};
}

inline auto from_chars(const char* first, const char* last, float& value,
                       [[maybe_unused]] chars_format fmt) -> from_chars_result
{
    // We assume `[first, last)` is a valid floating point literal.

    // We do not support (nor require) other floating point formats.
    assert(fmt == chars_format::fixed);

    char buffer[128];
    const auto length = std::min<size_t>(last - first, std::size(buffer) - 1);

    std::memcpy(buffer, first, length);
    buffer[length] = '\0';

    errno = 0;
    char* endptr = nullptr;
    const auto result = std::strtof(buffer, &endptr);

    if(errno == ERANGE)
        return from_chars_result{endptr, std::errc::result_out_of_range};
    else if(errno == EINVAL)
        return from_chars_result{first, std::errc::invalid_argument};
    else
        assert(errno == 0);

    value = result;
    return from_chars_result{endptr, std::errc()};
}
} // namespace gta3sc::util

#pragma once
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <system_error>
#include <algorithm>

namespace gta3sc::utils
{
// This is a placeholder implementation of std::from_chars.
//
// It is very limited in capability (limited to our requirements), and is
// intended to be replaced directly with <charconv> once it is fully available.
//
// See https://en.cppreference.com/w/cpp/utility/from_chars for details.

struct from_chars_result
{
    const char* ptr;
    std::errc ec;
};

enum class chars_format
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

    constexpr long min_value = (-2147483647 - 1), max_value = 2147483647;

    static_assert(std::numeric_limits<long>::min() <= min_value
                  && std::numeric_limits<long>::max() >= max_value);

    char buffer[64];
    const auto length = std::min<size_t>(last - first, std::size(buffer) - 1);

    std::memcpy(buffer, first, length);
    buffer[length] = '\0';

    errno = 0;
    char* endptr = 0;
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
                       chars_format fmt) -> from_chars_result
{
    // We assume `[first, last)` is a valid floating point literal.

    // We do not support (nor require) other floating point formats.
    assert(fmt == chars_format::fixed);

    char buffer[128];
    const auto length = std::min<size_t>(last - first, std::size(buffer) - 1);

    std::memcpy(buffer, first, length);
    buffer[length] = '\0';

    errno = 0;
    char* endptr = 0;
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
}

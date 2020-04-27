#pragma once
#include <cstring>
#include <type_traits>

namespace gta3sc::util
{
// This is a placeholder implementation of std::bit_cast and is intended to
// be replaced with <bit> once it is fully available.
//
// See https://en.cppreference.com/w/cpp/numeric/bit_cast for details.
template<typename To, typename From>
constexpr auto bit_cast(const From& src) noexcept -> To
{
    static_assert(
            sizeof(To) == sizeof(From)
            && std::is_trivially_copyable_v<From> && std::is_trivial_v<To>);
    To dst;
    std::memcpy(&dst, &src, sizeof(dst));
    return dst;
}
}

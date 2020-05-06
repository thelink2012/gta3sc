#pragma once
#include <gta3sc/util/ctype.hpp>
#include <string>
#include <string_view>

namespace gta3sc::util
{
/// Checks whether two strings are equal ignoring any difference of casing.
constexpr auto insensitive_equal(std::string_view lhs,
                                 std::string_view rhs) noexcept -> bool
{
    if(lhs.size() != rhs.size())
        return false;

    for(auto lhs_it = lhs.begin(), rhs_it = rhs.begin(), lhs_end = lhs.end();
        lhs_it != lhs_end; ++lhs_it, ++rhs_it)
    {
        if(util::toupper(*lhs_it) != util::toupper(*rhs_it))
            return false;
    }

    return true;
}
} // namespace gta3sc::util

#pragma once
#include <ostream>
#include <vector>

// TODO stop including overloads in namespace std (UB)
// TODO migrate from ostream to std::format
// https://github.com/onqtam/doctest/blob/master/doc/markdown/stringification.md

namespace std // NOLINT(cert-dcl58-cpp): FIXME
{
inline auto operator<<(std::ostream& os, std::byte byte_val) -> std::ostream&
{
    // FIXME not exception safe (needs gsl.finally)
    const auto old_flags = os.flags();
    os << std::hex << std::showbase << static_cast<int>(byte_val);
    os.flags(old_flags);
    return os;
}

template<typename T>
inline auto operator<<(std::ostream& os, const std::vector<T>& v)
        -> std::ostream&
{
    os << "{";
    for(size_t i = 0; i < v.size(); ++i)
        os << v[i] << (i + 1 == v.size() ? "" : ", ");
    os << "}";
    return os;
}
} // namespace std

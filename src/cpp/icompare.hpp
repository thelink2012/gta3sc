/// Case-Insensitive Compare
///
#pragma once
#include <string>
#include <cstring>

#if defined(_MSC_VER)
inline int strcasecmp(const char* a, const char* b)
{
    return _stricmp(a, b);
}

inline int strncasecmp(const char* a, const char* b, size_t count)
{
    return _strnicmp(a, b, count);
}
#elif defined (__unix__) || (__MINGW32__) || (defined (__APPLE__) && defined (__MACH__))
    // Already has strcasecmp and strncasecmp
#else
#   error
#endif

/// std::less<void> but case insensitive
struct iless
{
    using is_transparent = int;

    bool operator()(const string_view& left, const string_view& right) const
    {
        auto ans = strncasecmp(left.data(), right.data(), (std::min)(left.size(), right.size()));
        return ans == 0? (left.size() < right.size()) : (ans < 0);
    }
};

/// std::equal_to<void> but case insensitive
struct iequal_to
{
    using is_transparent = int;

    bool operator()(const string_view& left, const string_view& right) const
    {
        if(left.size() != right.size())
            return false;
        return strncasecmp(left.data(), right.data(), left.size()) == 0;
    }
};

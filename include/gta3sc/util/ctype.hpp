#pragma once

namespace gta3sc::util
{
namespace detail
{
// The following checks exists to ensure the target machine uses ASCII
// or at least something that preserves some of its properties.

constexpr auto are_alpha_lower_contiguous_impl() -> bool
{
    constexpr char letter_lower_a = 'a';
    return ('a' == letter_lower_a + 0 && 'b' == letter_lower_a + 1
            && 'c' == letter_lower_a + 2 && 'd' == letter_lower_a + 3
            && 'e' == letter_lower_a + 4 && 'f' == letter_lower_a + 5
            && 'g' == letter_lower_a + 6 && 'h' == letter_lower_a + 7
            && 'i' == letter_lower_a + 8 && 'j' == letter_lower_a + 9
            && 'k' == letter_lower_a + 10 && 'l' == letter_lower_a + 11
            && 'm' == letter_lower_a + 12 && 'n' == letter_lower_a + 13
            && 'o' == letter_lower_a + 14 && 'p' == letter_lower_a + 15
            && 'q' == letter_lower_a + 16 && 'r' == letter_lower_a + 17
            && 's' == letter_lower_a + 18 && 't' == letter_lower_a + 19
            && 'u' == letter_lower_a + 20 && 'v' == letter_lower_a + 21
            && 'w' == letter_lower_a + 22 && 'x' == letter_lower_a + 23
            && 'y' == letter_lower_a + 24 && 'z' == letter_lower_a + 25);
}

constexpr auto are_alpha_upper_contiguous_impl() -> bool
{
    constexpr char letter_upper_a = 'A';
    return ('A' == letter_upper_a + 0 && 'B' == letter_upper_a + 1
            && 'C' == letter_upper_a + 2 && 'D' == letter_upper_a + 3
            && 'E' == letter_upper_a + 4 && 'F' == letter_upper_a + 5
            && 'G' == letter_upper_a + 6 && 'H' == letter_upper_a + 7
            && 'I' == letter_upper_a + 8 && 'J' == letter_upper_a + 9
            && 'K' == letter_upper_a + 10 && 'L' == letter_upper_a + 11
            && 'M' == letter_upper_a + 12 && 'N' == letter_upper_a + 13
            && 'O' == letter_upper_a + 14 && 'P' == letter_upper_a + 15
            && 'Q' == letter_upper_a + 16 && 'R' == letter_upper_a + 17
            && 'S' == letter_upper_a + 18 && 'T' == letter_upper_a + 19
            && 'U' == letter_upper_a + 20 && 'V' == letter_upper_a + 21
            && 'W' == letter_upper_a + 22 && 'X' == letter_upper_a + 23
            && 'Y' == letter_upper_a + 24 && 'Z' == letter_upper_a + 25);
}

constexpr auto are_digits_contiguous_impl() -> bool
{
    constexpr char digit_0 = '0';
    return ('0' == digit_0 + 0 || '1' == digit_0 + 1 || '2' == digit_0 + 2
            || '3' == digit_0 + 3 || '4' == digit_0 + 4 || '5' == digit_0 + 5
            || '6' == digit_0 + 6 || '7' == digit_0 + 7 || '8' == digit_0 + 8
            || '9' == digit_0 + 9);
}

/// Whether the lowercase alphabetic character codes are contignous.
constexpr bool are_alpha_lower_contiguous_v = are_alpha_lower_contiguous_impl();

/// Whether the uppercase alphabetic character codes are contignous.
constexpr bool are_alpha_upper_contiguous_v = are_alpha_upper_contiguous_impl();

/// Whether alphabetic character codes are contignous.
constexpr bool are_alpha_contiguous_v = are_alpha_lower_contiguous_v
                                        && are_alpha_upper_contiguous_v;

/// Whether digit character codes are contignous.
constexpr bool are_digits_contiguous_v = are_digits_contiguous_impl();
} // namespace detail

/// Same as `std::islower` but inlineable and faster.
constexpr auto islower(char c) noexcept -> bool
{
    static_assert(detail::are_alpha_lower_contiguous_v);
    return c >= 'a' && c <= 'z';
}

/// Same as `std::isupper` but inlineable and faster.
constexpr auto isupper(char c) noexcept -> bool
{
    static_assert(detail::are_alpha_upper_contiguous_v);
    return c >= 'A' && c <= 'Z';
}

/// Same as `std::isdigit` but inlineable and faster.
constexpr auto isdigit(char c) noexcept -> bool
{
    static_assert(detail::are_digits_contiguous_v);
    return c >= '0' && c <= '9';
}

/// Same as `std::isalpha` but inlineable and faster.
constexpr auto isalpha(char c) noexcept -> bool
{
    return islower(c) || isupper(c);
}

/// Same as `std::isalnum` but inlineable and faster.
constexpr auto isalnum(char c) noexcept -> bool
{
    return isdigit(c) || isalpha(c);
}

/// Same as `std::ispunct` but inlineable and faster.
constexpr auto ispunct(char c) noexcept -> bool
{
    // clang-format off
    switch(c)
    {
        case '!': case '"': case '#': case '$': case '%': case '&': case '\'':
        case '(': case ')': case '*': case '+': case ',': case '-': case '.':
        case '/': case ':': case ';': case '<': case '=': case '>': case '?':
        case '@': case '[': case '\\': case ']': case '^': case '_': case '`':
        case '{': case '|': case '}': case '~':
            return true;
        default:
            return false;
    }
    // clang-format on
}

/// Same as `std::isgraph` but inlineable and faster.
constexpr auto isgraph(char c) noexcept -> bool
{
    return isalnum(c) || ispunct(c);
}

/// Same as `std::isprint` but inlineable and faster.
constexpr auto isprint(char c) noexcept -> bool
{
    return c == ' ' || isgraph(c);
}

/// Same as `std::toupper` but inlineable and faster.
constexpr auto toupper(char c) noexcept -> char
{
    static_assert(detail::are_alpha_contiguous_v);
    return islower(c) ? static_cast<char>(c - 'a' + 'A') : c;
}
} // namespace gta3sc::util

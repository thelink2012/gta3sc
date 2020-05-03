#pragma once
#include <iosfwd>
#include <string_view>
#include <type_traits>

namespace gta3sc::util
{
/// Used to make new types on top of `std::string_view`.
///
/// This is a `std::string_view` wrapper with implicit conversion to
/// `std::string_view` (hence weak).
template<typename Tag>
class WeakStringVieweable
{
public:
    constexpr WeakStringVieweable() noexcept = default;

    explicit constexpr WeakStringVieweable(std::string_view value) noexcept :
        m_value(value)
    {}

    // NOLINTNEXTLINE: Implicit conversion is the intent of this.
    constexpr operator std::string_view() const noexcept { return m_value; }

    friend constexpr auto operator==(const WeakStringVieweable& lhs,
                                     const WeakStringVieweable& rhs) noexcept
            -> bool
    {
        return lhs.m_value == rhs.m_value;
    }

    friend constexpr auto operator!=(const WeakStringVieweable& lhs,
                                     const WeakStringVieweable& rhs) noexcept
            -> bool
    {
        return !(lhs == rhs);
    }

    // TODO operator<=>

    friend constexpr auto operator<<(std::ostream& os,
                                     const WeakStringVieweable& rhs)
            -> std::ostream&
    {
        return os << rhs.m_value;
    }

private:
    std::string_view m_value;
};
} // namespace gta3sc::util

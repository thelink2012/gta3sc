#pragma once
#include <iterator>
#include <type_traits>

namespace gta3sc::util::iterator_adaptor
{
/// An iterator adaptor for `Iter` using an `Adaptor` traits.
///
/// An iterator adaptor is an iterator that wraps and modifies the behaviour of
/// the iterator it encapsulates. For example, `std::reverse_iterator` is an
/// iterator adaptor that reverses the direction of iteration of a given
/// iterator.
///
/// This is a helper to create adaptors without the need for re-implementing
/// an iterator entirely. The `Adaptor` type specifies how the adaptation
/// should be done. See `IdentityAdaptor` and `DereferenceAdaptor` for examples.
template<typename Iter, typename Adaptor>
class Iterator
{
public:
    using iterator_type = Iter;
    using difference_type = typename Adaptor::difference_type;
    using iterator_category = typename Adaptor::iterator_category;
    using reference = typename Adaptor::reference;
    using pointer = typename Adaptor::pointer;
    using value_type = typename Adaptor::value_type;

public:
    constexpr Iterator() = default;

    constexpr explicit Iterator(iterator_type x) : inner_it(std::move(x)) {}

    constexpr Iterator(const Iterator&) = default;
    constexpr auto operator=(const Iterator&) -> Iterator& = default;

    constexpr Iterator(Iterator&&) noexcept = default;
    constexpr auto operator=(Iterator&&) noexcept -> Iterator& = default;

    constexpr ~Iterator() = default;

    /// Enable conversions possible in the inner iterator in this adaptor.
    template<typename U> // NOLINTNEXTLINE: Intentional implicit conversion.
    constexpr Iterator(const Iterator<U, Adaptor>& other) :
        inner_it(other.inner_it)
    {}

    template<typename U>
    constexpr auto operator=(const Iterator<U, Adaptor>& other) -> Iterator&
    {
        inner_it = other;
        return *this;
    }

    [[nodiscard]] constexpr auto base() const -> iterator_type
    {
        return inner_it;
    }

    constexpr auto operator*() const -> reference
    {
        return adaptor.dereference(inner_it);
    }

    constexpr auto operator->() const -> pointer
    {
        return std::addressof(operator*());
    }

    constexpr decltype(auto)
    operator[](difference_type n) const /* -> unspecified */
    {
        return adaptor.at(inner_it, n);
    }

    constexpr auto operator++() -> Iterator&
    {
        adaptor.increment(inner_it);
        return *this;
    }

    constexpr auto operator++(int) -> Iterator
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    constexpr auto operator--() -> Iterator&
    {
        adaptor.decrement(inner_it);
        return *this;
    }

    constexpr auto operator--(int) -> Iterator
    {
        auto temp = *this;
        --(*this);
        return temp;
    }

    constexpr auto operator+=(difference_type n) -> Iterator&
    {
        adaptor.advance_forward(inner_it, n);
        return *this;
    }

    constexpr auto operator-=(difference_type n) -> Iterator&
    {
        adaptor.advance_backward(inner_it, n);
        return *this;
    }

    constexpr auto operator+(difference_type n) const -> Iterator
    {
        auto temp = *this;
        temp += n;
        return temp;
    }

    constexpr auto operator-(difference_type n) const -> Iterator
    {
        auto temp = *this;
        temp -= n;
        return temp;
    }

    constexpr friend auto operator==(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it == rhs.inner_it;
    }

    constexpr friend auto operator!=(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it != rhs.inner_it;
    }

    constexpr friend auto operator<=(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it <= rhs.inner_it;
    }

    constexpr friend auto operator>=(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it >= rhs.inner_it;
    }

    constexpr friend auto operator<(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it <= rhs.inner_it;
    }

    constexpr friend auto operator>(const Iterator& lhs, const Iterator& rhs)
            -> bool
    {
        return lhs.inner_it >= rhs.inner_it;
    }

    constexpr friend auto operator+(difference_type n, const Iterator& rhs)
            -> Iterator
    {
        return rhs + n;
    }

    template<typename Iter1, typename Iter2>
    constexpr friend auto operator-(const Iterator<Iter1, Adaptor>& lhs,
                                    const Iterator<Iter2, Adaptor>& rhs)
            -> decltype(lhs.base() - rhs.base())
    {
        return lhs.base() - rhs.base();
    }

private:
    [[no_unique_address]] Adaptor adaptor;
    Iter inner_it;
};

/// An adaptor which behaves like the original iterator.
template<typename Iter>
struct IdentityAdaptor
{
    using difference_type =
            typename std::iterator_traits<Iter>::difference_type;

    using iterator_category =
            typename std::iterator_traits<Iter>::iterator_category;

    using reference = typename std::iterator_traits<Iter>::reference;

    using pointer = typename std::iterator_traits<Iter>::pointer;

    using value_type = typename std::iterator_traits<Iter>::value_type;

    [[nodiscard]] constexpr auto dereference(const Iter& it) const -> reference
    {
        return *it;
    }

    [[nodiscard]] constexpr auto
    at(const Iter& it, difference_type n) const /* -> unspecified */
    {
        return it[n];
    }

    constexpr void increment(Iter& it) const { ++it; }

    constexpr void decrement(Iter& it) const { --it; }

    constexpr void advance_forward(Iter& it, difference_type n) const
    {
        it += n;
    }

    constexpr void advance_backward(Iter& it, difference_type n) const
    {
        it -= n;
    }
};

/// An adaptor which performs dereference on the original iterator value.
template<typename Iter>
struct DereferenceAdaptor : IdentityAdaptor<Iter>
{
    using typename IdentityAdaptor<Iter>::difference_type;

    using typename IdentityAdaptor<Iter>::iterator_category;

    using reference = std::remove_pointer_t<std::remove_reference_t<
            typename IdentityAdaptor<Iter>::reference>>&;

    using pointer
            = std::remove_pointer_t<typename IdentityAdaptor<Iter>::pointer>;

    using value_type
            = std::remove_pointer_t<typename IdentityAdaptor<Iter>::value_type>;

    [[nodiscard]] constexpr auto dereference(const Iter& it) const -> reference
    {
        return **it;
    }

    [[nodiscard]] constexpr decltype(auto)
    at(const Iter& it, difference_type n) const /* -> unspecified */
    {
        return *it[n];
    }
};
} // namespace gta3sc::util::iterator_adaptor

#pragma once
#include <gta3sc/util/iterator-adaptor.hpp>
#include <utility>

namespace gta3sc::util::iterator_adaptor
{
namespace detail
{
/// Copies the `const` and `volatile` specifiers from `OriginalType` into `T`.
template<typename OriginalType, typename T>
struct restore_cv // NOLINT: Naming convention mimics <type_traits>
{
    using type = T;
};

template<typename OriginalType, typename T>
struct restore_cv<const OriginalType, T>
{
    using type = std::add_const_t<T>;
};

template<typename OriginalType, typename T>
struct restore_cv<volatile OriginalType, T>
{
    using type = std::add_volatile_t<T>;
};

template<typename OriginalType, typename T>
struct restore_cv<const volatile OriginalType, T>
{
    using type = std::add_cv_t<T>;
};

template<typename OriginalType, typename T>
using restore_cv_t = typename restore_cv<OriginalType, T>::type;
} // namespace detail

/// Defines an adaptor (see inner class) that performs `get<I>` on the original
/// iterator value.
template<size_t I>
struct ElementAdaptor
{
    template<typename Iter>
    using NextAdaptor = IdentityAdaptor<Iter>;

    template<typename Iter>
    struct Adaptor : NextAdaptor<Iter>
    {
        using typename NextAdaptor<Iter>::difference_type;

        using typename NextAdaptor<Iter>::iterator_category;

        using value_type = std::tuple_element_t<
                I, typename NextAdaptor<Iter>::value_type>;

        using reference = detail::restore_cv_t<
                std::remove_reference_t<typename NextAdaptor<Iter>::reference>,
                std::tuple_element_t<
                        I, std::remove_reference_t<
                                   typename NextAdaptor<Iter>::reference>>>&;

        using pointer = detail::restore_cv_t<
                std::remove_pointer_t<typename NextAdaptor<Iter>::pointer>,
                std::tuple_element_t<
                        I, std::remove_pointer_t<
                                   typename NextAdaptor<Iter>::pointer>>>*;

        [[nodiscard]] constexpr auto dereference(const Iter& it) const
                -> reference
        {
            return get<I>(*it);
        }

        [[nodiscard]] constexpr decltype(auto)
        at(const Iter& it, difference_type n) const /* -> unspecified */
        {
            return get<I>(*it[n]);
        }
    };
};

/// Defines an adaptor (see inner class) that dereferences the value of `get<I>`
/// on the original iterator value.
template<size_t I>
struct DereferenceElementAdaptor
{
    template<typename Iter>
    using NextAdaptor = typename ElementAdaptor<I>::template Adaptor<Iter>;

    template<typename Iter>
    struct Adaptor : NextAdaptor<Iter>
    {
        using typename NextAdaptor<Iter>::difference_type;

        using typename NextAdaptor<Iter>::iterator_category;

        using reference = std::remove_pointer_t<std::remove_reference_t<
                typename NextAdaptor<Iter>::reference>>&;

        using pointer
                = std::remove_pointer_t<typename NextAdaptor<Iter>::pointer>;

        using value_type
                = std::remove_pointer_t<typename NextAdaptor<Iter>::value_type>;

        [[nodiscard]] constexpr auto dereference(const Iter& it) const
                -> reference
        {
            return *NextAdaptor<Iter>::dereference(it);
        }

        [[nodiscard]] constexpr decltype(auto)
        at(const Iter& it, difference_type n) const /* -> unspecified */
        {
            return *NextAdaptor<Iter>::at(it, n);
        }
    };
};

/// An adaptor which takes the first element of a pair-like object.
template<typename Iter>
using KeyAdaptor = ElementAdaptor<0>::Adaptor<Iter>;

/// An adaptor which takes the second element of a pair-like object.
template<typename Iter>
using ValueAdaptor = ElementAdaptor<1>::Adaptor<Iter>;

/// An adaptor which dereferences the first element of a pair-like object.
template<typename Iter>
using DereferenceKeyAdaptor = DereferenceElementAdaptor<0>::Adaptor<Iter>;

/// An adaptor which dereferences the second element of a pair-like object.
template<typename Iter>
using DereferenceValueAdaptor = DereferenceElementAdaptor<1>::Adaptor<Iter>;
} // namespace gta3sc::util::iterator_adaptor

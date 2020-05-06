#pragma once
#include <gta3sc/util/iterator-adaptor.hpp>

namespace gta3sc::util
{
/// A view into a container.
template<typename Container, template<typename> typename IteratorAdaptor
                             = iterator_adaptor::IdentityAdaptor>
class ContainerView
{
private:
    using container_const_iterator = decltype(
            std::cbegin(std::declval<Container>()));

public:
    using iterator = iterator_adaptor::Iterator<
            container_const_iterator,
            IteratorAdaptor<container_const_iterator>>;

    using value_type = typename std::iterator_traits<iterator>::value_type;
    using pointer = typename std::iterator_traits<iterator>::pointer;
    using const_pointer = const pointer;
    using reference = typename std::iterator_traits<iterator>::reference;
    using const_reference = const reference;
    using difference_type =
            typename std::iterator_traits<iterator>::difference_type;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using size_type = decltype(std::size(std::declval<Container>()));

public:
    /// Constructs an empty view.
    constexpr ContainerView() noexcept = default;

    /// Constructs a view to the specified container.
    constexpr explicit ContainerView(const Container& container) noexcept :
        m_container(&container)
    {}

    /// Returns the number of elements in the view.
    [[nodiscard]] constexpr auto size() const -> size_type
    {
        return m_container ? std::size(*m_container) : 0;
    }

    /// Checks whether there is any element in the view.
    [[nodiscard]] constexpr auto empty() const -> size_type
    {
        return std::empty(*m_container);
    }

    /// Returns the first element of the view.
    [[nodiscard]] constexpr auto front() const { return *begin(); }

    /// Returns the last element in the view.
    [[nodiscard]] constexpr auto back() const { return *std::prev(end()); }

    /// Returns an iterator to the beggining of the view.
    [[nodiscard]] constexpr auto begin() const -> iterator
    {
        return m_container ? iterator(std::cbegin(*m_container)) : iterator();
    }

    /// Returns an iterator to the end of the view.
    [[nodiscard]] constexpr auto end() const -> iterator
    {
        return m_container ? iterator(std::cend(*m_container)) : iterator();
    }

    /// Returns a reverse iterator to the beggining of the view.
    [[nodiscard]] constexpr auto rbegin() const
    {
        return reverse_iterator(begin());
    }

    /// Returns a reverse iterator to the end of the view.
    [[nodiscard]] constexpr auto rend() const
    {
        return reverse_iterator(end());
    }

private:
    const Container* m_container{};
};
} // namespace gta3sc::util

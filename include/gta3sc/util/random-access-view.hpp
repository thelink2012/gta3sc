#pragma once
#include <gta3sc/util/iterator-adaptor.hpp>

namespace gta3sc::util
{
/// A view into a range that provides indexing in constant time.
template<typename T, template<typename> typename IteratorAdaptor
                     = iterator_adaptor::IdentityAdaptor>
class RandomAccessView
{
public:
    using iterator
            = iterator_adaptor::Iterator<const T*, IteratorAdaptor<const T*>>;

    using value_type = typename std::iterator_traits<iterator>::value_type;
    using pointer = typename std::iterator_traits<iterator>::pointer;
    using const_pointer = const pointer;
    using reference = typename std::iterator_traits<iterator>::reference;
    using const_reference = const reference;
    using difference_type =
            typename std::iterator_traits<iterator>::difference_type;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using size_type = size_t;

public:
    /// Constructs an empty view.
    constexpr RandomAccessView() noexcept = default;

    /// Constructs a view to the range `[begin, begin + count)`.
    template<typename RandomAccessIterator>
    constexpr RandomAccessView(RandomAccessIterator begin,
                               size_type count) noexcept :
        RandomAccessView(begin, begin + count)
    {}

    /// Constructs a view to the range `[begin, end)`.
    template<typename RandomAcessIterator>
    constexpr RandomAccessView(RandomAcessIterator begin,
                               RandomAcessIterator end) noexcept :
        m_begin(begin), m_end(end)
    {}

    /// Returns the number of elements in the view.
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    {
        return end() - begin();
    }

    /// Checks whether there is any element in the view.
    [[nodiscard]] constexpr auto empty() const noexcept -> size_type
    {
        return size() == 0;
    }

    /// Returns the first element of the view.
    [[nodiscard]] constexpr auto front() const noexcept { return *begin(); }

    /// Returns the last element in the view.
    [[nodiscard]] constexpr auto back() const noexcept
    {
        return *std::prev(end());
    }

    /// Returns an iterator to the beggining of the view.
    [[nodiscard]] constexpr auto begin() const noexcept -> iterator
    {
        return iterator(m_begin);
    }

    /// Returns an iterator to the end of the view.
    [[nodiscard]] constexpr auto end() const noexcept -> iterator
    {
        return iterator(m_end);
    }

    /// Returns a reverse iterator to the beggining of the view.
    [[nodiscard]] constexpr auto rbegin() const noexcept -> iterator
    {
        return reverse_iterator(begin());
    }

    /// Returns a reverse iterator to the end of the view.
    [[nodiscard]] constexpr auto rend() const noexcept -> iterator
    {
        return reverse_iterator(end());
    }

    /// Returns the n-th element of the view.
    constexpr auto operator[](difference_type n) noexcept
    {
        return *(begin() + n);
    }

private:
    iterator m_begin{};
    iterator m_end{};
};
} // namespace gta3sc::util

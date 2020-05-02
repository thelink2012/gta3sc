#pragma once
#include <algorithm>
#include <cassert>
#include <gta3sc/util/intrusive-bidirectional-list-node.hpp>

namespace gta3sc::util
{
/// A intrusive doubly-linked list container.
///
/// This is container of `IntrusiveBidirectionalListNode` elements. It does not
/// perform any memory allocation whatsoever and just manipulates the list
/// by using `algorithm::circular_list`.
///
/// This container does not own any of the elements it contains! Therefore users
/// must still manage their lifetime and guarantee that they live as long as
/// they are present in the container.
///
/// The container provides a sentinel node for its linked elements, as such
/// users do not need to manually maintain a sentinel as required by
/// `IntrusiveBidirectionalListNode`.
///
/// Inserted elements may belong to a single container at once. Do not insert
/// a element present in a container into another. Additionally, the container
/// does not manage the circular list formed by its elements. Users may alter
/// that list without direct access to its container as long as the links are
/// kept in a consistent state.
///
/// The destruction of this container simply erases the sentinel node from the
/// circular list (transforming the list into a linear list). That is, the
/// linked list continues to exist.
///
/// The behaviour of the methods are similar to that of `std::list` except where
/// noted (as in the explanation above). The following methods are not available
/// due to the nature of this implementation:
///
///  * `size()` and `max_size()`: Due to the non-intrusive nature of this
/// container, the size of the list must be computed, in linear time, thus
/// we refrain from providing such implementation in face of the constant
/// time expectation from users.
///  * `resize()`: This container does not perform allocations.
///  * `emplace()`, `emplace_back()`, `emplace_front()`: This container is
/// not capable of creating elements.
///  * Overloads with `std::initializer_list<value_type>` are not provided
///  since that would require copying. Instead, `std::initializer_list<pointer>`
/// is used.
template<typename T>
class IntrusiveList
{
private:
    using node_type = IntrusiveBidirectionalListNode<T>;
    static_assert(std::is_base_of_v<node_type, T>);

public:
    using iterator = typename node_type::Iterator;
    using const_iterator = typename node_type::ConstIterator;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using size_type = typename iterator::size_type;
    using difference_type = typename iterator::difference_type;

    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

public:
    constexpr IntrusiveList() noexcept
    {
        algorithm::circular_list::init(sentinel);
    }

    /// Behaves the same as construction followed by `assign(first, last)`.
    template<typename InputIterator>
    constexpr IntrusiveList(InputIterator first, InputIterator last) noexcept :
        IntrusiveList()
    {
        assign(first, last);
    }

    /// Behaves the same as construction followed by
    /// `assign_pointer(ilist.begin(), ilist.end())`
    constexpr IntrusiveList(std::initializer_list<pointer> ilist) noexcept :
        IntrusiveList()
    {
        assign_pointer(ilist.begin(), ilist.end());
    }

    constexpr IntrusiveList(const IntrusiveList&) = delete;
    constexpr auto operator=(const IntrusiveList&) -> IntrusiveList& = delete;

    constexpr IntrusiveList(IntrusiveList&& other) noexcept : IntrusiveList()
    {
        other.swap(*this);
    }

    constexpr auto operator=(IntrusiveList&& other) noexcept -> IntrusiveList&
    {
        if(this == &other)
            return *this;

        clear();
        other.swap(*this);
        return *this;
    }

    constexpr ~IntrusiveList() noexcept { clear(); }

    //
    // Iterators
    //

    constexpr auto begin() noexcept -> iterator { return std::next(end()); }

    constexpr auto begin() const noexcept -> const_iterator
    {
        return std::next(end());
    }

    constexpr auto cbegin() const noexcept -> const_iterator { return begin(); }

    constexpr auto end() noexcept -> iterator { return iterator(sentinel); }

    constexpr auto end() const noexcept -> const_iterator
    {
        // The following const_cast is hacky, but what else can we do?
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_iterator(const_cast<node_type&>(sentinel));
    }

    constexpr auto cend() const noexcept -> const_iterator { return end(); }

    constexpr auto rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    constexpr auto rbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    constexpr auto crbegin() const noexcept -> const_reverse_iterator
    {
        return rbegin();
    }

    constexpr auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    constexpr auto rend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    constexpr auto crend() const noexcept -> const_reverse_iterator
    {
        return rend();
    }

    //
    // Element access
    //

    constexpr auto front() -> reference { return *begin(); }

    constexpr auto front() const -> const_reference { return *begin(); }

    constexpr auto back() -> reference { return *std::prev(end()); }

    constexpr auto back() const -> const_reference { return *std::prev(end()); }

    //
    // Capacity
    //

    constexpr auto empty() const noexcept -> bool
    {
        return algorithm::circular_list::empty(sentinel);
    }

    //
    // Modifiers
    //

    /// Clears the container.
    ///
    /// This removes the sentinel node from the circular list,
    /// effectively eliminating the relationship between the container
    /// and the list.
    ///
    /// The previously associated list becomes a linear list.
    constexpr void clear() noexcept
    {
        algorithm::circular_list::erase(sentinel);
        algorithm::circular_list::init(sentinel);
    }

    /// Inserts the given `node` before `pos`.
    ///
    /// The given node must not be part of any list.
    constexpr auto insert(const_iterator pos, reference node) noexcept
            -> iterator
    {
        algorithm::circular_list::insert(*pos.unconst(), node);
        return iterator(node);
    }

    /// Inserts the elements in the range `[first, last)` before `pos`.
    ///
    /// This is equivalent to calling `insert` on each of the elements
    /// in the sequence. As such, each of them, must not be part of any
    /// list.
    template<typename InputIterator>
    constexpr void insert(const_iterator pos, InputIterator first,
                          InputIterator last) noexcept
    {
        for(auto it = first; it != last; ++it)
            insert(pos, *it);
    }

    /// Behaves as if calling `clear()` followed by `insert(end(), first,
    /// last)`.
    template<typename InputIterator>
    constexpr void assign(InputIterator first, InputIterator last) noexcept
    {
        clear();
        insert(first, last);
    }

    /// Inserts the given node at the front of the container's list.
    constexpr void push_front(reference node) noexcept
    {
        insert(begin(), node);
    }

    /// Inserts the given node at the back of the container's list.
    constexpr void push_back(reference node) noexcept { insert(end(), node); }

    /// Unlinks the given element from the container and returns an iterator
    /// to the element following the removed one.
    constexpr auto erase(const_iterator pos) noexcept -> iterator
    {
        assert(pos != const_iterator(sentinel));
        return erase(pos, std::next(pos));
    }

    /// Unlinks the range `[first, last)` from the container and returns an
    /// iterator following the last element removed.
    constexpr auto erase(const_iterator first, const_iterator last) noexcept
            -> iterator
    {
        algorithm::circular_list::erase(*first.unconst(), *last.unconst());
        return last.unconst();
    }

    /// Erases the first element from the container's list.
    constexpr void pop_front() noexcept { erase(begin()); }

    /// Erases the last element from the container's list.
    constexpr void pop_back() noexcept { erase(std::prev(end())); }

    /// Swaps the container's list of this and `other`.
    constexpr void swap(IntrusiveList& other) noexcept
    {
        return algorithm::circular_list::swap(sentinel, other.sentinel);
    }

    //
    // Operations
    //

    /// Transfers the elements `[first, last)` from `other` into this.
    ///
    /// The elements are transfered to before `pos`.
    constexpr void splice(const_iterator pos,
                          [[maybe_unused]] IntrusiveList& other,
                          const_iterator other_first,
                          const_iterator other_last) noexcept

    {
        return algorithm::circular_list::splice(
                *pos.unconst(), *other_first.unconst(), *other_last.unconst());
    }

    /// Behaves as if calling `splice(pos, other, other_first, other_last)`
    /// followed by `other.clear()`.
    constexpr void splice(const_iterator pos, IntrusiveList&& other,
                          const_iterator other_first,
                          const_iterator other_last) noexcept

    {
        splice(pos, other, other_first, other_last);
        other.clear();
    }

    /// Behaves as if calling `splice(pos, other, other_it,
    /// std::next(other_it))`.
    constexpr void splice(const_iterator pos, IntrusiveList& other,
                          const_iterator other_it) noexcept
    {
        splice(pos, other, other_it, std::next(other_it));
    }

    /// Behaves as if calling `splice(pos, other, other_it)` followed by
    /// `other.clear()`.
    constexpr void splice(const_iterator pos, IntrusiveList&& other,
                          const_iterator other_it) noexcept
    {
        splice(pos, other, other_it);
        other.clear();
    }

    /// Behaves as if calling `splice(pos, other, other.begin(), other.end())`.
    constexpr void splice(const_iterator pos, IntrusiveList& other) noexcept
    {
        splice(pos, other, other.begin(), other.end());
        assert(other.empty());
    }

    /// Behaves as if calling `splice(pos, other)`.
    constexpr void splice(const_iterator pos, IntrusiveList&& other) noexcept
    {
        splice(pos, other);
        assert(other.empty());
    }

    /// Erase elements that compare equal to `value`.
    constexpr auto remove(const T& value) noexcept -> size_type
    {
        return remove_if([&](const T& iter_val) { return iter_val == value; });
    }

    /// Erases elements on which `pred(elem)` is true.
    template<typename UnaryPredicate>
    constexpr auto remove_if(UnaryPredicate pred) noexcept -> size_type
    {
        size_type count{};
        for(auto it = begin(); it != end();)
        {
            if(auto from_it = it++; pred(*from_it))
            {
                ++count;
                while(it != end() && pred(*it))
                {
                    ++it;
                    ++count;
                }
                it = erase(from_it, it);
            }
        }
        return count;
    }

    // TODO provide tests for this
    // TODO sort, unique, merge, reverse, non-member swap, erase, operator<=>

    //
    // Pointer version of a few methods
    //

    /// Same as `insert` except `InputIt::value_type` is `pointer`.
    template<typename InputIt>
    constexpr void insert_pointer(const_iterator pos, InputIt first,
                                  InputIt last) noexcept
    {
        for(auto it = first; it != last; ++it)
            insert(pos, **it);
    }

    /// Behaves as if calling `insert(pos, ilist.begin(), ilist.end())`.
    constexpr void insert_pointer(const_iterator pos,
                                  std::initializer_list<pointer> ilist) noexcept
    {
        return insert_pointer(pos, ilist.begin(), ilist.end());
    }

    /// Same as `assign` except `InputIt::value_type` is `pointer`.
    template<typename InputIt>
    constexpr void assign_pointer(InputIt first, InputIt last) noexcept
    {
        clear();
        insert_pointer(end(), first, last);
    }

    /// Behaves as if calling `assign_pointer(ilist.begin(), ilist.end())`.
    constexpr void assign_pointer(std::initializer_list<pointer> ilist) noexcept
    {
        return assign_pointer(ilist.begin(), ilist.end());
    }

private:
    node_type sentinel;
};

template<typename T>
constexpr auto operator==(const IntrusiveList<T>& lhs,
                          const IntrusiveList<T>& rhs) noexcept -> bool
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename T>
constexpr auto operator!=(const IntrusiveList<T>& lhs,
                          const IntrusiveList<T>& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}
} // namespace gta3sc::util

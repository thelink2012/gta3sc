#pragma once
#include <cassert>
#include <iterator>
#include <type_traits>

namespace gta3sc::util
{
template<typename T>
class IntrusiveBidirectionalListNode;

namespace algorithm::circular_list
{
template<typename U>
constexpr void init(IntrusiveBidirectionalListNode<U>& node) noexcept;

template<typename U>
constexpr auto empty(const IntrusiveBidirectionalListNode<U>& node) noexcept
        -> bool;

template<typename U>
constexpr void insert(IntrusiveBidirectionalListNode<U>& pos,
                      IntrusiveBidirectionalListNode<U>& node) noexcept;

template<typename U>
constexpr void erase(IntrusiveBidirectionalListNode<U>& node) noexcept;

template<typename U>
constexpr void erase(IntrusiveBidirectionalListNode<U>& first,
                     IntrusiveBidirectionalListNode<U>& last) noexcept;

template<typename U>
constexpr void swap(IntrusiveBidirectionalListNode<U>& lhs,
                    IntrusiveBidirectionalListNode<U>& rhs) noexcept;

template<typename U>
constexpr void splice(IntrusiveBidirectionalListNode<U>& pos,
                      IntrusiveBidirectionalListNode<U>& other_first,
                      IntrusiveBidirectionalListNode<U>& other_last) noexcept;
} // namespace algorithm::circular_list
} // namespace gta3sc::util

namespace gta3sc::util
{
/// Base class for nodes of intrusive doubly linked list of values of type `T`.
///
/// Please note that an intrusive list of this type requires the existence
/// of a sentinel node backing the past-the-end element. This is necessary
/// due to the BidirectionalIterator requirement of decrementing an `end()`
/// iterator.
///
/// Does not inherit from `IntrusiveForwardListNode` to make a clear
/// distinction on the sentinel node semantics.
///
/// This type is trivially destructible, therefore, upon destruction the node
/// is not unlinked from the list it pertains. This must be managed manually
/// if necessary.
///
/// Be careful with the thread safety of derived objects when they have
/// iterators pointing to them (including having the object as part of a
/// container). You may have only constant references, but the iterator
/// contains a non-constant reference!
template<typename T>
class IntrusiveBidirectionalListNode
{
private:
    template<bool>
    class IteratorImpl;

public:
    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    template<typename U>
    friend constexpr void algorithm::circular_list::init(
            IntrusiveBidirectionalListNode<U>& node) noexcept;

    template<typename U>
    friend constexpr auto algorithm::circular_list::empty(
            const IntrusiveBidirectionalListNode<U>& node) noexcept -> bool;

    template<typename U>
    friend constexpr void algorithm::circular_list::insert(
            IntrusiveBidirectionalListNode<U>& pos,
            IntrusiveBidirectionalListNode<U>& node) noexcept;

    template<typename U>
    friend constexpr void algorithm::circular_list::erase(
            IntrusiveBidirectionalListNode<U>& node) noexcept;

    template<typename U>
    friend constexpr void algorithm::circular_list::erase(
            IntrusiveBidirectionalListNode<U>& first,
            IntrusiveBidirectionalListNode<U>& last) noexcept;

    template<typename U>
    friend constexpr void algorithm::circular_list::swap(
            IntrusiveBidirectionalListNode<U>& lhs,
            IntrusiveBidirectionalListNode<U>& rhs) noexcept;

    template<typename U>
    friend constexpr void algorithm::circular_list::splice(
            IntrusiveBidirectionalListNode<U>& pos,
            IntrusiveBidirectionalListNode<U>& other_first,
            IntrusiveBidirectionalListNode<U>& other_last) noexcept;

private:
    T* next{};
    T* prev{};
};

/// An iterator for intrusive doubly linked lists.
///
/// The type `T` must be derived from `IntrusiveBidirectionalListNode`.
///
/// The past-the-end element for this iterator must be a sentinel node.
/// See `IntrusiveBidirectionalListNode` for the reasoning.
template<typename T>
template<bool IsConstIter>
class IntrusiveBidirectionalListNode<T>::IteratorImpl
{
private:
    using iterator = IteratorImpl;
    using node_type = IntrusiveBidirectionalListNode;

public:
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using reference = T&;
    using pointer = T*;
    using value_type = T;

    static_assert(std::is_base_of_v<node_type, T>);

public:
    /// Constructs an invalid iterator.
    constexpr IteratorImpl() noexcept = default;

    /// Constructs an iterator pointing to the specified node.
    constexpr explicit IteratorImpl(node_type& node) noexcept : curr(&node) {}

    /// Constructs an iterator pointing to the specified node or
    /// past-the-end (i.e. to the sentinel) if the node is null.
    constexpr explicit IteratorImpl(node_type* node,
                                    node_type& sentinel) noexcept :
        curr(node ? node : &sentinel)
    {}

    constexpr IteratorImpl(const IteratorImpl&) noexcept = default;

    constexpr auto operator=(const IteratorImpl&) noexcept
            -> IteratorImpl& = default;

    /// Enable conversion from iterator to const_iterator.
    template<bool IsOtherConstIter,
             typename = std::enable_if_t<IsConstIter && !IsOtherConstIter>>
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    constexpr IteratorImpl(const IteratorImpl<IsOtherConstIter>& other) noexcept
        :
        curr(other.operator->())
    {}

    constexpr IteratorImpl(IteratorImpl&&) noexcept = default;

    constexpr auto operator=(IteratorImpl&&) noexcept
            -> IteratorImpl& = default;

    constexpr ~IteratorImpl() noexcept = default;

    constexpr auto operator*() const
            -> std::conditional_t<IsConstIter, const value_type, value_type>&
    {
        return static_cast<value_type&>(*curr);
    }

    constexpr auto operator->() const
            -> std::conditional_t<IsConstIter, const value_type, value_type>*
    {
        return static_cast<value_type*>(curr);
    }

    constexpr auto operator++() -> iterator&
    {
        // This should never assert since the past-the-end iterator contains
        // a physical sentinel node in its place.
        assert(curr && curr->next);
        curr = curr->next;
        return *this;
    }

    constexpr auto operator++(int) -> iterator
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    constexpr auto operator--() -> iterator&
    {
        assert(curr && curr->prev);
        curr = curr->prev;
        return *this;
    }

    constexpr auto operator--(int) -> iterator
    {
        auto temp = *this;
        --(*this);
        return temp;
    }

    constexpr friend auto operator==(const iterator& lhs,
                                     const iterator& rhs) noexcept -> bool
    {
        return lhs.curr == rhs.curr;
    }

    constexpr friend auto operator!=(const iterator& lhs,
                                     const iterator& rhs) noexcept -> bool
    {
        return !(lhs == rhs);
    }

    /// Converts a const_iterator into an iterator.
    ///
    /// This is not a violation of the type system since the construction
    /// of iterator requires a non-constant `T`. Nevertheless, the use of
    /// this method should be limited to containers.
    constexpr auto unconst() const noexcept -> IteratorImpl<false>
    {
        return IteratorImpl<false>(*curr);
    }

private:
    friend class IteratorImpl<!IsConstIter>;
    node_type* curr{};
};

namespace algorithm::circular_list
{
/// Initialises the given `node` as an empty circular list.
template<typename T>
constexpr void init(IntrusiveBidirectionalListNode<T>& node) noexcept
{
    node.next = node.prev = static_cast<T*>(&node);
}

/// Checks whether the circular list on which `node` is in is empty.
template<typename T>
constexpr auto empty(const IntrusiveBidirectionalListNode<T>& node) noexcept
        -> bool
{
    if(&node == node.next)
    {
        assert(&node == node.prev);
        return true;
    }
    return false;
}

/// Inserts a non-circular list node `node` before the node `pos` of a
/// circular list.
template<typename T>
constexpr void insert(IntrusiveBidirectionalListNode<T>& pos,
                      IntrusiveBidirectionalListNode<T>& node) noexcept
{
    assert(pos.next && pos.prev);
    assert(!node.next && !node.prev);

    node.next = static_cast<T*>(&pos);
    node.prev = pos.prev;

    pos.prev->next = static_cast<T*>(&node);
    pos.prev = static_cast<T*>(&node);

    assert(pos.prev == &node && node.next == &pos);
}

/// Removes the range `[first, last)` from its circular list.
///
/// After this call, the range of elements aren't circular anymore.
template<typename T>
constexpr void erase(IntrusiveBidirectionalListNode<T>& first,
                     IntrusiveBidirectionalListNode<T>& last) noexcept
{
    assert(first.prev && first.next);
    assert(last.prev && last.next);

    if(&first == &last)
        return;

    T* first_prev = first.prev;
    T* last_prev = last.prev;

    first_prev->next = static_cast<T*>(&last);
    last_prev->next = nullptr;

    last.prev = first_prev;
    first.prev = nullptr;

    assert(!first.prev && !last_prev->next);
}

/// Removes `node` from its circular list.
///
/// After this call, `node` isn't circular anymore.
template<typename T>
constexpr void erase(IntrusiveBidirectionalListNode<T>& node) noexcept
{
    assert(node.prev && node.next);

    if(empty(node))
    {
        // We need to handle this corner-case because `&node == node.next`
        // and as such erase of an empty range does nothing.
        node.next = node.prev = nullptr;
    }
    else
    {
        erase(node, *node.next);
    }

    assert(!node.prev && !node.next);
}

/// Given two circular list nodes, swaps them in their respective lists.
///
/// That is, after this call, `lhs` is on the same position as `rhs` in its
/// list, and vice-versa.
template<typename T>
constexpr void swap(IntrusiveBidirectionalListNode<T>& lhs,
                    IntrusiveBidirectionalListNode<T>& rhs) noexcept
{
    assert(lhs.next && lhs.prev);
    assert(rhs.next && rhs.prev);

    if(&lhs == &rhs)
        return;

    if(empty(lhs))
    {
        if(empty(rhs))
            return;
        return swap(rhs, lhs);
    }

    const bool was_rhs_empty = empty(rhs);
    T* rhs_first_node = rhs.next;
    T* rhs_last_node = rhs.prev;

    auto unsafe_link = [](IntrusiveBidirectionalListNode<T>& node,
                          IntrusiveBidirectionalListNode<T>& first,
                          IntrusiveBidirectionalListNode<T>& last) {
        node.next = static_cast<T*>(&first);
        node.prev = static_cast<T*>(&last);
        first.prev = static_cast<T*>(&node);
        last.next = static_cast<T*>(&node);
    };

    unsafe_link(rhs, *lhs.next, *lhs.prev);

    if(!was_rhs_empty)
        unsafe_link(lhs, *rhs_first_node, *rhs_last_node);
    else
        init(lhs);

    assert(lhs.next && lhs.prev);
    assert(rhs.next && rhs.prev);
}

/// Transfers the nodes `[other_first, other_last)` from a certain circular
/// list into before `pos` from another circular list.
template<typename T>
constexpr void splice(IntrusiveBidirectionalListNode<T>& pos,
                      IntrusiveBidirectionalListNode<T>& other_first,
                      IntrusiveBidirectionalListNode<T>& other_last) noexcept

{
    assert(pos.next && pos.prev);
    assert(other_first.next && other_first.prev);
    assert(other_last.next && other_last.prev);

    if(&other_first == &other_last)
        return;

    T* pos_prev = pos.prev;
    T* other_first_prev = other_first.prev;
    T* other_last_prev = other_last.prev;

    other_last_prev->next = static_cast<T*>(&pos);
    pos.prev = other_last_prev;

    other_first_prev->next = static_cast<T*>(&other_last);
    other_last.prev = other_first_prev;

    pos_prev->next = static_cast<T*>(&other_first);
    other_first.prev = pos_prev;

    assert(pos.next && pos.prev);
    assert(other_first.next && other_first.prev);
    assert(other_last.next && other_last.prev);
}
} // namespace algorithm::circular_list
} // namespace gta3sc::util

#pragma once
#include <cassert>
#include <iterator>
#include <type_traits>

namespace gta3sc::util
{
template<typename T>
class IntrusiveForwardListNode;

namespace algorithm::linear_list
{
template<typename U>
constexpr void insert_after(IntrusiveForwardListNode<U>& pos,
                            IntrusiveForwardListNode<U>& node) noexcept;

template<typename U>
constexpr auto push_back(IntrusiveForwardListNode<U>& node, U* first,
                         U* last) noexcept -> std::pair<U*, U*>;
} // namespace algorithm::linear_list
} // namespace gta3sc::util

namespace gta3sc::util
{
/// Base class for nodes of intrusive singly linked lists of values of type `T`.
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
class IntrusiveForwardListNode
{
private:
    template<bool>
    class IteratorImpl;

public:
    using Iterator = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

    template<typename U>
    friend constexpr void algorithm::linear_list::insert_after(
            IntrusiveForwardListNode<U>& pos,
            IntrusiveForwardListNode<U>& node) noexcept;

    template<typename U>
    friend constexpr auto
    algorithm::linear_list::push_back(IntrusiveForwardListNode<U>& node,
                                      U* first, U* last) noexcept
            -> std::pair<U*, U*>;

private:
    T* next{};
};

/// A forward iterator for intrusive singly linked lists.
///
/// The type `T` must be derived from `IntrusiveForwardListNode`.
///
/// The past-the-end element for this iterator is represent as a null
/// pointer such that a next pointer pointing to null finishes iteration.
template<typename T>
template<bool IsConstIter>
class IntrusiveForwardListNode<T>::IteratorImpl
{
private:
    using iterator = IteratorImpl;
    using node_type = IntrusiveForwardListNode;

public:
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using iterator_category = std::forward_iterator_tag;
    using reference = T&;
    using pointer = T*;
    using value_type = T;

    static_assert(std::is_base_of_v<node_type, T>);

public:
    /// Constructs an iterator pointing to the past-the-end element of a
    /// singly linked list.
    constexpr IteratorImpl() noexcept = default;

    /// Constructs an iterator pointing to the specified node.
    explicit constexpr IteratorImpl(node_type& node) noexcept : curr(&node) {}

    /// Constructs an iterator pointing to the specified node or
    /// past-the-end if the node is null.
    explicit constexpr IteratorImpl(node_type* node, std::nullptr_t) noexcept :
        curr(node ? node : nullptr)
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
        curr(other.curr)
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
        assert(curr);
        curr = curr->next;
        return *this;
    }

    constexpr auto operator++(int) -> iterator
    {
        auto temp = *this;
        ++(*this);
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
    [[nodiscard]] constexpr auto unconst() const noexcept -> IteratorImpl<false>
    {
        return IteratorImpl<false>(curr, nullptr);
    }

private:
    friend class IteratorImpl<!IsConstIter>;
    node_type* curr{};
};

namespace algorithm::linear_list
{
/// Inserts a `node` not linked to any singly linked list into after `pos` in
/// its list.
template<typename T>
constexpr void insert_after(IntrusiveForwardListNode<T>& pos,
                            IntrusiveForwardListNode<T>& node) noexcept
{
    assert(!node.next);

    T* pos_next = pos.next;

    pos.next = static_cast<T*>(&node);
    node.next = pos_next;
}

/// Inserts `node` at the end of the list represented by `[first, last]`.
///
/// The list represented by `[first, last]` is a singly linked list starting
/// from `first` and ending (inclusive) in `last`. If both values are `nullptr`
/// then the list is empty.
///
/// Returns the new list representation.
template<typename T>
constexpr auto push_back(IntrusiveForwardListNode<T>& node, T* first,
                         T* last) noexcept -> std::pair<T*, T*>
{
    assert(!node.next);
    assert(!last || !last->next);

    if(last)
    {
        last->next = static_cast<T*>(&node);
    }
    else
    {
        assert(!first);
        first = static_cast<T*>(&node);
    }

    last = static_cast<T*>(&node);

    return {first, last};
}
} // namespace algorithm::linear_list
} // namespace gta3sc::util

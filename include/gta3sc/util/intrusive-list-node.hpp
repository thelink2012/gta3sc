#pragma once
#include <cassert>
#include <iterator>
#include <type_traits>

namespace gta3sc::util
{
template<typename T>
class IntrusiveList;

namespace detail
{
    template<typename T, bool ConstIterTag>
    class IntrusiveListForwardIterator;

    template<typename T, bool ConstIterTag>
    class IntrusiveListBidirectionalIterator;

    template<typename T>
    class IntrusiveBidirectionalListNodeAlgorithms;
}

template<typename T>
using IntrusiveListForwardIterator
        = detail::IntrusiveListForwardIterator<T, false>;

template<typename T>
using ConstIntrusiveListForwardIterator
        = detail::IntrusiveListForwardIterator<T, true>;

template<typename T>
using IntrusiveListBidirectionalIterator
        = detail::IntrusiveListBidirectionalIterator<T, false>;

template<typename T>
using ConstIntrusiveListBidirectionalIterator
        = detail::IntrusiveListBidirectionalIterator<T, true>;

/// Base class for nodes of intrusive singly linked lists of values of type `T`.
///
/// This type is trivially destructible, therefore, upon destruction the node
/// is not unlinked from the list it pertains. This must be managed manually
/// if necessary.
///
/// Be careful with the thread safety of derived objects (specially when in a
/// container) since this struct have interior mutability.
template<typename T>
class IntrusiveForwardListNode
{
protected:
    template<typename, bool>
    friend class detail::IntrusiveListForwardIterator;

    template<typename>
    friend class IntrusiveBidirectionalListNodeAlgorithms;

    mutable T* next{};
};

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
/// Be careful with the thread safety of derived objects (specially when in a
/// container) since this struct have interior mutability.
template<typename T>
class IntrusiveBidirectionalListNode
{
public:
    template<typename, bool>
    friend class detail::IntrusiveListBidirectionalIterator;

    template<typename>
    friend class IntrusiveList;

    mutable T* next{};
    mutable T* prev{};
};

namespace detail
{
    /// A forward iterator for intrusive singly linked lists.
    ///
    /// The type `T` must be derived from `IntrusiveForwardListNode`.
    ///
    /// The past-the-end element for this iterator is represent as a null
    /// pointer such that a next pointer pointing to null finishes iteration.
    template<typename T, bool IsConstIter>
    class IntrusiveListForwardIterator
    {
    private:
        using iterator = IntrusiveListForwardIterator;
        using node_type = IntrusiveForwardListNode<T>;

        template<typename U>
        using maybe_const = std::conditional_t<IsConstIter, const U, U>;

    public:
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using iterator_category = std::forward_iterator_tag;
        using reference = T&;
        using pointer = T*;
        using value_type = T;

        // Do not allow IntrusiveBidirectionalListNode because the past-the-end
        // iterator semantics are different.
        static_assert(std::is_base_of_v<node_type, T>);

    public:
        /// Constructs an iterator pointing to the past-the-end element of a
        /// singly linked list.
        IntrusiveListForwardIterator() {}

        /// Constructs an iterator pointing to the specified node.
        explicit IntrusiveListForwardIterator(maybe_const<node_type>& node) :
            curr(&node)
        {}

        /// Constructs an iterator pointing to the specified node or
        /// past-the-end if the node is null.
        explicit IntrusiveListForwardIterator(maybe_const<node_type>* node,
                                              std::nullptr_t) :
            curr(node ? node : nullptr)
        {}

        IntrusiveListForwardIterator(const IntrusiveListForwardIterator&)
                = default;

        IntrusiveListForwardIterator&
        operator=(const IntrusiveListForwardIterator&)
                = default;

        /// Enable conversion from iterator to const_iterator.
        template<bool IsOtherConstIter,
                 typename = std::enable_if_t<IsConstIter && !IsOtherConstIter>>
        IntrusiveListForwardIterator(
                const IntrusiveListForwardIterator<T, IsOtherConstIter>&
                        other) :
            curr(other.operator->())
        {}

        auto operator*() const -> maybe_const<value_type>&
        {
            return static_cast<maybe_const<value_type>&>(*curr);
        }

        auto operator->() const -> maybe_const<value_type>*
        {
            return static_cast<maybe_const<value_type>*>(curr);
        }

        iterator& operator++()
        {
            // No need to assert for curr->next since null means past-the-end.
            assert(curr);
            curr = curr->next;
            return *this;
        }

        iterator operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator& rhs) const { return curr == rhs.curr; }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

    private:
        maybe_const<node_type>* curr{};
    };

    /// An iterator for intrusive doubly linked lists.
    ///
    /// The type `T` must be derived from `IntrusiveForwardListNode`.
    ///
    /// The past-the-end element for this iterator must be a sentinel node.
    /// See `IntrusiveBidirectionalListNode` for the reasoning.
    template<typename T, bool IsConstIter>
    class IntrusiveListBidirectionalIterator
    {
    private:
        using iterator = IntrusiveListBidirectionalIterator;
        using node_type = IntrusiveBidirectionalListNode<T>;

        template<typename U>
        using maybe_const = std::conditional_t<IsConstIter, const U, U>;

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
        IntrusiveListBidirectionalIterator() {}

        /// Constructs an iterator pointing to the specified node.
        explicit IntrusiveListBidirectionalIterator(
                maybe_const<node_type>& node) :
            curr(&node)
        {}

        /// Constructs an iterator pointing to the specified node or
        /// past-the-end (i.e. to the sentinel) if the node is null.
        explicit IntrusiveListBidirectionalIterator(
                maybe_const<node_type>* node,
                maybe_const<node_type>& sentinel) :
            curr(node ? node : &sentinel)
        {}

        IntrusiveListBidirectionalIterator(
                const IntrusiveListBidirectionalIterator&)
                = default;

        IntrusiveListBidirectionalIterator&
        operator=(const IntrusiveListBidirectionalIterator&)
                = default;

        /// Enable conversion from iterator to const_iterator.
        template<bool IsOtherConstIter,
                 typename = std::enable_if_t<IsConstIter && !IsOtherConstIter>>
        IntrusiveListBidirectionalIterator(
                const IntrusiveListBidirectionalIterator<T, IsOtherConstIter>&
                        other) :
            curr(other.operator->())
        {}

        auto operator*() const -> maybe_const<value_type>&
        {
            return static_cast<maybe_const<value_type>&>(*curr);
        }

        auto operator->() const -> maybe_const<value_type>*
        {
            return static_cast<maybe_const<value_type>*>(curr);
        }

        iterator& operator++()
        {
            // This should never assert since the past-the-end iterator contains
            // a physical sentinel node in its place.
            assert(curr && curr->next);
            curr = curr->next;
            return *this;
        }

        iterator operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        iterator& operator--()
        {
            assert(curr && curr->prev);
            curr = curr->prev;
            return *this;
        }

        iterator operator--(int)
        {
            auto temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const iterator& rhs) const { return curr == rhs.curr; }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

    protected:
        maybe_const<node_type>* curr{};
    };

    template<typename T>
    class IntrusiveBidirectionalListNodeAlgorithms
    {
    private:
        using node_type = IntrusiveBidirectionalListNode<T>;
        using iterator = IntrusiveListBidirectionalIterator<T, false>;
        using pointer = typename iterator::pointer;

    public:
        /// Unlinks the given node from its list.
        static inline void unlink_node(node_type& node)
        {
            pointer node_prev = node.prev;
            pointer node_next = node.next;

            if(node.prev)
            {
                node.prev->next = node_next;
                node.prev = nullptr;
            }

            if(node.next)
            {
                node.next->prev = node_prev;
                node.next = nullptr;
            }
        }
    };

}

//
// Algorithms
//

/// Unlinks the given node from its list.
template<typename T>
inline void unlink_node(IntrusiveBidirectionalListNode<T>& node)
{
    using Algorithms = detail::IntrusiveBidirectionalListNodeAlgorithms<T>;
    return Algorithms::unlink_node(node);
}
}

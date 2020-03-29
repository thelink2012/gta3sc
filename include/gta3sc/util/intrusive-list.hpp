#pragma once
#include <iterator>
#include <type_traits>

namespace gta3sc::util
{
template<typename T>
class IntrusiveListForwardIterator;

template<typename T>
class IntrusiveListBidirectionalIterator;

/// Base class for nodes of intrusive singly linked lists of values of type `T`.
template<typename T>
class IntrusiveForwardListNode
{
protected:
    template<typename U>
    friend class IntrusiveListForwardIterator;

    T* next{};
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
template<typename T>
class IntrusiveBidirectionalListNode
{
protected:
    template<typename U>
    friend class IntrusiveListBidirectionalIterator;

    T* next{};
    T* prev{};
};

/// A forward iterator for intrusive singly linked lists.
///
/// The type `T` must be derived from `IntrusiveForwardListNode`.
///
/// The past-the-end element for this iterator is represent as a null pointer
/// such that a next pointer pointing to null finishes iteration.
template<typename T>
class IntrusiveListForwardIterator
{
private:
    using iterator = IntrusiveListForwardIterator;
    using node_type = IntrusiveForwardListNode<std::remove_cv_t<T>>;

public:
    using difference_type = std::ptrdiff_t;
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
    explicit IntrusiveListForwardIterator(reference node) : curr(&node) {}

    /// Constructs an iterator pointing to the specified node or past-the-end
    /// if the node is null.
    explicit IntrusiveListForwardIterator(pointer node, std::nullptr_t) :
        curr(node ? node : nullptr)
    {}

    reference operator*() { return *curr; }
    pointer operator->() { return curr; }

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
    pointer curr{};
};

/// An iterator for intrusive doubly linked lists.
///
/// The type `T` must be derived from `IntrusiveForwardListNode`.
///
/// The past-the-end element for this iterator must be a sentinel node.
/// See `IntrusiveBidirectionalListNode` for the reasoning.
template<typename T>
class IntrusiveListBidirectionalIterator
{
private:
    using iterator = IntrusiveListBidirectionalIterator;
    using node_type = IntrusiveBidirectionalListNode<std::remove_cv_t<T>>;

public:
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using reference = T&;
    using pointer = T*;
    using value_type = T;

    static_assert(std::is_base_of_v<node_type, T>);

public:
    /// Constructs an invalid iterator.
    IntrusiveListBidirectionalIterator() {}

    /// Constructs an iterator pointing to the specified node.
    explicit IntrusiveListBidirectionalIterator(reference node) : curr(&node) {}

    /// Constructs an iterator pointing to the specified node or past-the-end
    /// (i.e. to the sentinel) if the node is null.
    explicit IntrusiveListBidirectionalIterator(pointer node,
                                                reference sentinel) :
        curr(node ? node : &sentinel)
    {}

    reference operator*() { return *curr; }
    pointer operator->() { return curr; }

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

private:
    pointer curr{};
};
}

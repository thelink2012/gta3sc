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
protected:
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

        auto operator-> () const -> maybe_const<value_type>*
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

        auto operator-> () const -> maybe_const<value_type>*
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
}

/// A intrusive doubly-linked list container.
///
/// This is container of `IntrusiveBidirectionalListNode` elements. It does not
/// perform any memory allocation whatsoever and just manipulates the `next`
/// and `prev` member variables of its elements.
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
/// does not manage the concrete list formed by its elements. Users may alter
/// that list without direct access to its container as long as the links are
/// kept in a consistent state.
///
/// The destruction of this container simply erases the sentinel node from the
/// concrete list. That is, the linked list continues to exist.
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
    using iterator = IntrusiveListBidirectionalIterator<T>;
    using const_iterator = ConstIntrusiveListBidirectionalIterator<T>;

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
    IntrusiveList() { unsafe_clear(); }

    /// Behaves the same as construction followed by `assign(first, last)`.
    template<typename InputIt>
    IntrusiveList(InputIt first, InputIt last) : IntrusiveList()
    {
        assign(first, last);
    }

    /// Behaves the same as construction followed by
    /// `assign_pointer(ilist.begin(), ilist.end())`
    IntrusiveList(std::initializer_list<pointer> ilist) : IntrusiveList()
    {
        assign_pointer(ilist.begin(), ilist.end());
    }

    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    IntrusiveList(IntrusiveList&& other) : IntrusiveList()
    {
        other.swap(*this);
    }

    IntrusiveList& operator=(IntrusiveList&& other) noexcept
    {
        if(this == &other)
            return *this;

        clear();
        other.swap(*this);
        return *this;
    }

    ~IntrusiveList() noexcept { clear(); }

    //
    // Iterators
    //

    auto begin() noexcept -> iterator { return iterator(*sentinel.next); }

    auto begin() const noexcept -> const_iterator
    {
        return const_iterator(*sentinel.next);
    }

    auto cbegin() const noexcept -> const_iterator { return begin(); }

    auto end() noexcept -> iterator { return iterator(sentinel); }

    auto end() const noexcept -> const_iterator
    {
        return const_iterator(sentinel);
    }

    auto cend() const noexcept -> const_iterator { return end(); }

    auto rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    auto rbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    auto crbegin() const noexcept -> const_reverse_iterator { return rbegin(); }

    auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    auto rend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    auto crend() const noexcept -> const_reverse_iterator { return rend(); }

    //
    // Element access
    //

    auto front() noexcept -> reference { return *sentinel.next; }

    auto front() const noexcept -> const_reference { return *sentinel.next; }

    auto back() noexcept -> reference { return *sentinel.prev; }

    auto back() const noexcept -> const_reference { return *sentinel.prev; }

    //
    // Capacity
    //

    bool empty() const noexcept { return sentinel.next == &sentinel; }

    //
    // Modifiers
    //

    /// Clears the container.
    ///
    /// This simply removes the sentinel node from the concrete list,
    /// effectively eliminating the relationship between the container
    /// and the concrete list.
    void clear() noexcept
    {
        if(empty())
            return;

        sentinel.next->prev = nullptr;
        sentinel.prev->next = nullptr;
        unsafe_clear();
    }

    /// Inserts the given `node` before `pos`.
    ///
    /// The given node must not be part of any concrete list.
    auto insert(const_iterator pos, reference node) noexcept -> iterator
    {
        assert(!node.next && !node.prev);

        node.next = const_cast<pointer>(&(*pos));
        node.prev = pos->prev;

        pos->prev->next = &node;
        pos->prev = &node;

        return iterator(node);
    }

    /// Inserts the elements in the range `[first, last)` before `pos`.
    ///
    /// This is equivalent to calling `insert` on each of the elements
    /// in the sequence. As such, each of them, must not be part of any
    /// concrete list.
    template<typename InputIt>
    void insert(const_iterator pos, InputIt first, InputIt last) noexcept
    {
        for(auto it = first; it != last; ++it)
            insert(pos, *it);
    }

    /// Behaves as if calling `clear()` followed by `insert(end(), first,
    /// last)`.
    template<typename InputIt>
    void assign(InputIt first, InputIt last) noexcept
    {
        clear();
        insert(first, last);
    }

    /// Inserts the given node at the front of the container's list.
    void push_front(reference node) noexcept { insert(begin(), node); }

    /// Inserts the given node at the back of the container's list.
    void push_back(reference node) noexcept { insert(end(), node); }

    /// Unlinks the given element from the container and returns an iterator
    /// to the element following the removed one.
    auto erase(const_iterator pos) noexcept -> iterator
    {
        assert(pos != const_iterator(sentinel));
        return erase(pos, std::next(pos));
    }

    /// Unlinks the range `[first, last)` from the container and returns an
    /// iterator following the last element removed.
    auto erase(const_iterator first, const_iterator last) noexcept -> iterator
    {
        if(first == last)
            return iterator(const_cast<reference>(*last));

        first->prev->next = const_cast<pointer>(&(*last));
        last->prev->next = nullptr;

        last->prev = const_cast<pointer>(first->prev);
        first->prev = nullptr;

        return iterator(const_cast<reference>(*last));
    }

    /// Erases the first element from the container's list.
    void pop_front(reference node) noexcept { erase(begin()); }

    /// Erases the last element from the container's list.
    void pop_back(reference node) noexcept { erase(std::prev(end())); }

    /// Swaps the container's list of this and `other`.
    void swap(IntrusiveList& other) noexcept
    {
        if(empty())
        {
            if(other.empty())
                return;
            return other.swap(*this);
        }

        const bool was_other_empty = other.empty();
        auto other_first_node = other.sentinel.next;
        auto other_last_node = other.sentinel.prev;

        other.unsafe_link(*sentinel.next, *sentinel.prev);

        if(!was_other_empty)
            unsafe_link(*other_first_node, *other_last_node);
        else
            unsafe_clear();
    }

    //
    // Operations
    //

    /// Transfers the elements `[first, last)` from `other` into this.
    ///
    /// The elements are transfered to before `pos`.
    void splice(const_iterator pos, IntrusiveList& other,
                const_iterator other_first, const_iterator other_last) noexcept

    {
        if(other_first == other_last)
            return;

        auto pos_prev = std::prev(pos);
        auto other_first_prev = std::prev(other_first);
        auto other_last_prev = std::prev(other_last);

        other_last_prev->next = const_cast<pointer>(&(*pos));
        pos->prev = const_cast<pointer>(&(*other_last_prev));

        other_first_prev->next = const_cast<pointer>(&(*other_last));
        other_last->prev = const_cast<pointer>(&(*other_first_prev));

        pos_prev->next = const_cast<pointer>(&(*other_first));
        other_first->prev = const_cast<pointer>(&(*pos_prev));
    }

    /// Behaves as if calling `splice(pos, other, other_first, other_last)`
    /// followed by `other.clear()`.
    void splice(const_iterator pos, IntrusiveList&& other,
                const_iterator other_first, const_iterator other_last) noexcept

    {
        splice(pos, other, other_first, other_last);
        other.clear();
    }

    /// Behaves as if calling `splice(pos, other, other_it,
    /// std::next(other_it))`.
    void splice(const_iterator pos, IntrusiveList& other,
                const_iterator other_it) noexcept
    {
        splice(pos, other, other_it, std::next(other_it));
    }

    /// Behaves as if calling `splice(pos, other, other_it)` followed by
    /// `other.clear()`.
    void splice(const_iterator pos, IntrusiveList&& other,
                const_iterator other_it) noexcept
    {
        splice(pos, other, other_it);
        other.clear();
    }

    /// Behaves as if calling `splice(pos, other, other.begin(), other.end())`.
    void splice(const_iterator pos, IntrusiveList& other) noexcept
    {
        splice(pos, other, other.begin(), other.end());
        assert(other.empty());
    }

    /// Behaves as if calling `splice(pos, other)`.
    void splice(const_iterator pos, IntrusiveList&& other) noexcept
    {
        splice(pos, other);
        assert(other.empty());
    }

    /// Erase elements that compare equal to `value`.
    auto remove(const T& value) noexcept -> size_type
    {
        return remove_if([&](const T& iter_val) { return iter_val == value; });
    }

    /// Erases elements on which `pred(elem)` is true.
    template<typename UnaryPredicate>
    auto remove_if(UnaryPredicate pred) noexcept -> size_type
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
    void insert_pointer(const_iterator pos, InputIt first,
                        InputIt last) noexcept
    {
        for(auto it = first; it != last; ++it)
            insert(pos, **it);
    }

    /// Behaves as if calling `insert(pos, ilist.begin(), ilist.end())`.
    void insert_pointer(const_iterator pos,
                        std::initializer_list<pointer> ilist) noexcept
    {
        return insert_pointer(pos, ilist.begin(), ilist.end());
    }

    /// Same as `assign` except `InputIt::value_type` is `pointer`.
    template<typename InputIt>
    void assign_pointer(InputIt first, InputIt last) noexcept
    {
        clear();
        insert_pointer(end(), first, last);
    }

    /// Behaves as if calling `assign_pointer(ilist.begin(), ilist.end())`.
    void assign_pointer(std::initializer_list<pointer> ilist) noexcept
    {
        return assign_pointer(ilist.begin(), ilist.end());
    }

private:
    void unsafe_clear()
    {
        sentinel.next = sentinel.prev = static_cast<pointer>(&sentinel);
    }

    void unsafe_link(reference first, reference last)
    {
        sentinel.next = &first;
        sentinel.prev = &last;
        first.prev = static_cast<pointer>(&sentinel);
        last.next = static_cast<pointer>(&sentinel);
    }

private:
    node_type sentinel;
};
}

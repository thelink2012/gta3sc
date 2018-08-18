#pragma once
#include <gta3sc/arena-allocator.hpp>

// XXX: This data-structure is still under heavy work! Semantics are
// not very well defined. Implementation neither. It should probably
// be rewritten and properly documented once we have enough experience
// with its usage.
// 
// Semantics first, implementation later.

#include <list>

namespace gta3sc
{
template<typename T>
struct LinkedIR
{
private:
    using list_type = std::list<arena_ptr<T>, ArenaAllocator<arena_ptr<T>>>;
    list_type list;

public:
    struct iterator;
    using size_type = typename list_type::size_type;
    using arena_pointer = arena_ptr<T>;
    using reference = T&;
    using pointer = T*;
    using value_type = T;

    using difference_type = typename list_type::difference_type;

public:
    explicit LinkedIR(ArenaMemoryResource& arena) :
        list(&arena)
    {}

    LinkedIR(const LinkedIR&) = delete;
    LinkedIR& operator=(const LinkedIR&) = delete;

    LinkedIR(LinkedIR&&) = default;
    LinkedIR& operator=(LinkedIR&&) = default;

    iterator begin() noexcept { return iterator(list.begin()); }
    iterator end() noexcept { return iterator(list.end()); }

    bool empty() const noexcept { return list.empty(); }

    // Hmm should this be O(1) really? If this becomes an intrusive list, we 
    // cannot guarante O(1) without nasty tricks.
    // Please rename the method if this becomes O(n).
    size_type size() const noexcept { return list.size(); }

    void push_front(arena_pointer ir) { return list.push_front(ir); }

    void push_back(arena_pointer ir) { return list.push_back(ir); }

    reference front() { return *list.front(); }
    reference back() { return *list.back(); }

    void splice(iterator pos, LinkedIR&& other)
    {
        list.splice(pos.it, std::move(other.list));
    }

    void splice_front(LinkedIR&& other)
    {
        splice(begin(), std::move(other));
    }

    void splice_back(LinkedIR&& other)
    {
        splice(end(), std::move(other));
    }

    iterator erase(iterator pos) { return iterator(list.erase(pos.it)); }

    // if the list becomes intrusive this method should be on the node
    arena_pointer detach(iterator pos)
    {
        auto ptr = std::addressof(*pos);
        erase(pos);
        return ptr;
    }


public:
    struct iterator
    {
    public:
        using difference_type = typename list_type::iterator::difference_type;
        using iterator_category = typename list_type::iterator::iterator_category;
        using reference = LinkedIR::reference;
        using pointer = LinkedIR::pointer;
        using value_type = LinkedIR::value_type;

        reference operator*() { return **it; }
        arena_pointer operator->() { return *it; }

        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { auto temp = *this; ++(*this); return temp; }

        iterator& operator--() { --it; return *this; }
        iterator operator--(int) { auto temp = *this; --(*this); return temp; }

        bool operator==(const iterator& rhs) const { return it == rhs.it; }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

    protected:
        typename list_type::iterator it;

        explicit iterator(typename list_type::iterator it) : it(it)
        {}

        friend class LinkedIR;
    };
};
}

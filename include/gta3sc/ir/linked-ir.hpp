#pragma once
#include <gta3sc/util/intrusive-list.hpp>

namespace gta3sc
{
/// A container of IR instructions.
///
/// This is an intrusive doubly linked list where each element is an IR
/// instruction. See `util::IntrusiveList` for more details.
template<typename IR>
class LinkedIR : public util::IntrusiveList<IR>
{
private:
    using super_type = util::IntrusiveList<IR>;

public:
    using typename super_type::const_iterator;
    using typename super_type::const_pointer;
    using typename super_type::const_reference;
    using typename super_type::const_reverse_iterator;
    using typename super_type::difference_type;
    using typename super_type::iterator;
    using typename super_type::pointer;
    using typename super_type::reference;
    using typename super_type::reverse_iterator;
    using typename super_type::size_type;
    using typename super_type::value_type;

public:
    LinkedIR() = default;
    LinkedIR(std::initializer_list<pointer> ilist) : super_type(ilist) {}

    LinkedIR(const LinkedIR&) = delete;
    LinkedIR& operator=(const LinkedIR&) = delete;

    LinkedIR(LinkedIR&&) = default;
    LinkedIR& operator=(LinkedIR&&) = default;

    /// Moves the elements of `other` into the front of this.
    void splice_front(LinkedIR&& other)
    {
        this->splice(this->begin(), std::move(other));
    }

    /// Moves the element of `other` into the back of this.
    void splice_back(LinkedIR&& other)
    {
        this->splice(this->end(), std::move(other));
    }

    /// Replaces the element at the specified `pos` by `other`.
    //
    /// Returns the iterator to `other`.
    auto replace(iterator pos, reference other) -> iterator
    {
        return this->insert(this->erase(pos), other);
    }
};
}

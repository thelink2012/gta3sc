#pragma once
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/span.hpp>
#include <string_view>

namespace gta3sc::util
{
/// Allocates a copy of the string `copy_from` in an arena and applies
/// `transform_op` to each of its characters.
///
/// Returns an immutable view to the allocated string.
template<typename UnaryOperation>
inline auto new_string(std::string_view copy_from, ArenaAllocator<> allocator,
                       UnaryOperation transform_op) -> std::string_view
{
    const auto result_size = copy_from.size();

    auto* result_ptr = allocator.allocate_object<char>(result_size);
    std::uninitialized_default_construct_n(result_ptr, result_size);

    for(size_t i = 0; i < copy_from.size(); ++i)
        result_ptr[i] = transform_op(copy_from[i]);

    return {result_ptr, result_size};
}

/// Allocates a copy of the string `copy_from` in an arena.
///
/// Returns an immutable view to the allocated string.
inline auto new_string(std::string_view copy_from, ArenaAllocator<> allocator)
        -> std::string_view
{
    constexpr auto identity = [](char c) { return c; };
    return new_string(copy_from, allocator, identity);
}

/// Allocates a new position in `current_array` and constructs `value` in it.
///
/// This function can be used to implement a dynamically growing array in the
/// arena specified by `allocator`. The used storage of the array are
/// represented by `current_array` and its capacity by `current_capacity` (i.e.
/// total storage available for the array).
///
/// If the current capacity is equal the size of `current_array`, more storage
/// will be allocated in the arena (usuallly using a growing factor of two)
/// to be able to hold another element. If the capacity is zero, the initial
/// capacity allocation will be of `default_capacity` elements.
///
/// \note Every time the capacity grows, memory is "leaked" in the arena. So
/// choose a proper value for `default_capacity`.
///
/// Returns the new array and the new capacity.
template<typename T, typename U>
inline auto new_array_element(U&& value, util::span<T> current_array,
                              size_t current_capacity, size_t default_capacity,
                              ArenaAllocator<> allocator)
        -> std::pair<util::span<T>, size_t>
{
    assert(default_capacity != 0);
    assert(current_array.size() <= current_capacity);

    if(current_array.size() == current_capacity)
    {
        const size_t new_capacity = current_capacity == 0
                                            ? default_capacity
                                            : current_capacity * 2;

        T* const new_array = allocator.allocate_object<T>(new_capacity);
        std::uninitialized_move(current_array.begin(), current_array.end(),
                                new_array);

        current_array = util::span(new_array, current_array.size());
        current_capacity = new_capacity;
    }

    current_array = util::span(current_array.data(), current_array.size() + 1);
    allocator.construct(&current_array.back(), std::forward<U>(value));

    return {current_array, current_capacity};
}
} // namespace gta3sc::util

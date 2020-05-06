#pragma once
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/span.hpp>
#include <string_view>

namespace gta3sc::util
{
/// Initializes the given storage and copies a string into it.
///
/// \param storage uninitialized storage to initialize.
/// \param storage_size the size of the storage.
/// \param copy_from the string to copy into `storage`.
/// \param transform_op operation to apply to each of the `copy_from` characters
///        before storing in `storage`.
///
/// Returns an immutable view to the initialized string.
template<typename UnaryOperation>
constexpr auto construct_string_at(char* storage, size_t storage_size,
                                   std::string_view copy_from,
                                   UnaryOperation transform_op)
        -> std::string_view
{
    std::uninitialized_default_construct_n(storage, storage_size);

    const auto result_size = std::min(storage_size, copy_from.size());

    for(size_t i = 0; i < result_size; ++i)
        storage[i] = transform_op(copy_from[i]);

    return {storage, result_size};
}

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

    return construct_string_at(result_ptr, result_size, copy_from,
                               std::move(transform_op));
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

/// Allocates a object `T` and a string alongside it.
///
/// This method behaves as if calling `std::make_pair(new_string(copy_from,
/// allocator, transform_op), allocator.new_object<T>(args...))` except that
/// the allocated string is retrievable from `T*` through `get_sibling_string`.
template<typename T, typename UnaryOperation, typename... CtorArgs>
inline auto
new_object_with_string(std::string_view copy_from, UnaryOperation transform_op,
                       ArenaAllocator<> allocator, CtorArgs&&... args)
        -> std::pair<std::string_view, T*>
{
    const auto string_size = copy_from.size();

    void* storage = allocator.allocate_bytes(sizeof(T) + string_size,
                                             alignof(T));

    auto* object_storage = static_cast<T*>(storage);
    auto* string_storage = static_cast<char*>(storage) + sizeof(T);

    allocator.construct<T>(object_storage, std::forward<CtorArgs>(args)...);
    construct_string_at(string_storage, string_size, copy_from,
                        std::move(transform_op));

    return {std::string_view(string_storage, string_size), object_storage};
}

/// Returns the string allocated alongside `object` in `new_object_with_string`.
///
/// Behaviour is undefined if `object` wasn't allocated by using
/// `new_object_with_string`.
template<typename T>
constexpr auto get_sibling_string(const T* object, size_t string_size) noexcept
        -> std::string_view
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* string_storage = reinterpret_cast<const char*>(object)
                                 + sizeof(T);
    return {string_storage, string_size};
}
} // namespace gta3sc::util

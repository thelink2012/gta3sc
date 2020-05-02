#pragma once
#include <gta3sc/util/arena-allocator.hpp>
#include <string_view>

namespace gta3sc::util
{
/// Allocates a copy of the string `copy_from` in an arena and applies
/// `transform_op` to each of its characters.
///
/// Returns an immutable view to the allocated string.
template<typename UnaryOperation>
inline auto allocate_string(std::string_view copy_from,
                            ArenaAllocator<char> allocator,
                            UnaryOperation transform_op) -> std::string_view
{
    const auto result_size = copy_from.size();

    auto* result_ptr = allocator.allocate(result_size);
    std::uninitialized_default_construct_n(result_ptr, result_size);

    for(size_t i = 0; i < copy_from.size(); ++i)
        result_ptr[i] = transform_op(copy_from[i]);

    return {result_ptr, result_size};
}

/// Allocates a copy of the string `copy_from` in an arena.
///
/// Returns an immutable view to the allocated string.
inline auto allocate_string(std::string_view copy_from,
                            ArenaAllocator<char> allocator) -> std::string_view
{
    constexpr auto identity = [](char c) { return c; };
    return allocate_string(copy_from, allocator, identity);
}
} // namespace gta3sc::util

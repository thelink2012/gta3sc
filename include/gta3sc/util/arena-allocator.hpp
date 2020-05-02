#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string_view>

namespace gta3sc
{
/// An owning pointer to a trivially destructible object in an arena.
///
/// An object encapsulated in such a pointer does not need a destructor call
/// and may be disposed of by simply deallocating storage.
template<typename T>
using ArenaPtr = T*;

// template<typename T>
// using ArenaPtr = std::enable_if_t<std::is_trivially_destructible_v<T>, T*>;
// The above causes problems when the full class definition is still not known.

/// This is a memory resource that releases the allocated memory only when
/// the resource is destroyed. It is intended for very fast memory allocations
/// in situations where memory is used to build up a few objects and then is
/// released all at once.
///
/// This implementation is based on the `std::pmr::monotonic_buffer_resource`
/// interface which is not yet available in C++ library implementations.
///
/// See also:
///  + http://eel.is/c++draft/mem.res.monotonic.buffer
///  + https://en.cppreference.com/w/cpp/memory/monotonic_buffer_resource
class ArenaMemoryResource
{
public:
    /// Sets the current buffer to null.
    ArenaMemoryResource() = default;

    /// Sets the current buffer to buffer and the next buffer size to
    /// buffer_size (but not less than 1). Then increase the next buffer
    /// size by an growth factor.
    ArenaMemoryResource(void* buffer, size_t buffer_size) :
        region_ptr(static_cast<char*>(buffer)),
        region_size(buffer_size),
        free_ptr(static_cast<char*>(buffer)),
        next_region_size(buffer_size * 2)
    {
        assert(buffer_size > 0);
    }

    /// Sets the current buffer to null and the next buffer size to
    /// a size no smaller than initial_size.
    explicit ArenaMemoryResource(size_t initial_size) : ArenaMemoryResource()
    {
        assert(initial_size > 0);
        this->next_region_size = initial_size;
    }

    ArenaMemoryResource(ArenaMemoryResource&&) = delete;
    ArenaMemoryResource(const ArenaMemoryResource&) = delete;

    auto operator=(const ArenaMemoryResource&) -> ArenaMemoryResource& = delete;
    auto operator=(ArenaMemoryResource &&) -> ArenaMemoryResource&& = delete;

    ~ArenaMemoryResource() { this->release(); }

    /// Checks whether memory allocated from `rhs` can be deallocated
    /// from `this` and vice-versa.
    auto operator==(const ArenaMemoryResource& rhs) const -> bool
    {
        return this == &rhs;
    }

    /// Returns !(*this == rhs).
    auto operator!=(const ArenaMemoryResource& rhs) const -> bool
    {
        return !(*this == rhs);
    }

    /// Allocates storage from the arena.
    ///
    /// If the current buffer has sufficient unused space to fit a block with
    /// the specified size and alignment, allocates the block from the buffer.
    ///
    /// Otherwise, allocates another buffer bigger than the current one,
    /// assigns it to the current buffer and allocates a block from this buffer.
    auto allocate(size_t bytes, size_t alignment) -> void*
    {
        size_t space{};

        if(region_ptr)
        {
            space = (region_ptr + region_size) - free_ptr;
            if(align(alignment, bytes, free_ptr, space))
            {
                auto* result = free_ptr;
                this->free_ptr += bytes;
                return result;
            }
        }

        // Either we don't have a region allocated or there's not enough
        // space in the current region. Allocate another region.

        // The next region must have space for the header and the requested
        // bytes. In the worst case, it also needs all the alignment bytes.
        const auto requires_size = sizeof(OwnedHeader) + alignment + bytes;
        if(next_region_size < requires_size)
            next_region_size = requires_size;

        auto* next_region_ptr = static_cast<char*>(operator new(
                next_region_size));
        if(next_region_ptr)
        {
            auto* next_region_header = new(next_region_ptr) OwnedHeader;
            next_region_header->prev_region_ptr = region_ptr;
            next_region_header->prev_region_size = region_size;
            next_region_header->owns_prev_region = owns_region;

            this->owns_region = true;
            this->region_ptr = next_region_ptr;
            this->region_size = next_region_size;
            this->free_ptr = next_region_ptr + sizeof(OwnedHeader);
            this->next_region_size *= 2;
            assert(next_region_size != 0);
            assert(free_ptr <= region_ptr + region_size);

            space = (region_ptr + region_size) - free_ptr;
            auto* result = align(alignment, bytes, free_ptr, space);
            assert(result != nullptr);

            this->free_ptr += bytes;
            return result;
        }

        return nullptr;
    }

    /// Does nothing. Memory used by this resource increases monotonically
    /// until its destruction.
    void deallocate(void* ptr, size_t bytes, size_t alignment) {}

    /// Releases all the memory allocated by the arena.
    ///
    /// The current buffer is set to the initial buffer if one was
    /// given in the constructor.
    ///
    /// This does not reset the size of the next region to be allocated.
    void release()
    {
        // Walks backward in the list of regions until we either find the
        // initial buffer we don't own, or we don't have any more buffers.
        while(owns_region && region_ptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto header = *reinterpret_cast<OwnedHeader*>(region_ptr);
            operator delete(region_ptr);
            this->region_ptr = header.prev_region_ptr;
            this->region_size = header.prev_region_size;
            this->owns_region = header.owns_prev_region;
        }

        assert(owns_region == false);
        this->free_ptr = region_ptr;
    }

private:
    char* region_ptr{};      //< The pointer to the current region.
    size_t region_size{};    //< The size of the current region.
    char* free_ptr{};        //< Next free memory in the current region.
    bool owns_region{false}; //< Whether we we the memory to this region.
                             //< Only the first region may be unowned.

    size_t next_region_size{4096}; //< The size of the next memory region.

    /// The first bytes of each region allocated by the arena
    /// contains the following structure.
    struct OwnedHeader
    {
        char* prev_region_ptr;   //< The pointer to the previous region.
        size_t prev_region_size; //< The size of the previous region.
        bool owns_prev_region;   //< Whether the arena owns the previous region.
    };

    static auto align(size_t alignment, size_t bytes, char*& ptr, size_t& space)
            -> char*
    {
        void* void_ptr = ptr;
        if(std::align(alignment, bytes, void_ptr, space))
        {
            ptr = static_cast<char*>(void_ptr);
            return ptr;
        }
        return nullptr;
    }
};

/// An allocator encapsulating an arena memory resource.
///
/// This differs from `std::pmr::polymorphic_allocator` with an
/// `std::pmr::monotonic_buffer_resource` in that the standard returns a
/// default copy constructed polymorphic allocator (i.e. allocates from
/// new/delete) in `select_on_container_copy_construction`, while this
/// allocator returns itself (default allocator_traits semantics).
///
/// This satisfies the requirements of Allocator.
/// https://en.cppreference.com/w/cpp/named_req/Allocator
template<typename T>
class ArenaAllocator
{
public:
    using value_type = T;

    /// This constructor is an implicit conversion from an arena resource.
    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    ArenaAllocator(ArenaMemoryResource* arena) : arena(arena)
    {
        assert(arena != nullptr);
    }

    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    template<typename U> // NOLINTNEXTLINE
    ArenaAllocator(const ArenaAllocator<U>& rhs) : arena(rhs.arena)
    {}

    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    template<typename U> // NOLINTNEXTLINE
    ArenaAllocator(ArenaAllocator<U>&& rhs) : arena(rhs.arena)
    {}

    auto allocate(size_t n) -> T*
    {
        auto* ptr = arena->allocate(n * sizeof(T), alignof(T));
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, size_t n)
    {
        arena->deallocate(p, n * sizeof(T), alignof(T));
    }

    template<typename U>
    auto operator==(const ArenaAllocator<U>& rhs) const -> bool
    {
        return *this->arena == *rhs.arena;
    }

    template<typename U>
    auto operator!=(const ArenaAllocator<U>& rhs) const -> bool
    {
        return !(*this == rhs);
    }

private:
    template<typename U>
    friend class ArenaAllocator;

    ArenaMemoryResource* arena;
};
} // namespace gta3sc

inline auto operator new(std::size_t count, gta3sc::ArenaMemoryResource& arena,
                         std::size_t align) -> void*
{
    return arena.allocate(count, align);
}

inline auto operator new[](std::size_t count,
                           gta3sc::ArenaMemoryResource& arena,
                           std::size_t align) -> void*
{
    return arena.allocate(count, align);
}

inline auto operator new(std::size_t count, gta3sc::ArenaMemoryResource& arena)
        -> void*
{
    return operator new(count, arena, alignof(std::max_align_t));
}

inline auto operator new[](std::size_t count,
                           gta3sc::ArenaMemoryResource& arena) -> void*
{
    return operator new[](count, arena, alignof(std::max_align_t));
}

inline void operator delete(void* ptr, gta3sc::ArenaMemoryResource& arena,
                            std::size_t size)
{}

inline void operator delete[](void* ptr, gta3sc::ArenaMemoryResource& arena,
                              std::size_t size)
{}

inline void operator delete(void* ptr, gta3sc::ArenaMemoryResource& arena)
{}

inline void operator delete[](void* ptr, gta3sc::ArenaMemoryResource& arena)
{}

namespace gta3sc::util
{
namespace detail
{
template<typename UnaryOperation>
inline auto allocate_string(std::string_view from, ArenaMemoryResource& arena,
                            UnaryOperation transform_op) -> std::string_view
{
    const auto result_size = from.size();
    auto* result_ptr = new(arena, alignof(char)) char[result_size];

    for(size_t i = 0; i < from.size(); ++i)
        result_ptr[i] = transform_op(from[i]);

    return {result_ptr, result_size};
}
} // namespace detail

/// Allocates a string in the arena and returns a view to it.
inline auto allocate_string(std::string_view from, ArenaMemoryResource& arena)
        -> std::string_view
{
    constexpr auto identity = [](char c) { return c; };
    return detail::allocate_string(from, arena, identity);
}

/// Allocates a string, converts it to ASCII uppercase, and returns a view to
/// it.
inline auto allocate_string_upper(std::string_view from,
                                  ArenaMemoryResource& arena)
        -> std::string_view
{
    return detail::allocate_string(from, arena, [](char c) {
        if(c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
        return c;
    });
}
} // namespace gta3sc::util

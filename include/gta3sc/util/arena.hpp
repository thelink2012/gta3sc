#pragma once
#include <cassert>
#include <cstddef>
#include <memory>

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

/// Helper base class for arena objects.
///
/// It's not necessary to inherit from this to have an object allocated in the
/// arena. This is simply a helper class to delete copy/move construction and
/// assignment from deriveds, which is necessary if the object isn't intended
/// to have value semantics and can only live allocated in an arena.
struct ArenaObj
{
    ArenaObj() noexcept = default;

    ArenaObj(const ArenaObj&) = delete;
    auto operator=(const ArenaObj&) -> ArenaObj& = delete;

    ArenaObj(ArenaObj&&) noexcept = delete;
    auto operator=(ArenaObj&&) noexcept -> ArenaObj& = delete;

    ~ArenaObj() = default;
};

/// This is a memory resource that releases the allocated memory only when
/// the resource is destroyed. It is intended for very fast memory allocations
/// in situations where memory is used to build up a few objects and then is
/// released all at once.
///
/// This implementation is based on the `std::pmr::monotonic_buffer_resource`
/// interface.
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
    ArenaMemoryResource(void* buffer, size_t buffer_size);

    /// Sets the current buffer to null and the next buffer size to
    /// a size no smaller than initial_size.
    explicit ArenaMemoryResource(size_t initial_size);

    ArenaMemoryResource(ArenaMemoryResource&&) = delete;
    ArenaMemoryResource(const ArenaMemoryResource&) = delete;

    auto operator=(const ArenaMemoryResource&) -> ArenaMemoryResource& = delete;
    auto operator=(ArenaMemoryResource &&) -> ArenaMemoryResource&& = delete;

    ~ArenaMemoryResource() { this->release(); }

    /// Checks whether memory allocated from `rhs` can be deallocated
    /// from `lhs` and vice-versa.
    friend auto operator==(const ArenaMemoryResource& lhs,
                           const ArenaMemoryResource& rhs) noexcept -> bool
    {
        return &lhs == &rhs;
    }

    /// Returns `!(lhs == rhs)`.
    friend auto operator!=(const ArenaMemoryResource& lhs,
                           const ArenaMemoryResource& rhs) noexcept -> bool
    {
        return !(lhs == rhs);
    }

    /// Allocates storage from the arena.
    ///
    /// If the current buffer has sufficient unused space to fit a block with
    /// the specified size and alignment, allocates the block from the buffer.
    ///
    /// Otherwise, allocates another buffer bigger than the current one,
    /// assigns it to the current buffer and allocates a block from this buffer.
    [[nodiscard]] auto allocate(size_t bytes, size_t alignment) -> void*;

    /// Does nothing. Memory used by this resource increases monotonically
    /// until its destruction.
    void deallocate(void* ptr, size_t bytes, size_t alignment) {}

    /// Releases all the memory allocated by the arena.
    ///
    /// The current buffer is set to the initial buffer if one was
    /// given in the constructor.
    ///
    /// This does not reset the size of the next region to be allocated.
    void release();

private:
    char* region_ptr{};      //< The pointer to the current region.
    size_t region_size{};    //< The size of the current region.
    char* free_ptr{};        //< Next free memory in the current region.
    bool owns_region{false}; //< Whether we own the memory to this region.
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
            -> char*;
};

/// An allocator encapsulating an arena memory resource.
///
/// This differs from `std::pmr::polymorphic_allocator` with an
/// `std::pmr::monotonic_buffer_resource` in that the standard returns a
/// default copy constructed polymorphic allocator (i.e. allocates from
/// new/delete) in `select_on_container_copy_construction`, while this
/// allocator returns itself (default allocator_traits semantics).
/// This is because this doesn't handle a polymorphic allocator, like
/// the standard, and so it is only capable of allocating from
/// `ArenaMemoryResource`.
///
/// This satisfies the requirements of Allocator.
/// https://en.cppreference.com/w/cpp/named_req/Allocator
template<typename T = std::byte>
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
    ArenaAllocator(const ArenaAllocator<U>& rhs) noexcept : arena(rhs.arena)
    {}

    [[nodiscard]] auto resource() const -> ArenaMemoryResource*
    {
        return arena;
    }

    [[nodiscard]] auto allocate(size_t n) -> T*
    {
        return allocate_object<T>(n);
    }

    void deallocate(T* p, size_t n) { return deallocate_object<T>(p, n); }

    template<typename U>
    [[nodiscard]] auto allocate_object(std::size_t n = 1) -> U*
    {
        // NOLINTNEXTLINE(bugprone-sizeof-expression): U may be `Aggregate*`.
        return static_cast<U*>(allocate_bytes(n * sizeof(U), alignof(U)));
    }

    template<typename U>
    void deallocate_object(U* p, std::size_t n = 1)
    {
        deallocate_bytes(p, n * sizeof(U), alignof(U));
    }

    [[nodiscard]] auto
    allocate_bytes(size_t nbytes, size_t alignment = alignof(std::max_align_t))
            -> void*
    {
        return arena->allocate(nbytes, alignment);
    }

    void deallocate_bytes(void* p, size_t nbytes,
                          size_t alignment = alignof(std::max_align_t))
    {
        return arena->deallocate(p, nbytes, alignment);
    }

    template<typename U, typename... CtorArgs>
    [[nodiscard]] auto new_object(CtorArgs&&... ctor_args) -> U*
    {
        static_assert(std::is_trivially_destructible_v<U>);
        U* p = allocate_object<U>();
        try
        {
            construct(p, std::forward<CtorArgs>(ctor_args)...);
        }
        catch(...)
        {
            deallocate_object(p);
            throw;
        }
        return p;
    }

    template<typename U>
    void delete_object(U* p)
    {
        destroy(p);
        deallocate(p);
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        static_assert(std::is_trivially_destructible_v<U>);
        std::uninitialized_construct_using_allocator<U>(
                p, *this, std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p)
    {
        p->~U();
    }

    template<typename T1, typename T2>
    friend auto operator==(const ArenaAllocator<T1>& lhs,
                           const ArenaAllocator<T2>& rhs) noexcept -> bool;

    template<typename T1, typename T2>
    friend auto operator!=(const ArenaAllocator<T1>& lhs,
                           const ArenaAllocator<T2>& rhs) noexcept -> bool;

private:
    template<typename U>
    friend class ArenaAllocator;

    ArenaMemoryResource* arena;
};

template<typename T1, typename T2>
inline auto operator==(const ArenaAllocator<T1>& lhs,
                       const ArenaAllocator<T2>& rhs) noexcept -> bool
{
    return *lhs.arena == *rhs.arena;
}

template<typename T1, typename T2>
inline auto operator!=(const ArenaAllocator<T1>& lhs,
                       const ArenaAllocator<T2>& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}
} // namespace gta3sc

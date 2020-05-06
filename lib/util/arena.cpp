#include <gta3sc/util/arena.hpp>

namespace gta3sc
{
ArenaMemoryResource::ArenaMemoryResource(void* buffer, size_t buffer_size) :
    region_ptr(static_cast<char*>(buffer)),
    region_size(buffer_size),
    free_ptr(static_cast<char*>(buffer)),
    next_region_size(buffer_size * 2)
{
    assert(buffer_size > 0);
}

ArenaMemoryResource::ArenaMemoryResource(size_t initial_size) :
    ArenaMemoryResource()
{
    assert(initial_size > 0);
    this->next_region_size = initial_size;
}

auto ArenaMemoryResource::allocate(size_t bytes, size_t alignment) -> void*
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

    auto* next_region_ptr = static_cast<char*>(operator new(next_region_size));
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

void ArenaMemoryResource::release()
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

auto ArenaMemoryResource::align(size_t alignment, size_t bytes, char*& ptr,
                                size_t& space) -> char*
{
    void* void_ptr = ptr;
    if(std::align(alignment, bytes, void_ptr, space))
    {
        ptr = static_cast<char*>(void_ptr);
        return ptr;
    }
    return nullptr;
}
} // namespace gta3sc

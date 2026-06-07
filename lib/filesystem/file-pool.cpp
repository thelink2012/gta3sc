#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <gta3sc/filesystem/file-pool.hpp>
#include <gta3sc/util/memory.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

// TODO eviction / paging

namespace gta3sc::filesystem
{
FilePool::FilePool(FilePool&& other) noexcept :
    files(std::move(other.files)), next_loc(other.next_loc)
{
    other.next_loc = FileLoc{1};
}

auto FilePool::operator=(FilePool&& other) noexcept -> FilePool&
{
    if(this != &other)
    {
        files = std::move(other.files);
        next_loc = other.next_loc;
        other.next_loc = FileLoc{1};
    }
    return *this;
}

auto FilePool::load_file(const std::filesystem::path& path)
        -> std::optional<FileEntryRef>
{
    std::error_code ec;
    const auto file_size = std::filesystem::file_size(path, ec);
    if(ec)
        return std::nullopt;

#ifdef _WIN32
    std::FILE* stream = _wfopen(path.c_str(), L"rb");
#else
    std::FILE* stream = std::fopen(path.c_str(), "rb");
#endif
    if(!stream)
        return std::nullopt;

    // Use file_size + 1 as the initial block size. If the file hasn't changed
    // since the stat, the first fread hits EOF immediately and we're done in
    // one pass. If the file grew between stat and read, the loop continues and
    // picks up the additional bytes.
    const size_t block_size = static_cast<size_t>(file_size) + 1;
    std::unique_ptr<char[]> buf;
    size_t bytes_read = 0;
    size_t n = 0;

    do
    {
        // Grow the buffer by additional block_size bytes.
        {
            auto temp = std::make_unique<char[]>(bytes_read + block_size + 1);
            if(buf)
                std::memcpy(temp.get(), buf.get(), bytes_read);
            buf = std::move(temp);
        }

        n = std::fread(buf.get() + bytes_read, 1, block_size, stream);
        bytes_read += n;
    } while(n == block_size);

    if(!std::feof(stream))
    {
        std::fclose(stream);
        return std::nullopt;
    }

    std::fclose(stream);
    buf[bytes_read] = '\0';
    return load_file_impl(path, std::move(buf), bytes_read);
}

auto FilePool::load_buffer(std::unique_ptr<char[]> data,
                           size_t size) -> std::optional<FileEntryRef>
{
    // `size` is the total allocation including the null terminator; the
    // content occupies data[0..size-2] and the terminator sits at data[size-1].
    assert(size >= 1);
    assert(data[size - 1] == '\0');
    return load_file_impl(std::filesystem::path{}, std::move(data), size - 1);
}

auto FilePool::load_file_impl(const std::filesystem::path& path,
                              std::unique_ptr<char[]> data,
                              size_t size) -> std::optional<FileEntryRef>
{
    assert(data[size] == '\0');

    if(size > static_cast<size_t>(std::numeric_limits<uint32_t>::max()) - 1)
        return std::nullopt;

    // FileLoc overflow check: advancing next_loc by size+1 must not wrap.
    if(next_loc + static_cast<std::ptrdiff_t>(size) + 1 <= next_loc)
        return std::nullopt;

    FileEntry entry;
    entry.path = path;
    entry.start_loc = next_loc;
    entry.file_length = static_cast<uint32_t>(size);
    entry.data = std::move(data);

    if(!path.empty())
    {
        std::error_code ec;
        const auto mtime = std::filesystem::last_write_time(path, ec);
        if(!ec)
            entry.file_mtime = mtime;
    }

    // next_loc starts at FileLoc{1} and only advances, so start_loc >= 1
    // and no_file_loc (i.e. FileLoc{0}) can never fall inside this file's
    // range.
    assert(entry.start_loc > no_file_loc);

    auto [it, inserted] = files.emplace(next_loc, std::move(entry));
    assert(inserted);

    next_loc = next_loc + static_cast<std::ptrdiff_t>(size) + 1;

    return FileEntryRef(it->second);
}

auto FilePool::find_entry(FileRange range) const -> const FileEntry*
{
    auto it = files.upper_bound(range.begin);
    if(it == files.begin())
        return nullptr;
    --it;

    const FileEntry& entry = it->second;
    if(range.end > entry.start_loc + entry.file_length)
        return nullptr;

    return &entry;
}

auto FilePool::string_copy_of(FileRange range) const
        -> std::optional<std::string>
{
    const FileEntry* entry = find_entry(range);
    if(!entry || !entry->data)
        return std::nullopt;

    // Once eviction is implemented we could re-read or repage the file
    // in case entry->data is nullptr.

    const auto offset = range.begin - entry->start_loc;
    return std::string(entry->data.get() + offset, range.size());
}

auto FilePool::string_copy_of(FileRange range, ArenaAllocator<> alloc) const
        -> std::optional<std::string_view>
{
    if(range.size() == 0) // avoid arena allocation for empty ranges
        return std::string_view{};

    const FileEntry* entry = find_entry(range);
    if(!entry || !entry->data)
        return std::nullopt;

    // Once eviction is implemented we could re-read or repage the file
    // in case entry->data is nullptr.

    const auto offset = range.begin - entry->start_loc;
    return util::new_string({entry->data.get() + offset, range.size()}, alloc);
}

FileEntryRef::FileEntryRef(FileEntry& e) : entry(&e)
{
    ++entry->ref_count;
}

FileEntryRef::~FileEntryRef()
{
    if(entry)
        --entry->ref_count;
}

FileEntryRef::FileEntryRef(FileEntryRef&& other) noexcept :
    entry(std::exchange(other.entry, nullptr))
{}

auto FileEntryRef::operator=(FileEntryRef&& other) noexcept -> FileEntryRef&
{
    if(this != &other)
    {
        if(entry)
            --entry->ref_count;
        entry = std::exchange(other.entry, nullptr);
    }
    return *this;
}

auto FileEntryRef::data() const -> const char*
{
    assert(entry != nullptr);
    return entry->data.get();
}

auto FileEntryRef::size() const -> size_t
{
    assert(entry != nullptr);
    return entry->file_length;
}

auto FileEntryRef::view() const -> std::string_view
{
    assert(entry != nullptr);
    return {data(), size()};
}

auto FileEntryRef::location_of(const char* p) const -> FileLoc
{
    assert(entry != nullptr);
    assert(p >= data() && p <= data() + size());
    return entry->start_loc + (p - data());
}

auto FileEntryRef::location_of(std::string_view sv) const -> FileLoc
{
    return location_of(sv.data());
}

auto FileEntryRef::view_of(FileRange range) const -> std::string_view
{
    assert(entry != nullptr);
    assert(range.begin >= entry->start_loc
           && range.end <= entry->start_loc + entry->file_length);
    const auto* begin = data() + (range.begin - entry->start_loc);
    return {begin, range.size()};
}

auto FileEntryRef::path() const -> const std::filesystem::path&
{
    assert(entry != nullptr);
    return entry->path;
}
} // namespace gta3sc::filesystem

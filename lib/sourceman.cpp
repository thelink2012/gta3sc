#include <algorithm>
#include <cassert>
#include <cstring>
#include <gta3sc/sourceman.hpp>

namespace gta3sc
{
bool SourceManager::iequal(std::string_view lhs, std::string_view rhs) const
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                      [](unsigned char ac, unsigned char bc) {
                          if(ac >= 'a' && ac <= 'z')
                              ac -= 32; // transform into upper
                          if(bc >= 'a' && bc <= 'z')
                              bc -= 32;
                          return ac == bc;
                      });
}

bool SourceManager::scan_directory(const std::filesystem::path& dir)
{
    std::error_code ec;
    for(const auto& entry :
        std::filesystem::recursive_directory_iterator(dir, ec))
    {
        if(ec)
            break;

        if(!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        const auto extension = path.extension().u8string();
        if(extension.size() == 3 && extension[0] == '.'
           && (extension[1] == 's' || extension[1] == 'S')
           && (extension[2] == 'c' || extension[2] == 'C'))
        {
            this->filename_to_path.emplace_back(
                    FilenamePathPair{path.filename().generic_string(), path});
        }
    }
    return !!ec;
}

auto SourceManager::load_file(std::string_view filename)
        -> std::optional<SourceFile>
{
    const auto it = std::find_if(
            filename_to_path.begin(), filename_to_path.end(),
            [&](const auto& pair) { return iequal(pair.filename, filename); });

    if(it == filename_to_path.end())
        return std::nullopt;

    return load_file(it->path);
}

auto SourceManager::load_file(const std::filesystem::path& path)
        -> std::optional<SourceFile>
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);

    if(ec)
        return std::nullopt;

#ifdef _WIN32
    std::FILE* stream = _wfopen(path.c_str(), L"rb");
#else
    std::FILE* stream = fopen(path.c_str(), "rb");
#endif

    if(!stream)
        return std::nullopt;

    return load_file(path, stream, size);
}

auto SourceManager::load_file(std::FILE* stream, size_t hint_size)
        -> std::optional<SourceFile>
{
    return load_file(std::filesystem::path(), stream, hint_size);
}

auto SourceManager::load_file(std::unique_ptr<char[]> data, size_t size)
        -> std::optional<SourceFile>
{
    return load_file(std::filesystem::path(), std::move(data), size);
}

auto SourceManager::load_file(const std::filesystem::path& path,
                              std::FILE* stream, size_t hint_size)
        -> std::optional<SourceFile>
{
    // TODO should this detect the encoding of the file and convert to utf8?

    std::unique_ptr<char[]> source_data;
    size_t source_size = 0; // not including null terminator

    // Add one to the hint_size so we can trigger EOF on the first iteration.
    const size_t block_size = (hint_size == size_t(-1) ? 4096 : 1 + hint_size);

    while(true)
    {
        const auto block_pos = source_size;

        // Reallocate the unique pointer (plus space for null terminator).
        auto temp_source_data = std::make_unique<char[]>(1 + source_size
                                                         + block_size);
        std::memcpy(temp_source_data.get(), source_data.get(), source_size);

        source_data = std::move(temp_source_data);
        source_size += block_size;

        if(const auto ncount = std::fread(&source_data[block_pos], 1,
                                          block_size, stream);
           ncount < block_size)
        {
            if(std::feof(stream))
            {
                source_size = block_pos + ncount;
                break;
            }
            return std::nullopt;
        }
    }

    source_data[source_size] = '\0';
    return load_file(path, std::move(source_data), source_size);
}

auto SourceManager::load_file(const std::filesystem::path& path,
                              std::unique_ptr<char[]> data, size_t size)
        -> std::optional<SourceFile>
{
    assert(data[size] == '\0');

    if(size > std::numeric_limits<decltype(SourceInfo::file_length)>::max() - 1)
        return std::nullopt;

    if(next_source_loc + size + 1 <= next_source_loc)
        return std::nullopt; // overflow

    SourceInfo info;
    info.path = path;
    info.start_loc = next_source_loc;
    info.file_length = size;
    info.data = std::move(data);

    assert(no_source_loc < info.start_loc
           || no_source_loc > info.start_loc + info.file_length);

    auto [it, _] = this->source_infos.emplace(next_source_loc, std::move(info));
    this->next_source_loc = next_source_loc + size + 1;

    return SourceFile(it->second);
}
} // namespace gta3sc

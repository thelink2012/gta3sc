#include <algorithm>
#include <cassert>
#include <gta3sc/sourceman.hpp>
#include <cstring>

namespace gta3sc
{
SourceFile::SourceFile(std::unique_ptr<char[]> source_data_a, size_t source_size_a) :
    source_data(std::move(source_data_a)), source_size(source_size_a)
{
    // Discover line locations.
    this->lines.reserve(source_size_a / 80);
    this->lines.push_back(&source_data[0]);
    for(size_t i = 0; i < source_size; ++i)
    {
        if(source_data[i] == '\n')
            this->lines.push_back(&source_data[i + 1]);
    }
    this->lines.shrink_to_fit();
}

auto SourceFile::from_stream(std::FILE* stream, size_t hint_size)
        -> std::optional<SourceFile>
{
    std::unique_ptr<char[]> source_data, temp_source_data;
    size_t source_size = 0; //< not including null terminator

    // Add one to the hint_size so we can trigger EOF on the first iteration.
    const size_t block_size = (hint_size == size_t(-1) ? 4096 : 1 + hint_size);

    while(true)
    {
        auto block_pos = source_size;

        // Reallocate the unique pointer (plus space for null terminator).
        temp_source_data = std::make_unique<char[]>(1 + source_size + block_size);
        std::memcpy(temp_source_data.get(), source_data.get(), source_size);
        std::swap(source_data, temp_source_data);
        source_size += block_size;

        auto ncount = std::fread(&source_data[block_pos], 1, block_size, stream);

        if(ncount < block_size)
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
    return SourceFile{std::move(source_data), source_size};
}

auto SourceFile::find_line_and_column(SourceLocation loc) const
        -> std::pair<unsigned, unsigned>
{
    assert(loc >= &this->source_data[0] && loc <= &this->source_data[source_size]);

    auto it_line_end = std::upper_bound(lines.begin(), lines.end(), loc);
    auto line = static_cast<unsigned>(std::distance(lines.begin(), it_line_end));

    assert(it_line_end != lines.begin());
    auto it_line_begin = std::prev(it_line_end);
    auto column = static_cast<unsigned>(1 + std::distance(*it_line_begin, loc));

    return {line, column};
}

auto SourceFile::view_with_terminator() const -> SourceRange
{
    return SourceRange(&source_data[0], source_size + 1);
}
}

#include "gta3sc/diagnostics.hpp"
#include "gta3sc/sourceman.hpp"
#include <filesystem>
#include <gta3sc/config/models.hpp>
#include <string_view>

// TODO replace SourceManager (and SourceFile) by FileManager once 
//      sourceman gets refactored.

namespace
{
auto next_line(const char*& cursor, char* output_buf,
               size_t output_size) noexcept -> size_t;
} // namespace

namespace gta3sc::config
{
auto load_models_from_ide(const std::filesystem::path& ide_path, bool objs_only,
                          SourceManager& fileman, DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&
{
    auto ide_file = fileman.load_file(ide_path);
    if(!ide_file)
    {
        diagman.report(SourceManager::no_source_loc,
                       Diag::config_models_could_not_open_file)
                .args(ide_path.generic_string());
        return std::move(builder);
    }

    return load_models_from_ide(*ide_file, objs_only, diagman,
                                std::move(builder));
}

auto load_models_from_level(const std::filesystem::path& root_path,
                            const std::filesystem::path& level_path,
                            bool objs_only, SourceManager& fileman,
                            DiagnosticHandler& diagman,
                            ArenaAllocator<> allocator) -> ModelTable
{
    return load_models_from_level(root_path, level_path, objs_only, fileman,
                                  diagman, ModelTable::Builder(allocator))
            .build();
}

auto load_models_from_level(
        const std::filesystem::path& root_path,
        const std::filesystem::path& level_path, bool objs_only,
        SourceManager& fileman, DiagnosticHandler& diagman,
        ModelTable::Builder&& builder) -> ModelTable::Builder&&
{
    auto level_file = fileman.load_file(level_path);
    if(!level_file)
    {
        diagman.report(SourceManager::no_source_loc,
                       Diag::config_models_could_not_open_file)
                .args(level_path.generic_string());
        return std::move(builder);
    }

    char line_buf[512];
    size_t line_len{};
    auto curr_file_cursor = level_file->code_data();

    for(auto line_cursor_start = curr_file_cursor;
        (line_len = next_line(curr_file_cursor, line_buf, std::size(line_buf)));
        line_cursor_start = curr_file_cursor)
    {
        const std::string_view line{line_buf, line_len};

        if(!line.starts_with("IDE"))
            continue;

        const auto ide_relative_path = line.substr(4);
        const auto ide_path = root_path / ide_relative_path;

        if(!std::filesystem::is_regular_file(ide_path))
        {
            auto loc_start = level_file->location_of(line_cursor_start);
            auto loc_end = loc_start
                            + (curr_file_cursor - line_cursor_start);
            diagman.report(loc_start,
                            Diag::config_models_could_not_open_file)
                    .range(SourceRange{loc_start, loc_end})
                    .args(ide_path.generic_string());

            continue;
        }


        builder = load_models_from_ide(ide_path, objs_only, fileman,
                                        diagman, std::move(builder));
    }

    return std::move(builder);
}

auto load_models_from_ide(
        const SourceFile& ide_file, bool objs_only, DiagnosticHandler& diagman,
        ModelTable::Builder&& builder) -> ModelTable::Builder&&
{
    char line_buf[128];
    bool is_in_section{};
    bool is_readable_section{};
    auto curr_file_cursor = ide_file.code_data();

    size_t line_len{};
    for(auto line_cursor_start = curr_file_cursor;
        (line_len = next_line(curr_file_cursor, line_buf, std::size(line_buf)));
        line_cursor_start = curr_file_cursor)
    {
        const std::string_view line{line_buf, line_len};

        if(!is_in_section)
        {
            is_in_section = true;
            if(line.starts_with("objs") || line.starts_with("tobj")
               || line.starts_with("anim"))
                is_readable_section = true;
            else
                is_readable_section = !objs_only;
            continue;
        }

        if(line.starts_with("end"))
        {
            is_in_section = false;
            is_readable_section = false;
            continue;
        }

        if(!is_readable_section)
            continue;

        uint32_t id{};
        char model_name[64];
        if(std::sscanf(line_buf, "%u %63s", &id, model_name) != 2) // NOLINT
        {
            auto loc_start = ide_file.location_of(line_cursor_start);
            auto loc_end = loc_start + (curr_file_cursor - line_cursor_start);
            diagman.report(loc_start, Diag::config_models_invalid_ide_line)
                    .range(SourceRange{loc_start, loc_end});
            continue;
        }

        builder.insert_model(model_name, id);
    }

    return std::move(builder);
}
} // namespace gta3sc::config

namespace
{
constexpr auto is_whitespace(char c) noexcept -> bool
{
    return c == ' ' || c == ',' || c == '\t' || c == '\r';
}

constexpr auto is_newline(char c) noexcept -> bool
{
    return c == '\0' || c == '\n';
}

/// Reads the next non-empty line from the character stream.
///
/// Transforms commas and control characters to spaces, trims leading and
/// trailing spaces, and skips comment lines.
///
/// In case the buffer is not large enough, the line is truncated.
///
/// \param cursor the cursor in the null-terminated character stream to read
/// from. The cursor is passed as reference and advanced to the line that
/// follows.
/// \param output_buf the output buffer to write the line to.
/// \param output_size the size of the output buffer.
/// \param output_len the length of the resulting output line.
/// \return the length of the line read, or `0` if end of stream reached.
auto next_line(const char*& cursor, char* output_buf,
               size_t output_size) noexcept -> size_t
{
    assert(output_size > 0);

    if(*cursor == '\0')
        return 0;

    if(*cursor == '\n')
        return next_line(++cursor, output_buf, output_size);

    auto output_end = output_buf;
    const auto* output_max = (output_buf + output_size) - 1;

    while(is_whitespace(*cursor) && !is_newline(*cursor))
        ++cursor;

    if(*cursor == '#')
    {
        while(!is_newline(*cursor))
            ++cursor;
        return next_line(cursor, output_buf, output_size);
    }

    for(; output_end < output_max && !is_newline(*cursor); ++cursor)
    {
        *output_end++ = is_whitespace(*cursor) ? ' ' : *cursor;
    }

    *output_end = '\0';

    while(output_end > output_buf && is_whitespace(output_end[-1]))
        *--output_end = '\0';

    const auto result_len = output_end - output_buf;

    if(result_len == 0)
        return next_line(cursor, output_buf, output_size);

    return result_len;
}
} // namespace
#include <cassert>
#include <filesystem>
#include <gta3sc/config/models.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/filesystem/file-pool.hpp>
#include <gta3sc/filesystem/path-resolver.hpp>
#include <gta3sc/util/ctype.hpp>
#include <string>
#include <string_view>

namespace gta3sc::config::diag
{
// Config Models diagnostics
const DiagnosticDescriptor models_invalid_ide_line(DiagnosticSeverity::error,
                                                   "Invalid IDE line", "TODO");
} // namespace gta3sc::config::diag

namespace
{
auto next_line(const char*& cursor, char* output_buf,
               size_t output_size) noexcept -> size_t;
} // namespace

namespace gta3sc::config
{
auto load_models_from_ide(const std::filesystem::path& ide_path, bool objs_only,
                          FilePool& file_pool, DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&
{
    auto ide_file = file_pool.load_file(ide_path);
    if(!ide_file)
    {
        diagman.report(no_file_loc, gta3sc::diag::could_not_open_file)
                .args(ide_path.generic_string());
        return std::move(builder);
    }

    // TODO page out the level file when done

    return load_models_from_ide(*ide_file, objs_only, diagman,
                                std::move(builder));
}

auto load_models_from_level(
        const gta3sc::filesystem::PathResolver& path_resolver,
        const std::filesystem::path& level_path, bool objs_only,
        FilePool& file_pool, DiagnosticHandler& diagman,
        ArenaAllocator<> allocator) -> ModelTable
{
    return load_models_from_level(path_resolver, level_path, objs_only,
                                  file_pool, diagman,
                                  ModelTable::Builder(allocator))
            .build();
}

auto load_models_from_level(
        const gta3sc::filesystem::PathResolver& path_resolver,
        const std::filesystem::path& level_path, bool objs_only,
        FilePool& file_pool, DiagnosticHandler& diagman,
        ModelTable::Builder&& builder) -> ModelTable::Builder&&
{
    auto level_file = file_pool.load_file(level_path);
    if(!level_file)
    {
        diagman.report(no_file_loc, gta3sc::diag::could_not_open_file)
                .args(level_path.generic_string());
        return std::move(builder);
    }

    // TODO page out the level file when done

    char line_buf[512];
    size_t line_len{};
    auto curr_file_cursor = level_file->data();

    for(auto line_cursor_start = curr_file_cursor;
        (line_len = next_line(curr_file_cursor, line_buf, std::size(line_buf)));
        line_cursor_start = curr_file_cursor)
    {
        const std::string_view line{line_buf, line_len};

        if(!line.starts_with("IDE") || line.size() <= 4)
            continue;

        const auto relative_ide_path = line.substr(4);
        const auto resolved_ide_path = path_resolver.resolve(relative_ide_path);

        const auto line_loc_start = level_file->location_of(line_cursor_start);
        const auto line_loc_end = line_loc_start
                                  + (curr_file_cursor - line_cursor_start);
        const auto line_range = FileRange{line_loc_start, line_loc_end};

        if(!resolved_ide_path)
        {
            diagman.report(line_loc_start, gta3sc::diag::could_not_open_file)
                    .range(line_range)
                    .args(relative_ide_path);
            continue;
        }

        auto ide_file = file_pool.load_file(*resolved_ide_path);
        if(!ide_file)
        {
            diagman.report(line_loc_start, gta3sc::diag::could_not_open_file)
                    .range(line_range)
                    .args(resolved_ide_path->generic_string());
            continue;
        }

        load_models_from_ide(*ide_file, objs_only, diagman, std::move(builder));
    }

    // Return the same rvalue reference as given as input.
    return std::move(builder);
}

auto load_models_from_ide(const FileEntryRef& ide_file, bool objs_only,
                          DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&
{
    char line_buf[128];
    bool is_in_section{};
    bool is_readable_section{};
    auto curr_file_cursor = ide_file.data();

    size_t line_len{};
    for(auto line_cursor_start = curr_file_cursor;
        (line_len = next_line(curr_file_cursor, line_buf, std::size(line_buf)));
        line_cursor_start = curr_file_cursor)
    {
        const std::string_view line{line_buf, line_len};

        if(line.starts_with("end"))
        {
            is_in_section = false;
            is_readable_section = false;
            continue;
        }

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

        if(!is_readable_section)
            continue;

        // TODO error if id is over int32
        uint32_t id{};
        char model_name[64];
        if(std::sscanf(line_buf, "%u %63s", &id, model_name) != 2) // NOLINT
        {
            const auto loc_start = ide_file.location_of(line_cursor_start);
            const auto loc_end = loc_start
                                 + (curr_file_cursor - line_cursor_start);
            diagman.report(loc_start, config::diag::models_invalid_ide_line)
                    .range(FileRange{loc_start, loc_end});
            continue;
        }

        // Model search is case sensitive and when stored it is uppercase, so
        // convert it first.
        std::string_view model_name_view(model_name);
        std::for_each(model_name_view.begin(), model_name_view.end(),
                      util::toupper);

        builder.insert_model(model_name_view, id);
    }

    // Return the same rvalue reference as given as input.
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
/// Also transforms '\\' into the platform path separator.
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
        if(is_whitespace(*cursor))
            *output_end++ = ' ';
        else if(*cursor == '\\')
            *output_end++ = std::filesystem::path::preferred_separator;
        else
            *output_end++ = *cursor;
    }

    // skip rest of line in case the output buffer was exhausted
    while(!is_newline(*cursor))
        ++cursor;

    *output_end = '\0';

    while(output_end > output_buf && is_whitespace(output_end[-1]))
        *--output_end = '\0';

    const auto result_len = output_end - output_buf;

    if(result_len == 0)
        return next_line(cursor, output_buf, output_size);

    return result_len;
}
} // namespace

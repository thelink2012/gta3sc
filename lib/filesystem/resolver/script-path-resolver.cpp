#include <algorithm>
#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/string.hpp>
#include <optional>
#include <system_error>

namespace gta3sc::filesystem
{
auto ScriptPathResolver::scan_directory(const std::filesystem::path& dir)
        -> bool
{
    std::error_code ec;
    // Symlink cycles could loop indefinitely but we are accepting this limitation
    // since we expect script trees to be small. But maybe FIXME?
    for(const auto& entry : std::filesystem::recursive_directory_iterator(
                dir,
                std::filesystem::directory_options::follow_directory_symlink,
                ec))
    {
        if(ec)
            break;

        if(!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        const auto extension = path.extension().string();

        if(extension.size() == 3 && extension[0] == '.'
           && util::toupper(extension[1]) == 'S'
           && util::toupper(extension[2]) == 'C')
        {
            filename_to_path.push_back(
                    {.filename = path.filename().generic_string(),
                     .path = path});
        }
    }
    return !ec;
}

auto ScriptPathResolver::resolve(std::string_view filename) const
        -> std::optional<std::filesystem::path>
{
    const auto it = std::find_if(
            filename_to_path.begin(), filename_to_path.end(),
            [&](const FilenamePath& pair) {
                return util::insensitive_equal(pair.filename, filename);
            });

    if(it == filename_to_path.end())
        return std::nullopt;

    return it->path;
}

} // namespace gta3sc::filesystem

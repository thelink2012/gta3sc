#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>
#include <gta3sc/util/string.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// TODO make win32 version be a simple RelativePathResolver.
//      evaluate whether we should create a new type that abstracts choice across platforms
//      or do it in this type.

namespace gta3sc::filesystem
{
RelativeInsensitivePathResolver::RelativeInsensitivePathResolver(
        std::filesystem::path root_path)
{
    std::error_code ec;
    root = std::filesystem::weakly_canonical(root_path, ec);
    if(ec)
        root = std::move(root_path);
}

auto RelativeInsensitivePathResolver::resolve(std::string_view relative_path)
        const -> std::optional<std::filesystem::path>
{
    // Make behavior consistent with RelativePathResolver.
    if(relative_path.empty())
        return root / relative_path;

    // TODO: dir_cache is not thread-safe; concurrent calls to resolve() on the
    // same instance are a data race. Protect with a (RW) mutex where necessary.

    // TODO: there is a significant number of optimization opportuniites in this class!
    //    1. dir_cache (std::map + std::vector<std::string>) is an MVP using arenas
    //    2. storing the relative path as key rather than root + relative.
    //    3. storing full relative paths and starting the search from the last component.

    std::filesystem::path current_root = root;

    // current_root and relative_path will be modified in place during this
    // loop. but they maintain the following invariants:
    //  - current_root is known to (probably) exist
    //  - current_root + relative_path is the path to be resolved.
    while(!relative_path.empty())
    {
        // TODO: std::filesystem::exists(current_root + remaining) here would
        // short-circuit the next iteration if current_root + remaining already
        // exists and doesn't need further insensitive resolution.

        // Extract current component.
        const auto sep = relative_path.find_first_of("/\\");
        const auto component = relative_path.substr(0, sep);

        // Advance relative_path to the next component.
        relative_path = sep == std::string_view::npos
                                ? std::string_view{}
                                : relative_path.substr(sep + 1);

        if(component.empty() || component == ".")
            continue;

        const auto* resolved_component = resolve_component(current_root, component);
        if(resolved_component == nullptr)
            return std::nullopt;

        current_root /= *resolved_component;
    }

    return current_root;
}

auto RelativeInsensitivePathResolver::list_directory(
        const std::filesystem::path& dir) const
        -> const std::vector<std::string>*
{
    auto cache_it = dir_cache.find(dir);
    if(cache_it == dir_cache.end())
    {
        std::vector<std::string> entries;
        std::error_code ec;
        for(const auto& entry : std::filesystem::directory_iterator(dir, ec))
            entries.push_back(entry.path().filename().generic_string());
        if(ec)
            return nullptr;
        cache_it = dir_cache.emplace(dir, std::move(entries)).first;
    }
    return &cache_it->second;
}

auto RelativeInsensitivePathResolver::find_entry(
        const std::vector<std::string>& entries,
        std::string_view name) -> const std::string*
{
    for(const auto& entry : entries)
    {
        if(util::insensitive_equal(entry, name))
            return &entry;
    }
    return nullptr;
}

auto RelativeInsensitivePathResolver::resolve_component(
        const std::filesystem::path& dir,
        std::string_view name) const -> const std::string*
{
    const auto* entries = list_directory(dir);
    if(entries == nullptr)
        return nullptr;
    return find_entry(*entries, name);
}

} // namespace gta3sc::filesystem

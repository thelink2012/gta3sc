#include <gta3sc/source-manager.hpp>

namespace gta3sc
{
auto SourceManager::scan_directory(const std::filesystem::path& dir) -> bool
{
    return this->script_resolver.scan_directory(dir);
}

auto SourceManager::load_file(std::string_view filename)
        -> std::optional<filesystem::FileEntryRef>
{
    auto path = this->script_resolver.resolve(filename);
    if(!path)
        return std::nullopt;
    return filesystem::FilePool::load_file(*path);
}
} // namespace gta3sc

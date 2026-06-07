#include <filesystem>
#include <gta3sc/filesystem/path-resolver.hpp>
#include <optional>
#include <system_error>

namespace gta3sc::filesystem
{
RelativePathResolver::RelativePathResolver(std::filesystem::path root_path)
{
    std::error_code ec;
    this->root = std::filesystem::weakly_canonical(root_path, ec);
    if(ec)
        this->root = std::move(root_path);
}

auto RelativePathResolver::resolve(std::string_view relative_path) const
        -> std::optional<std::filesystem::path>
{
    return root / relative_path;
}
} // namespace gta3sc::filesystem

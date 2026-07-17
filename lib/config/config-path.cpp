#include <cstdlib>
#include <filesystem>
#include <gta3sc/config/config-path.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__) || defined(__FreeBSD__)
#include <unistd.h>
#else
#error "Unsupported platform"
#endif

namespace gta3sc::config
{
auto config_search_paths(const std::filesystem::path& exe_dir,
                         const std::filesystem::path& home)
        -> std::vector<std::filesystem::path>
{
    std::vector<std::filesystem::path> paths;
    if(!exe_dir.empty())
        paths.push_back(exe_dir / "config");
#if !defined(_WIN32)
    if(!home.empty())
        paths.push_back(home / ".local/share/gta3sc/config");
    paths.emplace_back("/usr/share/gta3sc/config");
#endif
    return paths;
}

auto find_config_root() -> std::optional<std::filesystem::path>
{
    namespace fs = std::filesystem;

    fs::path exe_dir;
    fs::path home_dir;

    if(const char* env_root = std::getenv("GTA3SC_CONFIG_ROOT"))
    {
        fs::path result{env_root};
        if(result.empty() || !result.is_absolute() || !fs::is_directory(result))
            return std::nullopt;
        return result;
    }

#if defined(_WIN32)
    {
        wchar_t buf[4096]{};
        auto n = GetModuleFileNameW(nullptr, buf,
                                    static_cast<DWORD>(std::size(buf)));
        if(n > 0 && n < std::size(buf))
            exe_dir = fs::path(buf).parent_path();
    }
#elif defined(__APPLE__)
    {
        uint32_t size = 4096;
        char buf[4096]{};
        if(_NSGetExecutablePath(buf, &size) == 0)
            exe_dir = fs::path(buf).parent_path();
    }
#elif defined(__linux__) || defined(__FreeBSD__)
    {
        char buf[4096]{};
#if defined(__linux__)
        ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
#else
        ssize_t len = ::readlink("/proc/curproc/file", buf, sizeof(buf) - 1);
#endif
        if(len > 0)
        {
            buf[len] = '\0';
            exe_dir = fs::path(buf).parent_path();
        }
    }
#else
#error "Unsupported platform"
#endif

    if(const char* home = std::getenv("HOME"); home != nullptr)
        home_dir = home;

    for(const auto& candidate : config_search_paths(exe_dir, home_dir))
    {
        if(fs::is_directory(candidate))
            return candidate;
    }
    return std::nullopt;
}

} // namespace gta3sc::config

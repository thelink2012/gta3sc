#pragma once
#include <filesystem>
#include <optional>
#include <vector>

namespace gta3sc::config
{
/// Returns ordered config-root candidates for the host platform.
///
/// The returned entries are typically:
/// - Linux/macOS: exe-adjacent `config/`, then
///   `home/.local/share/gta3sc/config`, then `/usr/share/gta3sc/config`.
/// - Windows: exe-adjacent `config/`.
///
/// \param exe_dir  Directory containing the gta3sc executable.
/// \param home     User home directory (e.g. `$HOME`).
auto config_search_paths(const std::filesystem::path& exe_dir,
                         const std::filesystem::path& home)
        -> std::vector<std::filesystem::path>;

/// Returns the config root directory.
///
/// If the `GTA3SC_CONFIG_ROOT` environment variable is set, its value is used
/// exclusively (other paths discovery is skipped). If the environment variable
/// is invalid, then `std::nullopt` is returned.
///
/// When the variable is unset, returns the first candidate from
/// \ref config_search_paths that is an existing directory, or `std::nullopt`
/// if none exists.
///
/// \note This function has several read side effects such as filesystem
/// access, system calls, etc).
auto find_config_root() -> std::optional<std::filesystem::path>;
} // namespace gta3sc::config

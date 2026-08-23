#pragma once
#include <string_view>

namespace gta3sc::cli
{
/// Git identity of this build.
///
/// The string is `<tag>-<hash>` when HEAD was exactly tagged, otherwise
/// `<hash>`. A `-dirty` suffix is appended when the worktree had uncommitted
/// changes. Fallbacks to `unknown` when git was unavailable during build.
[[nodiscard]] auto version() noexcept -> std::string_view;

} // namespace gta3sc::cli

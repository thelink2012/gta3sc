#pragma once
#include <cstddef>

namespace gta3sc::codegen::trilogy
{
/// Target GTA Trilogy title for codegen layout differences.
enum class TrilogyGame
{
    Gta3,
    Gtavc,
    Gtasa,
};

[[nodiscard]] constexpr auto
global_var_header_marker(TrilogyGame game) -> std::byte
{
    switch(game)
    {
        case TrilogyGame::Gta3:
            return std::byte{0x00};
        case TrilogyGame::Gtavc:
        case TrilogyGame::Gtasa:
            return std::byte{0x6D};
    }
    return std::byte{0x00};
}

[[nodiscard]] constexpr auto uses_q11_4_floats(TrilogyGame game) -> bool
{
    return game == TrilogyGame::Gta3;
}

} // namespace gta3sc::codegen::trilogy

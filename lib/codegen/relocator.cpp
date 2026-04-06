#include <bit>
#include <gta3sc/codegen/relocator.hpp>
using std::bit_cast;

namespace gta3sc::codegen
{
bool Relocator::relocate(RelocationTable& reloc_table,
                         DiagnosticHandler& diagman) const
{
    bool has_error = false;

    for(const auto& entry : reloc_table.fixup_table())
    {
        if(!relocate(reloc_table, entry, diagman))
            has_error = true;
    }

    for(const auto& entry : reloc_table.file_fixup_table())
    {
        if(!relocate(reloc_table, entry, diagman))
            has_error = true;
    }

    return !has_error;
}

bool Relocator::relocate(RelocationTable& reloc_table,
                         const RelocationTable::FixupEntry& entry,
                         DiagnosticHandler& diagman) const
{
    auto offset = reloc_table.relocate(entry, diagman);
    if(!offset)
        return false;

    relocate(*bytecode, entry.offset, *offset);
    return true;
}

bool Relocator::relocate(RelocationTable& reloc_table,
                         const RelocationTable::FileFixupEntry& entry,
                         DiagnosticHandler& diagman) const
{
    auto offset = reloc_table.relocate(entry, diagman);
    if(!offset)
        return false;

    relocate(*bytecode, entry.offset, *offset);
    return true;
}

void Relocator::relocate(std::vector<std::byte>& bytecode, uint32_t at,
                         int32_t target)
{
    assert(bytecode.size() >= at + 4);

    // Same code as Emitter::emit_raw_i32.

    bytecode[at] = bit_cast<std::byte>(
            static_cast<uint8_t>(target & 0x000000FFU));

    bytecode[at + 1] = bit_cast<std::byte>(
            static_cast<uint8_t>((target & 0x0000FF00U) >> 8U));

    bytecode[at + 2] = bit_cast<std::byte>(
            static_cast<uint8_t>((target & 0x00FF0000U) >> 16U));

    bytecode[at + 3] = bit_cast<std::byte>(
            static_cast<uint8_t>((target & 0xFF000000U) >> 24U));
}
} // namespace gta3sc::codegen

// TODO provide a more lazy relocation interface such that we can relocate
// missions one by one instead of having to relocate the entire multifile.
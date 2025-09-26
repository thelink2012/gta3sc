#include <bit>
#include <gta3sc/codegen/relocator.hpp>
using std::bit_cast;

namespace gta3sc::codegen
{
bool Relocator::relocate(RelocationTable& reloc_table,
                         DiagnosticHandler& diagman) const
{
    // TODO keep going after an error

    for(const auto& entry : reloc_table.fixup_table())
    {
        if(!relocate(reloc_table, entry, diagman))
            return false;
    }

    for(const auto& entry : reloc_table.file_fixup_table())
    {
        if(!relocate(reloc_table, entry, diagman))
            return false;
    }

    return true;
}

bool Relocator::relocate(RelocationTable& reloc_table,
                         const RelocationTable::FixupEntry& entry,
                         DiagnosticHandler& diagman) const
{
    auto offset = reloc_table.relocate(entry, diagman);
    if(!offset)
        return false;

    return relocate(*output, entry.offset, *offset);
}

bool Relocator::relocate(RelocationTable& reloc_table,
                         const RelocationTable::FileFixupEntry& entry,
                         DiagnosticHandler& diagman) const
{
    auto offset = reloc_table.relocate(entry, diagman);
    if(!offset)
        return false;

    return relocate(*output, entry.offset, *offset);
}

bool Relocator::relocate(std::vector<std::byte>& output, uint32_t at,
                         int32_t target_value)
{
    if(output.size() < at + 4)
        return false; // TODO diag?

    // Same code as Emitter::emit_raw_i32.

    output[at] = bit_cast<std::byte>(
            static_cast<uint8_t>(target_value & 0x000000FFU));

    output[at + 1] = bit_cast<std::byte>(
            static_cast<uint8_t>((target_value & 0x0000FF00U) >> 8U));

    output[at + 2] = bit_cast<std::byte>(
            static_cast<uint8_t>((target_value & 0x00FF0000U) >> 16U));

    output[at + 3] = bit_cast<std::byte>(
            static_cast<uint8_t>((target_value & 0xFF000000U) >> 24U));

    return true;
}
} // namespace gta3sc::codegen
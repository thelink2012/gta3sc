#pragma once
#include <gta3sc/codegen/relocation-table.hpp>

namespace gta3sc
{
class DiagnosticHandler;
} // namespace gta3sc

namespace gta3sc::codegen
{
class Relocator
{
public:
    Relocator(std::vector<std::byte>& output) : output(&output) {}

    Relocator(Relocator&&) noexcept = default;
    auto operator=(Relocator&&) noexcept -> Relocator& = default;

    Relocator(const Relocator&) = delete;
    auto operator=(const Relocator&) -> Relocator& = delete;

    ~Relocator() noexcept = default;

    bool relocate(RelocationTable& reloc_table,
                  DiagnosticHandler& diagman) const;

    bool relocate(RelocationTable& reloc_table,
                  const RelocationTable::FixupEntry& entry,
                  DiagnosticHandler& diagman) const;

    bool relocate(RelocationTable& reloc_table,
                  const RelocationTable::FileFixupEntry& entry,
                  DiagnosticHandler& diagman) const;

    static bool relocate(std::vector<std::byte>& output, uint32_t at,
                         int32_t target_value);

private:
    std::vector<std::byte>* output;
};
} // namespace gta3sc::codegen
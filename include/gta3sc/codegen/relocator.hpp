#pragma once
#include <gta3sc/codegen/relocation-table.hpp>

namespace gta3sc
{
class DiagnosticHandler;
} // namespace gta3sc

namespace gta3sc::codegen
{
/// Relocator fixups relocation offsets in the bytecode.
///
/// Given a relocation table and fixup entries, patches the bytecode
/// such that labels are resolved to their correct offsets.
class Relocator
{
public:
    /// Constructs a relocator for the given bytecode.
    ///
    /// All following relocate calls assume the bytecode is large enough for the
    /// fixups.
    explicit Relocator(std::vector<std::byte>& bytecode) : bytecode(&bytecode)
    {}

    Relocator(Relocator&&) noexcept = default;
    auto operator=(Relocator&&) noexcept -> Relocator& = default;

    Relocator(const Relocator&) = delete;
    auto operator=(const Relocator&) -> Relocator& = delete;

    ~Relocator() noexcept = default;

    /// Relocates the bytecode according to the given relocation table.
    ///
    /// In case of failure, `false` is returned and a diagnostic is produced.
    [[nodiscard]] bool relocate(RelocationTable& reloc_table,
                                DiagnosticHandler& diagman) const;

    /// Relocates the bytecode according to the given fixup entry.
    ///
    /// In case of failure, `false` is returned and a diagnostic is produced.
    [[nodiscard]] bool relocate(RelocationTable& reloc_table,
                                const RelocationTable::FixupEntry& entry,
                                DiagnosticHandler& diagman) const;

    /// Relocates the bytecode according to the given file fixup entry.
    ///
    /// In case of failure, `false` is returned and a diagnostic is produced.
    [[nodiscard]] bool relocate(RelocationTable& reloc_table,
                                const RelocationTable::FileFixupEntry& entry,
                                DiagnosticHandler& diagman) const;

    /// Relocates the value at `at` in `bytecode` to value `target`.
    ///
    /// Effectivelly writes 4 bytes at `bytecode[at] = target` in an
    /// endian-independent fashion.
    ///
    /// Assumes there is enough space in `bytecode` to write the 4 bytes.
    static void relocate(std::vector<std::byte>& bytecode, uint32_t at,
                         int32_t target);

private:
    std::vector<std::byte>* bytecode;
};
} // namespace gta3sc::codegen
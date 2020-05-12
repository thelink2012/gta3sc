#pragma once
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/container-view.hpp>

namespace gta3sc
{
class DiagnosticHandler;
} // namespace gta3sc

namespace gta3sc::codegen
{
/// Table storing information about offsets that need relocation.
///
/// This table stores the offset where labels are defined as well as used.
///
/// The table is populated as code is generated. After code generation,
/// one should iterate on its entries and fixup the word at the specified
/// entry (which is a label reference) to the now known label offset.
class RelocationTable
{
public:
    using AbsoluteOffset = uint32_t;
    using RelativeOffset = int32_t;

    /// Used to mark uninitialized `AbsoluteOffset`s.
    static constexpr AbsoluteOffset invalid_offset = -1;

    /// Describes the location of a label.
    struct LabelLoc
    {
        /// File where the label is located.
        const SymbolTable::File* origin_file{};
        /// Label being described.
        const SymbolTable::Label* label{};
        /// The offset in the generated code where this label is located.
        AbsoluteOffset offset{invalid_offset};
    };

    /// Describes a relocation offset.
    struct FixupEntry
    {
        /// File where the relocation will take place.
        const SymbolTable::File* origin_file{};
        /// The label that `offset` should point to.
        const SymbolTable::Label* label{};
        /// The offset in the generated code that needs relocation.
        AbsoluteOffset offset{invalid_offset};
    };

    /// Describes the location of a file.
    struct FileLoc
    {
        /// The file being described.
        const SymbolTable::File* file{};
        /// The offset in the generated code where this file is located.
        AbsoluteOffset offset{invalid_offset};
    };

    /// Describes a relocatable offset for a file.
    struct FileFixupEntry
    {
        /// The file that `offset` should point to.
        const SymbolTable::File* file{};
        /// The offset in the generated code that needs relocation.
        AbsoluteOffset offset{invalid_offset};
    };

private:
    using LabelDefTable = std::vector<LabelLoc>;
    using LabelFixupTable = std::vector<FixupEntry>;
    using FileDefTable = std::vector<FileLoc>;
    using FileFixupTable = std::vector<FileFixupEntry>;

public:
    using FixupTableView = util::ContainerView<LabelFixupTable>;
    using FileFixupTableView = util::ContainerView<FileFixupTable>;

public:
    /// Constructs an empty table.
    RelocationTable() noexcept = default;

    /// Constructs an empty table with reserved capacity to hold the labels and
    /// files in the given symbol table.
    explicit RelocationTable(const SymbolTable& table_size_hint) noexcept;

    RelocationTable(const RelocationTable&) = delete;
    auto operator=(const RelocationTable&) -> RelocationTable& = delete;

    RelocationTable(RelocationTable&&) noexcept = default;
    auto operator=(RelocationTable&&) noexcept -> RelocationTable& = default;

    ~RelocationTable() noexcept = default;

    /// Registers the location of a label.
    ///
    /// Returns whether insertion took place i.e. `false` if the label has been
    /// already registered in the table.
    auto insert_label_loc(const SymbolTable::Label& label,
                          const SymbolTable::File& origin,
                          AbsoluteOffset offset) -> bool;

    /// Registers the location of a file.
    ///
    /// Returns whether insertion took place i.e. `false` if the file has been
    /// already registered in the table.
    auto insert_file_loc(const SymbolTable::File& file, AbsoluteOffset offset)
            -> bool;

    /// Registers an offset that needs relocation (i.e a label reference).
    ///
    /// This method may invalidate views and iterators to the fixup table.
    ///
    /// \param label the label being referenced in `offset`.
    /// \param origin the file where `offset` is in.
    /// \param offset the offset that needs relocation.
    void insert_fixup_entry(const SymbolTable::Label& label,
                            const SymbolTable::File& origin,
                            AbsoluteOffset offset);

    /// Registers an offset that needs relocation (i.e. a file reference).
    ///
    /// This method may invalidate views and iterators to the fixup table.
    void insert_fixup_entry(const SymbolTable::File& file,
                            AbsoluteOffset offset);

    /// Computes the relocated offset for the given relocation entry.
    ///
    /// Before calling this, all labels and files associated with `entry`
    /// (directly or indirectly) must have had been registered through
    /// `insert_label_loc` or `insert_file_loc`.
    ///
    /// In case relocation isn't possible, `std::nullopt` is returned and
    /// a diagnostic is produced in `diagman`.
    [[nodiscard]] auto relocate(const FixupEntry& entry,
                                DiagnosticHandler& diagman) const
            -> std::optional<RelativeOffset>;

    /// Computes the relocated offset for the given relocation entry.
    ///
    /// Before calling this, the file associated with `entry` must have
    /// had been registered through `insert_file_loc`.
    ///
    /// In case relocation isn't possible, `std::nullopt` is returned and
    /// a diagnostic is produced in `diagman`.
    [[nodiscard]] auto relocate(const FileFixupEntry& entry,
                                DiagnosticHandler& diagman) const
            -> std::optional<RelativeOffset>;

    /// Obtains a view to the fixup table entries.
    [[nodiscard]] auto fixup_table() const noexcept -> FixupTableView
    {
        return FixupTableView(m_label_fixup_table);
    }

    /// Obtains a view to the file fixup table entries.
    [[nodiscard]] auto file_fixup_table() const noexcept -> FileFixupTableView
    {
        return FileFixupTableView(m_file_fixup_table);
    }

private:
    /// Checks whether two files are in the same segment.
    ///
    /// Two files are in the same segment if it's possible to have a
    /// relative offset to the other file from both files (i.e. you can GOTO
    /// from A to B and from B to A).
    [[nodiscard]] static auto
    is_in_same_segment(const SymbolTable::File& filea,
                       const SymbolTable::File& fileb) noexcept -> bool;

    /// Checks whether the given file is in the main segment.
    ///
    /// A file is in the main segment if it's possible to reach the file through
    /// the use of an absolute offset.
    [[nodiscard]] static auto
    is_in_main_segment(const SymbolTable::File& file) noexcept -> bool;

    /// Gets the offset for the start of the segment of the given file.
    [[nodiscard]] auto
    segment_base_for(const SymbolTable::File& file) const noexcept
            -> AbsoluteOffset;

    /// Resizes `label_def_table` such that it can hold the given label.
    void resize_label_def_table(const SymbolTable::Label& label);

    /// Resizes `file_def_table` such that it can hold the given file.
    void resize_file_def_table(const SymbolTable::File& file);

private:
    LabelDefTable m_label_def_table;
    LabelFixupTable m_label_fixup_table;
    FileDefTable m_file_def_table;
    FileFixupTable m_file_fixup_table;
};
} // namespace gta3sc::codegen

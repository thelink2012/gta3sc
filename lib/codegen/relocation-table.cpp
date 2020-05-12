#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/diagnostics.hpp>

namespace gta3sc::codegen
{
RelocationTable::RelocationTable(const SymbolTable& table_size_hint) noexcept
{
    const auto num_labels = table_size_hint.labels().size();
    const auto num_files = table_size_hint.files().size();

    m_label_def_table.reserve(num_labels);
    m_file_def_table.reserve(num_files);

    // Assume at least one reference for each label / file.
    m_label_fixup_table.reserve(num_labels);
    m_file_fixup_table.reserve(num_files);
}

auto RelocationTable::insert_label_loc(const SymbolTable::Label& label,
                                       const SymbolTable::File& origin,
                                       AbsoluteOffset offset) -> bool
{
    resize_label_def_table(label);
    resize_file_def_table(origin);

    if(m_label_def_table[label.id()].offset != invalid_offset)
        return false;

    m_label_def_table[label.id()] = LabelLoc{&origin, &label, offset};
    return true;
}

auto RelocationTable::insert_file_loc(const SymbolTable::File& file,
                                      AbsoluteOffset offset) -> bool
{
    resize_file_def_table(file);

    if(m_file_def_table[file.id()].offset != invalid_offset)
        return false;

    m_file_def_table[file.id()] = FileLoc{&file, offset};
    return true;
}

void RelocationTable::insert_fixup_entry(const SymbolTable::Label& label,
                                         const SymbolTable::File& origin,
                                         AbsoluteOffset offset)
{
    resize_label_def_table(label);
    resize_file_def_table(origin);
    m_label_fixup_table.push_back(FixupEntry{&origin, &label, offset});
}

void RelocationTable::insert_fixup_entry(const SymbolTable::File& file,
                                         AbsoluteOffset offset)
{
    resize_file_def_table(file);
    m_file_fixup_table.push_back(FileFixupEntry{&file, offset});
}

auto RelocationTable::relocate(const FixupEntry& entry,
                               DiagnosticHandler& diagman) const
        -> std::optional<RelativeOffset>
{
    assert(entry.label != nullptr);
    assert(entry.label->id() < m_label_def_table.size());

    const auto& label_def = m_label_def_table[entry.label->id()];
    assert(label_def.offset != invalid_offset);

    assert(label_def.origin_file != nullptr && entry.origin_file != nullptr);
    const auto& label_origin_file = *label_def.origin_file;
    const auto& entry_origin_file = *entry.origin_file;

    if(is_in_main_segment(label_origin_file))
    {
        return label_def.offset;
    }
    else if(is_in_same_segment(label_origin_file, entry_origin_file))
    {
        const auto segbase = segment_base_for(label_origin_file);

        if(segbase == label_def.offset)
        {
            diagman.report(entry.label->source().begin,
                           Diag::codegen_label_at_local_zero_offset)
                    .range(entry.label->source());
            return std::nullopt;
        }

        assert(segbase < label_def.offset);
        return -(label_def.offset - segbase);
    }
    else
    {
        // TODO this should have a front-end checking equivalent 'cause we do
        // not have code locations here
        diagman.report(SourceManager::no_source_loc,
                       Diag::codegen_label_ref_across_segments);
        return std::nullopt;
    }
}

auto RelocationTable::relocate(const FileFixupEntry& entry,
                               [[maybe_unused]] DiagnosticHandler& diagman)
        const -> std::optional<RelativeOffset>
{
    assert(entry.file != nullptr);
    assert(entry.file->id() < m_file_def_table.size());

    const auto& file_def = m_file_def_table[entry.file->id()];
    assert(file_def.offset != invalid_offset);

    return file_def.offset;
}

auto RelocationTable::segment_base_for(
        const SymbolTable::File& file) const noexcept -> AbsoluteOffset
{
    assert(file.id() < m_file_def_table.size());

    const auto& file_def = m_file_def_table[file.id()];
    assert(file_def.offset != invalid_offset);

    switch(file.type())
    {
        case SymbolTable::FileType::main:
        case SymbolTable::FileType::main_extension:
        case SymbolTable::FileType::subscript:
            return 0;
        case SymbolTable::FileType::mission:
            return file_def.offset;
    }
    assert(false);
}

auto RelocationTable::is_in_main_segment(const SymbolTable::File& file) noexcept
        -> bool
{
    switch(file.type())
    {
        case SymbolTable::File::Type::main:
        case SymbolTable::File::Type::main_extension:
        case SymbolTable::File::Type::subscript:
            return true;
        case SymbolTable::File::Type::mission:
            return false;
    }
    assert(false);
}

auto RelocationTable::is_in_same_segment(
        const SymbolTable::File& filea, const SymbolTable::File& fileb) noexcept
        -> bool
{
    switch(filea.type())
    {
        case SymbolTable::File::Type::main:
        case SymbolTable::File::Type::main_extension:
        case SymbolTable::File::Type::subscript:
            return is_in_main_segment(fileb);
        case SymbolTable::File::Type::mission:
            return fileb.type() == SymbolTable::File::Type::mission
                   && fileb.type_id() == filea.type_id();
    }
    assert(false);
}

void RelocationTable::resize_label_def_table(const SymbolTable::Label& label)
{
    if(label.id() >= m_label_def_table.size())
    {
        m_label_def_table.resize(label.id() + 1);
    }
}

void RelocationTable::resize_file_def_table(const SymbolTable::File& file)
{
    if(file.id() >= m_file_def_table.size())
    {
        m_file_def_table.resize(file.id() + 1);
    }
}
} // namespace gta3sc::codegen

// TODO should we optimize the allocations / data structures of this class?
// if we use this in a parallel algorithm with multiple instances a lot of
// memory will be used by the definition tables.

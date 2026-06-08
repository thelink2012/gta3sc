#include "gta3sc/codegen/trilogy/emitter.hpp"
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <optional>
#include <string_view>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto file_label_prefix = "@@"sv;

auto is_mission_file(const gta3sc::SymbolTable::File& file) noexcept -> bool
{
    return file.type() == gta3sc::SymbolTable::FileType::mission;
}
} // namespace

namespace gta3sc::codegen::trilogy::diag
{
const DiagnosticDescriptor
        used_object_name_too_long(DiagnosticSeverity::error,
                                  "Used object name too long", "TODO");
} // namespace gta3sc::codegen::trilogy::diag

namespace gta3sc::codegen::trilogy
{
auto MultifileCodeGen::generate(const LinkedIR<SemaIR>& ir,
                                RelocationTable& reloc_table,
                                std::vector<std::byte>& output) -> bool
{
    assert(current_multifile_offset == 0);

    if(auto header_size = compute_header_stubs(); header_size)
        output.resize(output.size() + *header_size);
    else
        return false;

    auto next_file = generate_first_file(ir, reloc_table, output);
    while(next_file && next_file->file)
    {
        next_file = generate_next_file(*next_file->file, next_file->next_ir,
                                       ir.end(), reloc_table, output);
    }

    if(!next_file)
        return false;

    assert(next_file->file == nullptr);
    assert(next_file->next_ir == ir.end());

    if(!generate_headers(reloc_table, output))
        return false;

    return true;
}

auto MultifileCodeGen::compute_header_stubs()
        -> std::optional<RelocationTable::AbsoluteOffset>
{
    assert(!computed_header_size);

    this->globals_top_index = storage->top_var_index(SymbolTable::global_scope);

    this->num_used_objects = symbol_table->used_objects().size();

    this->num_missions = std::count_if(symbol_table->files().begin(),
                                       symbol_table->files().end(),
                                       is_mission_file);

    this->header_size = global_var_header_size() + used_object_header_size()
                        + mission_header_size();

    this->current_multifile_offset = header_size;
    this->computed_header_size = true;

    return header_size;
}

bool MultifileCodeGen::generate_headers(const RelocationTable& reloc_table,
                                        std::vector<std::byte>& output)
{
    assert(computed_header_size);

    bool has_error = false;
    CodeEmitter emitter(header_size);

    RelocationTable::AbsoluteOffset next_header_offset{0};
    has_error |= !generate_global_var_header(next_header_offset, emitter);
    has_error |= !generate_used_object_header(next_header_offset, emitter);
    has_error |= !generate_mission_header(next_header_offset, emitter,
                                          reloc_table);

    // If not satisfied, there's a bug in the header size calculation
    // compared to its generation counterpart.
    assert(emitter.offset() == header_size);

    if(has_error)
        return false;

    if(output.size() < header_size)
        output.resize(header_size);

    emitter.drain(output.begin());

    return true;
}

auto MultifileCodeGen::global_var_header_size() const -> uint32_t
{
    return globals_top_index * 4;
}

auto MultifileCodeGen::used_object_header_size() const -> uint32_t
{
    return 8 + 4 + (24 * (1 + num_used_objects));
}

auto MultifileCodeGen::mission_header_size() const -> uint32_t
{
    return 8 + 4 + 4 + 2 + 2 + (4 * num_missions);
}

bool MultifileCodeGen::generate_global_var_header(
        RelocationTable::AbsoluteOffset& next_header_offset,
        CodeEmitter& emitter)
{
    const auto fill_size = globals_top_index <= 2 ? 0
                                                  : globals_top_index * 4 - 8;

    next_header_offset += global_var_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0x6D})
            .emit_fill(std::byte{0}, fill_size);
    return true;
}

bool MultifileCodeGen::generate_used_object_header(
        RelocationTable::AbsoluteOffset& next_header_offset,
        CodeEmitter& emitter)
{
    constexpr size_t max_used_object_name_length = 24;
    constexpr size_t max_used_object_name_without_terminator
            = max_used_object_name_length - 1;

    bool has_error = false;

    next_header_offset += used_object_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0})
            .emit_raw_u32(1 + num_used_objects)
            .emit_fill(std::byte{0}, max_used_object_name_length);

    std::vector<const SymbolTable::UsedObject*> used_objects(num_used_objects);
    for(const auto& used_object : symbol_table->used_objects())
        used_objects[used_object.id()] = &used_object;

    for(auto* used_object : used_objects)
    {
        assert(used_object != nullptr);
        auto used_object_name = used_object->name();
        auto used_object_source = used_object->source();

        if(used_object_name.size() > max_used_object_name_without_terminator)
        {
            used_object_name = used_object_name.substr(
                    0, max_used_object_name_without_terminator);

            has_error = true;
            diag->report(used_object_source, diag::used_object_name_too_long);
        }

        emitter.emit_raw_bytes(used_object_name.begin(), used_object_name.end(),
                               max_used_object_name_length);
    }

    return !has_error;
}

bool MultifileCodeGen::generate_mission_header(
        RelocationTable::AbsoluteOffset& next_header_offset,
        CodeEmitter& emitter, const RelocationTable& reloc_table)
{
    next_header_offset += mission_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0})
            .emit_raw_u32(main_segment_size)
            .emit_raw_u32(largest_mission_script_size)
            .emit_raw_u16(num_missions)
            .emit_raw_u16(0); // number of exclusive missions (TODO implement in
                              // the future)

    std::vector<uint32_t> mission_script_offsets(
            num_missions, RelocationTable::invalid_offset);
    for(const auto& file_loc : reloc_table.files())
    {
        if(is_mission_file(*file_loc.file))
            mission_script_offsets[file_loc.file->type_id()] = file_loc.offset;
    }

    for(const auto& offset : mission_script_offsets)
        emitter.emit_raw_u32(offset);

    return true;
}

auto MultifileCodeGen::generate_first_file(
        const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    assert(computed_header_size);
    return generate_next_file(ir.begin(), ir.end(), reloc_table, output);
}

auto MultifileCodeGen::generate_next_file(
        LinkedIR<SemaIR>::const_iterator next_ir,
        LinkedIR<SemaIR>::const_iterator max_ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    assert(computed_header_size);

    if(next_ir == max_ir)
        return NextFile{nullptr, next_ir};

    auto first_file = detect_file_label(*next_ir);
    // Expects next_ir to be pointing to a file label.
    // Failure to do so is a problem on the client of the library, not the
    // code being compiled.
    assert(first_file != nullptr);

    ++next_ir; // skip file label
    return generate_next_file(*first_file, next_ir, max_ir, reloc_table,
                              output);
}

auto MultifileCodeGen::generate_next_file(
        const SymbolTable::File& file, LinkedIR<SemaIR>::const_iterator next_ir,
        LinkedIR<SemaIR>::const_iterator max_ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    assert(computed_header_size);

    bool first_fileoff_insert = reloc_table.insert_file_loc(
            file, current_multifile_offset);

    // Failure to do so is a problem on the client of the library, not the
    // code being compiled. This could imply a file label is appearing twice.
    assert(first_fileoff_insert);

    CodeGen codegen(file, current_multifile_offset, *storage, *diag);

    NextFile result{nullptr, max_ir};
    auto output_iter = std::back_inserter(output);

    const auto offset_before_script = current_multifile_offset;

    for(auto ir = next_ir; ir != max_ir; ++ir)
    {
        if(auto next_file = detect_file_label(*ir); next_file)
        {
            assert(next_file != &file);
            result = NextFile{next_file, std::next(ir)};
            break;
        }

        if(auto result_it = codegen.generate(*ir, reloc_table, output_iter);
           result_it)
            output_iter = *result_it;

        // TODO should we check for overflow? in CodeGen maybe?
        current_multifile_offset = codegen.absolute_offset();
    }

    if(RelocationTable::is_in_main_segment(file))
        main_segment_size = std::max(main_segment_size,
                                     current_multifile_offset);

    if(is_mission_file(file))
        largest_mission_script_size = std::max(largest_mission_script_size,
                                               current_multifile_offset
                                                       - offset_before_script);

    return {result};
}

auto MultifileCodeGen::detect_file_label(const SemaIR& line)
        -> const SymbolTable::File*
{
    if(!line.has_label())
        return nullptr;

    const auto& label = line.label();
    const auto label_name = label.name();
    if(!label_name.starts_with(file_label_prefix))
        return nullptr;

    const auto file_id = label_name.substr(file_label_prefix.size());
    const auto* file = symbol_table->lookup_file(file_id);

    // Expects all file labels to have a corresponding symbol table entry.
    // Failure to do so is a problem on the client of the library, not the
    // code being compiled.
    assert(file != nullptr);

    return file;
}
} // namespace gta3sc::codegen::trilogy
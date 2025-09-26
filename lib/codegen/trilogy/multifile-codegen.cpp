#include "gta3sc/codegen/trilogy/emitter.hpp"
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <print>
#include <string_view>
using namespace std::literals::string_view_literals;

// TODO unit test

namespace
{
constexpr auto file_label_prefix = "@@"sv;
}

namespace gta3sc::codegen::trilogy
{
auto MultifileCodeGen::global_var_header_size() const -> uint32_t
{
    return 8 + num_globals * 4;
}

auto MultifileCodeGen::used_object_header_size() const -> uint32_t
{
    return 8 + (24 * (1 + num_used_objects)) + 4;
}

auto MultifileCodeGen::mission_header_size() const -> uint32_t
{
    return 8 + 4 + 4 + 2 + 2 + (4 * num_missions);
}

bool MultifileCodeGen::generate_header_stubs(std::vector<std::byte>& output)
{
    this->num_globals = symbol_table->scope(SymbolTable::global_scope).size();

    this->num_used_objects = symbol_table->used_objects().size();

    this->num_missions = std::count_if(
            symbol_table->files().begin(), symbol_table->files().end(),
            [](const auto& file) {
                return file.type() == SymbolTable::FileType::mission;
            });

    this->header_size = global_var_header_size() + used_object_header_size()
                        + mission_header_size();
    this->current_multifile_offset = header_size;

    output.clear();
    output.resize(current_multifile_offset);

    return true;
}

bool MultifileCodeGen::generate_headers(const RelocationTable& reloc_table,
                                        std::vector<std::byte>& output)
{
    const auto max_used_object_name_length = 24;

    CodeEmitter emitter(header_size);

    RelocationTable::AbsoluteOffset next_header_offset = 0;

    next_header_offset += global_var_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0})
            .emit_fill(std::byte{0}, num_globals * 4);

    next_header_offset += used_object_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0})
            .emit_raw_u32(1 + num_used_objects)
            .emit_fill(std::byte{0}, max_used_object_name_length);
    // TODO better encapsulate this code
    {
        std::vector<std::string_view> used_object_names;
        used_object_names.resize(num_used_objects);
        for(const auto& used_object : symbol_table->used_objects())
            used_object_names[used_object.id()] = used_object.name();

        for(auto used_object_name : used_object_names)
        {
            if(used_object_name.size() > max_used_object_name_length)
            {
                // TODO diag
                used_object_name = used_object_name.substr(
                        0, max_used_object_name_length);
            }

            emitter.emit_raw_bytes(used_object_name.begin(),
                                   used_object_name.end(),
                                   max_used_object_name_length);
        }
    }

    next_header_offset += mission_header_size();
    emitter.emit_opcode(0x0002)
            .emit_i32(next_header_offset)
            .emit_raw_byte(std::byte{0})
            .emit_raw_u32(0) // TODO main script size
            .emit_raw_u32(0) // TODO max mission script size
            .emit_raw_u16(num_missions)
            .emit_raw_u16(0); // number of exclusive missions (TODO implement in
                              // the future)
    // TODO better encapsulate this code
    {
        std::vector<uint32_t> mission_script_offsets;
        mission_script_offsets.resize(num_missions,
                                      RelocationTable::invalid_offset);
        for(const auto& file_loc : reloc_table.files())
        {
            if(file_loc.file->type() == SymbolTable::FileType::mission)
                mission_script_offsets[file_loc.file->id()] = file_loc.offset;
        }

        for(const auto& mission_script_offset : mission_script_offsets)
        {
            emitter.emit_raw_u32(mission_script_offset);
        }
    }

    if(emitter.offset() != header_size)
    {
        // TODO diag
        return false;
    }

    if(output.size() < header_size)
        output.resize(header_size);

    emitter.drain(output.begin());

    return true;
}

auto MultifileCodeGen::generate_first_file(
        const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    return generate_next_file(ir.begin(), ir.end(), reloc_table, output);
}

auto MultifileCodeGen::generate_next_file(
        LinkedIR<SemaIR>::const_iterator next_ir,
        LinkedIR<SemaIR>::const_iterator max_ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    if(next_ir == max_ir)
        return NextFile{nullptr, next_ir};

    auto first_file = detect_file_label(*next_ir);
    if(!first_file)
    {
        // TODO diag
        return std::nullopt;
    }

    ++next_ir; // skip file label
    return generate_next_file(*first_file, next_ir, max_ir, reloc_table,
                              output);
}

// TODO could I use a OutputIterator?
auto MultifileCodeGen::generate_next_file(
        const SymbolTable::File& file, LinkedIR<SemaIR>::const_iterator next_ir,
        LinkedIR<SemaIR>::const_iterator max_ir, RelocationTable& reloc_table,
        std::vector<std::byte>& output) -> std::optional<NextFile>
{
    if(!reloc_table.insert_file_loc(file, current_multifile_offset))
    {
        // TODO diag -- damn something very wrong here
        return std::nullopt;
    }

    // TODO revisit the Emitter buffer thing. Can we make it bufferless / no
    // allocations?
    CodeGen codegen(file, current_multifile_offset, *storage, *diag);

    auto output_iter = std::back_inserter(output);
    bool has_any_error = false;

    for(auto ir = next_ir; ir != max_ir; ++ir)
    {
        if(auto next_file = detect_file_label(*ir); next_file)
        {
            assert(next_file != &file);
            return NextFile{next_file, std::next(ir)};
        }

        const auto prev_output_size = output.size();

        if(auto result_it = codegen.generate(*ir, reloc_table, output_iter);
           result_it)
            output_iter = *result_it;
        else
            has_any_error = true;

        // TODO should we check for overflow? (in CodeGen maybe?)
        current_multifile_offset = codegen.absolute_offset();
    }

    if(has_any_error)
        return std::nullopt;

    return NextFile{nullptr, max_ir};
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
    if(!file)
    {
        // TODO this is actually a little bit fatal. What do we do? Diag?
        return nullptr;
    }

    return file;
}
} // namespace gta3sc::codegen::trilogy
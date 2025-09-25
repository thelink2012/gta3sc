#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <string_view>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto file_label_prefix = "@@"sv;
}

namespace gta3sc::codegen::trilogy
{

bool MultifileCodeGen::generate(const LinkedIR<SemaIR>& ir,
                                RelocationTable& reloc_table,
                                std::vector<std::byte>& output)
{
    RelocationTable::AbsoluteOffset current_file_offset = 0;
    std::unique_ptr<CodeGen> codegen;

    const auto num_globals
            = symbol_table->scope(SymbolTable::global_scope).size();
    const auto num_used_objects = symbol_table->used_objects().size();

    const auto num_missions = std::count_if(
            symbol_table->files().begin(), symbol_table->files().end(),
            [](const auto& file) {
                return file.type() == SymbolTable::FileType::mission;
            });

    // TODO check if this calculation is really correct (does num_global shift by 2 already?)
    current_file_offset += 8 + (num_globals * 4 - 8) + 8 + 4
                           + (24 * (1 + num_used_objects)) + 8 + 4 + 4 + 2 + 2
                           + (4 * num_missions);
    
    output.clear();
    output.resize(current_file_offset);

    for(const auto& line : ir)
    {
        if(line.has_label())
        {
            const auto& label = line.label();
            const auto label_name = label.name();
            if(label_name.starts_with(file_label_prefix))
            {
                const auto file_id = label_name.substr(
                        file_label_prefix.size());
                const auto* file = symbol_table->lookup_file(file_id);
                if(!file)
                {
                    // TODO diag
                    return false;
                }

                if(!reloc_table.insert_file_loc(*file, current_file_offset))
                {
                    // TODO diag -- damn something very wrong here
                    return false;
                }

                // FIXME this is cloggy
                if(codegen)
                    *codegen = CodeGen(*file, current_file_offset, *storage,
                                       *diag);
                else
                    codegen = std::make_unique<CodeGen>(
                            *file, current_file_offset, *storage, *diag);
            }
        }

        if(!codegen)
        {
            // TODO diag -- damn something very wrong here
            return false;
        }

        const auto prev_output_size = output.size();

        if(!codegen->generate(line, reloc_table, std::back_inserter(output)))
            return false; // TODO maybe can continue?

        // TODO check overflow
        current_file_offset += output.size() - prev_output_size;
    }

    // TODO go back to header to fill it up

    return true;
}
} // namespace gta3sc::codegen::trilogy
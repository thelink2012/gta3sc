#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>

namespace gta3sc::codegen::trilogy
{
constexpr size_t default_emitter_capacity = 256;

auto CodeGen::generate(const SemaIR& ir, RelocationTable& reloc_table) -> bool
{
    emitter.buffer_reserve(default_emitter_capacity);

    if(ir.has_label())
    {
        const auto label_offset = base_offset + emitter.offset();
        const bool inserted = reloc_table.insert_label_loc(ir.label(), *file,
                                                           label_offset);
        assert(inserted);
    }

    if(ir.has_command())
    {
        if(!generate_command(ir.command(), reloc_table))
            return false;
    }

    return true;
}

auto CodeGen::generate_command(const SemaIR::Command& command,
                               RelocationTable& reloc_table) -> bool
{
    const auto& cmd_def = command.def();
    const auto& params = cmd_def.params();

    if(!cmd_def.target_handled() || !cmd_def.target_id())
    {
        diag->report(command.source().begin,
                     Diag::codegen_target_does_not_support_command)
                .range(command.source());
        return false;
    }

    emitter.emit_opcode(*cmd_def.target_id(), command.not_flag());

    auto param_it = params.begin();
    for(const auto& arg : command.args())
    {
        assert(param_it != params.end());

        generate_argument(arg, *param_it, reloc_table);

        if(!param_it->is_optional())
            ++param_it;
    }

    if(cmd_def.has_optional_param())
        emitter.emit_eoal();

    return true;
}

void CodeGen::generate_argument(const SemaIR::Argument& arg,
                                const CommandTable::ParamDef& param,
                                RelocationTable& reloc_table)
{
    switch(arg.type())
    {
        using Type = SemaIR::Argument::Type;
        case Type::INT:
        case Type::CONSTANT:
            generate_int(arg);
            break;
        case Type::FLOAT:
            generate_float(arg);
            break;
        case Type::TEXT_LABEL:
            generate_text_label(arg, param);
            break;
        case Type::STRING:
            generate_string(arg, param);
            break;
        case Type::VARIABLE:
            generate_var_ref(arg);
            break;
        case Type::LABEL:
            generate_label(arg, reloc_table);
            break;
        case Type::FILENAME:
            generate_filename_label(arg, reloc_table);
            break;
        case Type::USED_OBJECT:
            generate_used_object(arg);
            break;
        default:
            assert(false);
            break;
    }
}

void CodeGen::generate_int(const SemaIR::Argument& arg)
{
    assert(arg.type() == SemaIR::Argument::Type::INT
           || arg.type() == SemaIR::Argument::Type::CONSTANT);
    assert(arg.pun_as_int());
    emitter.emit_int(*arg.pun_as_int());
}

void CodeGen::generate_float(const SemaIR::Argument& arg)
{
    assert(arg.type() == SemaIR::Argument::Type::FLOAT);
    emitter.emit_q11_4(*arg.pun_as_float());
}

void CodeGen::generate_used_object(const SemaIR::Argument& arg)
{
    assert(arg.type() == SemaIR::Argument::Type::USED_OBJECT);
    const auto& uobj = *arg.as_used_object();

    const int32_t uobj_seq_id = 1 + uobj.id();
    emitter.emit_int(-uobj_seq_id);
}

void CodeGen::generate_text_label(
        const SemaIR::Argument& arg,
        [[maybe_unused]] const CommandTable::ParamDef& param)
{
    constexpr auto output_size = 8;

    assert(arg.type() == SemaIR::Argument::Type::TEXT_LABEL);
    const std::string_view value = *arg.as_text_label();

    emitter.emit_raw_bytes(value.begin(), value.end(), output_size);
}

void CodeGen::generate_string(
        const SemaIR::Argument& arg,
        [[maybe_unused]] const CommandTable::ParamDef& param)
{
    constexpr auto output_size = 128;

    assert(arg.type() == SemaIR::Argument::Type::STRING);
    const std::string_view value = *arg.as_string();

    emitter.emit_raw_bytes(value.begin(), value.end(), output_size);
}

void CodeGen::generate_var_ref(const SemaIR::Argument& arg)
{
    assert(arg.type() == SemaIR::Argument::Type::VARIABLE);
    const SemaIR::VarRef var_ref = *arg.as_var_ref();
    const SymbolTable::Variable& var_def = var_ref.def();

    // TODO array refs
    assert(!var_ref.has_index());

    // TODO text label var refs
    assert(var_def.type() == SymbolTable::VarType::INT
           || var_def.type() == SymbolTable::VarType::FLOAT);

    if(var_def.scope() == SymbolTable::global_scope)
        emitter.emit_var(4 * storage->var_index(var_def));
    else
        emitter.emit_lvar(storage->var_index(var_def));
}

void CodeGen::generate_label(const SemaIR::Argument& arg,
                             RelocationTable& reloc_table)
{
    assert(arg.type() == SemaIR::Argument::Type::LABEL);
    const SymbolTable::Label& label = *arg.as_label();

    const auto reloc_offset = base_offset + emitter.offset() + 1;
    reloc_table.insert_fixup_entry(label, *file, reloc_offset);

    emitter.emit_i32(0);
}

void CodeGen::generate_filename_label(const SemaIR::Argument& arg,
                                      RelocationTable& reloc_table)
{
    assert(arg.type() == SemaIR::Argument::Type::FILENAME);
    const SymbolTable::File& filename = *arg.as_filename();

    const auto reloc_offset = base_offset + emitter.offset() + 1;
    reloc_table.insert_fixup_entry(filename, reloc_offset);

    emitter.emit_i32(0);
}
} // namespace gta3sc::codegen::trilogy

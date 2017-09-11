#include <stdinc.h>
#include "disassembler.hpp"
#include "program.hpp"
#include "cdimage.hpp"

optional<size_t> Disassembler::data_index(uint32_t local_offset) const
{
    for(size_t i = 0; i < this->decompiled.size(); ++i)
    {
        if(this->decompiled[i].offset == local_offset)
            return i;
    }
    return nullopt;
}

void Disassembler::run_analyzer(size_t from_offset)
{
    while(auto opt_next = this->skip_custom_header(from_offset))
    {
        this->parse_custom_header(from_offset, *opt_next);
        from_offset = *opt_next;
    }

    this->to_explore.emplace(from_offset);
    this->analyze();
}

void Disassembler::analyze()
{
    while(!this->to_explore.empty())
    {
        auto offset = this->to_explore.top();
        this->to_explore.pop();

        this->explore(offset);
    }
}

void Disassembler::explore(size_t offset)
{
    if(offset >= bf.size)
    {
        if(this->type == Type::RecursiveTraversal)
        {
            program.warning(nocontext, "a branch command jumps into the local offset 0x{:X} which is outside the bytecode", offset);
            program.note(nocontext, "use --verbose to find which block this offset belongs to");
        }
        return;
    }

    if(offset_explored[offset])
        return; // already explored

    if(auto opt_cmdid = bf.fetch_u16(offset))
    {
        bool not_flag = (*opt_cmdid & 0x8000) != 0;
        uint16_t pureid = *opt_cmdid & 0x7FFF;

        if(auto opt_cmd = this->command_from_opcode(*opt_cmdid))
        {
            if(explore_opcode(offset, *opt_cmd, not_flag))
                return;

            program.warning(nocontext, "could not disassembly opcode 0x{:X} at local offset 0x{:X}", pureid, offset);
            program.note(nocontext, "use --verbose to find which block this offset belongs to");
        }
        else
        {
            program.warning(nocontext, "found unknown opcode 0x{:X} at local offset 0x{:X}", pureid, offset);
            program.note(nocontext, "use --verbose to find which block this offset belongs to"); 
        }
    }

    // Exploring this byte wasn't quite successful, try the next one.
    this->to_explore.emplace(offset + 1);
}

optional<const Command&> Disassembler::command_from_opcode(uint16_t opcode) const
{
    uint16_t pureid = opcode & 0x7FFF;
    if(pureid < this->oatc_start.value_or(0xFFFF))
    {
        return this->program.commands.find_command(pureid);
    }
    else
    {
        size_t ordinal = pureid - *this->oatc_start;
        if(ordinal < this->oatc_table.size())
        {
            if(auto ptr = this->oatc_table[ordinal])
                return *ptr;
        }
        return nullopt;
    }
}

optional<size_t> Disassembler::skip_custom_header(size_t offset) const
{
    uint8_t head[12+4];

    if(!bf.fetch_bytes(offset, sizeof(head), head))
        return nullopt;

    // custom headers magic number
    if(head[0] != 0x02 || head[1] != 0x00 || head[2] != 0x01 || head[7] != 0xFF
        || head[8] != 0x7F || head[9] != 0xFE || head[10] != 0x00 || head[11] != 0x00)
        return nullopt;

    size_t end_offset = std::abs(bf.fetch_i32(offset + 3).value());
    if(end_offset <= offset)
        return nullopt;

    return end_offset;
}

void Disassembler::parse_custom_header(size_t offset, size_t end_offset)
{
    char fourcc[5]; fourcc[4] = '\0';
    bf.fetch_bytes(offset+12, 4, fourcc).value();

    try
    {
        if(!std::memcmp(fourcc, "OATC", 4))
        {
            auto ordinal_begin = bf.fetch_u16(offset + 16).value();
            auto num_ordinals = bf.fetch_u16(offset + 18).value();

            std::vector<const Command*> oatc_table;
            oatc_table.reserve(num_ordinals);

            size_t table_end   = offset + 20 + 12 * num_ordinals;
            for(size_t curroff = offset + 20; curroff < table_end; curroff += 12)
            {
                optional<string_view> cmdname;
                auto hash = bf.fetch_u32(curroff+4).value();
                auto offz = bf.fetch_u32(curroff+8).value();
                if(offz) cmdname = bf.fetch_zstring(offz + offset + 12).value();

                if(auto opt = this->program.commands.find_command(hash, cmdname))
                    oatc_table.emplace_back(std::addressof(*opt));
                else
                    oatc_table.emplace_back(nullptr);
            }

            this->oatc_start = ordinal_begin;
            this->oatc_table = std::move(oatc_table);
        }
    }
    catch(const bad_optional_access&)
    {
        program.error(nocontext, "corrupted {} header", fourcc);
    }
}

optional<size_t> Disassembler::explore_opcode(size_t op_offset, const Command& command, bool not_flag)
{
    // delay addition of offsets into `this->to_explore`, the opcode may be illformed while we're analyzing it.
    std::stack<size_t> interesting_offsets;

    size_t offset = op_offset + 2;

    bool stop_it = false;
    size_t argument_id = 0;

    auto& commands = this->program.commands;
    bool is_switch_start     = commands.equal(command, commands.switch_start);
    bool is_switch_continued = commands.equal(command, commands.switch_continued);

    auto check_for_imm32 = [&](auto value, const Command::Arg& arg)
    {
        if(is_switch_start && argument_id == 1)
        {
            this->switch_cases_left = value;
        }

        if(arg.type == ArgType::Label)
        {
            if(is_switch_start || is_switch_continued)
            {
                if(this->switch_cases_left == 0)
                    return; // don't take offset
                    
                if(is_switch_start && argument_id != 3) // not default label
                    --this->switch_cases_left;
            }

            interesting_offsets.emplace(value);
        }
    };

    if(is_switch_start)
    {
        // We need this set to 0 since the switch cases argument mayn't
        // be a constant (ill-formed, but game executes).
        this->switch_cases_left = 0;
    }

    // TODO optimize out fetches here to just check offset + N

    try
    {
        for(auto it = command.args.begin();
            !stop_it && it != command.args.end();
            (it->optional? it : ++it), ++argument_id)
        {
            if(it->type == ArgType::TextLabel32)
            {
                if(std::next(it, 1) != command.args.end() && std::next(it, 1)->type == it->type
                && std::next(it, 2) != command.args.end() && std::next(it, 2)->type == it->type
                && std::next(it, 3) != command.args.end() && std::next(it, 3)->type == it->type)
                {
                    bf.fetch_chars(offset, 128).value();
                    offset += 128;
                    it += 3;
                    continue;
                }
                else
                {
                    return nullopt;
                }
            }

            optional<uint8_t> opt_argtype = bf.fetch_u8(offset++);

            if(!opt_argtype)
                return nullopt;

            // Handle III/VC string arguments
            if(*opt_argtype > 0x06 && !this->program.opt.has_text_label_prefix)
            {
                if(it->type == ArgType::TextLabel)
                {
                    offset = offset - 1; // there was no data type, remove one byte
                    bf.fetch_chars(offset, 8).value();
                    offset += 8;
                    continue;
                }
                else if(*opt_argtype == 0x0E 
                        && it->type == ArgType::String 
                        && this->program.opt.cleo)
                {
                    // III/VC CLEO suppots variable length strings
                    // let it pass
                }
                else
                {
                    return nullopt;
                }
            }

            switch(*opt_argtype)
            {
                case 0x00: // EOA (end of args)
                    if(!it->optional)
                        return nullopt;
                    stop_it = true;
                    break;

                case 0x01: // Int32
                {
                    int32_t imm32 = bf.fetch_i32(offset).value();
                    check_for_imm32(imm32, *it);
                    offset += sizeof(int32_t);
                    break;
                }

                case 0x04: // Int8
                {
                    int8_t imm8 = bf.fetch_i8(offset).value();
                    check_for_imm32(imm8, *it);
                    offset += sizeof(int8_t);
                    break;
                }

                case 0x05: // Int16
                {
                    int16_t imm16 = bf.fetch_i16(offset).value();
                    check_for_imm32(imm16, *it);
                    offset += sizeof(int16_t);
                    break;
                }

                case 0x02: // Global Int/Float Var
                case 0x03: // Local Int/Float Var
                case 0x0A: // Global TextLabel Var (SA)
                case 0x0B: // Local TextLabel Var (SA)
                case 0x10: // Global TextLabel16 Var (SA)
                case 0x11: // Local TextLabel16 Var (SA)
                    bf.fetch_u16(offset).value();
                    offset += sizeof(uint16_t);
                    break;

                case 0x07: // Global Int/Float Array (SA)
                case 0x08: // Local Int/Float Array (SA)
                case 0x0C: // Global TextLabel Array (SA)
                case 0x0D: // Local TextLabel Array (SA)
                case 0x12: // Global TextLabel16 Array (SA)
                case 0x13: // Local TextLabel16 Array (SA)
                {
                    bf.fetch_u32(offset+0).value();  // u16 + i16
                    bf.fetch_u16(offset+4).value(); // u8 + u8
                    offset += 6;
                    break;
                }

                case 0x06: // Float
                    if(this->program.opt.use_half_float)
                    {
                        bf.fetch_i16(offset).value();
                        offset += sizeof(int16_t);
                    }
                    else
                    {
                        bf.fetch_u32(offset).value();
                        offset += sizeof(uint32_t);
                    }
                    break;

                case 0x09: // Immediate 8-byte string (SA)
                    bf.fetch_chars(offset, 8).value();
                    offset += 8;
                    break;

                case 0x0F: // Immediate 16-byte string (SA)
                    bf.fetch_chars(offset, 16).value();
                    offset += 16;
                    break;

                case 0x0E: // Immediate variable-length string (SA)
                {
                    uint8_t count = bf.fetch_u8(offset).value();
                    bf.fetch_chars(offset+1, count).value();
                    offset += (count+1);
                    break;
                }

                default:
                    return nullopt;
            }
        }
    }
    catch(const bad_optional_access&)
    {
        // opcode is incorrect or broken
        return nullopt;
    }

    // OK, opcode is not ill formed, we can push up the new offsets to explore
    while(!interesting_offsets.empty())
    {
        int32_t label_param = interesting_offsets.top();
        interesting_offsets.pop();

        if(label_param >= 0)
        {
            main_asm.label_offsets.emplace(label_param);

            if(main_asm.type == Type::RecursiveTraversal)
                main_asm.to_explore.emplace(label_param);
        }
        else
        {
            this->label_offsets.emplace(-label_param);

            if(this->type == Type::RecursiveTraversal)
                this->to_explore.emplace(-label_param);
        }
    }

    if(this->type == Type::LinearSweep)
    {
        // Add next instruction to be explored.
        this->to_explore.emplace(offset);
    }
    else if(this->type == Type::RecursiveTraversal)
    {
        // add next instruction as the next thing to be explored, if this isn't a instruction that
        // terminates execution or jumps unconditionally to another offset.
        // TODO would be nice if this was actually configurable.
        if(!commands.equal(command, commands.goto_)
        && !commands.equal(command, commands.return_)
        && !commands.equal(command, commands.cleo_return)
        && !commands.equal(command, commands.terminate_this_script)
        && !commands.equal(command, commands.terminate_this_custom_script))
        {
            if((is_switch_start || is_switch_continued) && this->switch_cases_left == 0)
            {
                // we are at the last SWITCH_START/SWITCH_CONTINUED command, after this, the game will take a branch.
            }
            else
            {
                this->to_explore.emplace(offset);
            }
        }
    }

    // mark this area as explored
    for(size_t i = op_offset; i < offset; ++i)
        this->offset_explored[i] = true;

    ++this->hint_num_ops;

    return offset - op_offset;
}

// this function should not really fail.
// before it runs, `explore_opcode` ran, meaning everything is alright.
DecompiledData Disassembler::opcode_to_data(size_t& offset)
{
    bool stop_it = false;

    auto cmdid     = *bf.fetch_u16(offset);
    bool not_flag  = (cmdid & 0x8000) != 0;

    const Command& command = *this->command_from_opcode(cmdid);

    DecompiledCommand ccmd { (cmdid & 0x8000) != 0, command };

    auto start_offset = offset;
    offset = offset + 2;

    // Helper functor to fetch array data.
    auto parse_array = [this, &ccmd](size_t offset, bool is_global, VarType type)
    {
        auto var_offset = *bf.fetch_u16(offset+0);
        auto index_var  = *bf.fetch_u16(offset+2);
        auto array_size = *bf.fetch_u8(offset+4);
        auto array_prop = *bf.fetch_u8(offset+5);

        auto elem_type  = (array_prop & 0x7F) == 0? DecompiledVarArray::ElemType::Int :
                            (array_prop & 0x7F) == 1? DecompiledVarArray::ElemType::Float :
                            (array_prop & 0x7F) == 2? DecompiledVarArray::ElemType::TextLabel :
                            (array_prop & 0x7F) == 3? DecompiledVarArray::ElemType::TextLabel16 :
                                                    DecompiledVarArray::ElemType::None;
        bool is_index_global = (array_prop & 0x80) != 0;

        ccmd.args.emplace_back(DecompiledVarArray{
            DecompiledVar{ is_global, type, uint32_t(var_offset) * (is_global? 1 : 4) },
            DecompiledVar{ is_index_global, VarType::Int, uint32_t(index_var) * (is_index_global? 1 : 4) },
            array_size,
            elem_type,
        });

        return offset + 6;
    };

    for(auto it = command.args.begin();
        !stop_it && it != command.args.end();
        it->optional? it : ++it)
    {
        if(it->type == ArgType::TextLabel32)
        {
            assert(std::next(it,1)->type == it->type);
            assert(std::next(it,2)->type == it->type);
            assert(std::next(it,3)->type == it->type);
            
            ccmd.args.emplace_back(DecompiledString{ DecompiledString::Type::String128, std::move(*bf.fetch_chars(offset, 128)) });

            it += 3;
            offset += 128;
            continue;
        }

        auto datatype = *bf.fetch_u8(offset++);

        // Handle III/VC string arguments
        if(datatype > 0x06 && !this->program.opt.has_text_label_prefix)
        {
            if(it->type == ArgType::TextLabel)
            {
                offset = offset - 1; // there was no data type, remove one byte
                ccmd.args.emplace_back(DecompiledString { DecompiledString::Type::TextLabel8, std::move(*bf.fetch_chars(offset, 8)) });
                offset += 8;
                continue;
            }

            // Code was already analyzed by explore_opcode and it went fine.
            // meaning either this is unreachable, or this is a variable
            // length string with cleo support enabled. Let it run.
        }

        switch(datatype)
        {
            case 0x00:
                ccmd.args.emplace_back(EOAL{});
                stop_it = true;
                break;

            case 0x01: // Int32
            {
                auto i32 = *bf.fetch_i32(offset);
                offset += sizeof(int32_t);
                ccmd.args.emplace_back(i32);
                break;
            }

            case 0x04: // Int8
            {
                auto i8 = *bf.fetch_i8(offset);
                offset += sizeof(int8_t);
                ccmd.args.emplace_back(i8);
                break;
            }

            case 0x05: // Int16
            {
                auto i16 = *bf.fetch_i16(offset);
                offset += sizeof(int16_t);
                ccmd.args.emplace_back(i16);
                break;
            }

            case 0x02: // Global Int/Float Var
                ccmd.args.emplace_back(DecompiledVar{ true, VarType::Int, *bf.fetch_u16(offset) });
                offset += sizeof(uint16_t);
                break;
            case 0x0A: // Global TextLabel Var (SA)
                ccmd.args.emplace_back(DecompiledVar{ true, VarType::TextLabel, *bf.fetch_u16(offset) });
                offset += sizeof(uint16_t);
                break;
            case 0x10: // Global TextLabel16 Var (SA)
                ccmd.args.emplace_back(DecompiledVar { true, VarType::TextLabel16, *bf.fetch_u16(offset) });
                offset += sizeof(uint16_t);
                break;

            case 0x03: // Local Int/Float Var
                ccmd.args.emplace_back(DecompiledVar{ false, VarType::Int, *bf.fetch_u16(offset) * 4u });
                offset += sizeof(uint16_t);
                break;
            case 0x0B: // Local TextLabel Var (SA)
                ccmd.args.emplace_back(DecompiledVar{ false, VarType::TextLabel, *bf.fetch_u16(offset) * 4u });
                offset += sizeof(uint16_t);
                break;
            case 0x11: // Local TextLabel16 Var (SA)
                ccmd.args.emplace_back(DecompiledVar { false, VarType::TextLabel16, *bf.fetch_u16(offset) * 4u });
                offset += sizeof(uint16_t);
                break;

            case 0x07: // Global Int/Float Array (SA)
                offset = parse_array(offset, true, VarType::Int);
                break;
            case 0x0C: // Global TextLabel Array (SA)
                offset = parse_array(offset, true, VarType::TextLabel);
                break;
            case 0x12: // Global TextLabel16 Array (SA)
                offset = parse_array(offset, true, VarType::TextLabel16);
                break;

            case 0x08: // Local Int/Float Array (SA)
                offset = parse_array(offset, false, VarType::Int);
                break;
            case 0x0D: // Local TextLabel Array (SA)
                offset = parse_array(offset, false, VarType::TextLabel);
                break;
            case 0x13: // Local TextLabel16 Array (SA)
                offset = parse_array(offset, false, VarType::TextLabel16);
                break;

            case 0x06: // Float
                if(this->program.opt.use_half_float)
                {
                    ccmd.args.emplace_back(*bf.fetch_i16(offset) / 16.0f);
                    offset += sizeof(int16_t);
                }
                else
                {
                    static_assert(std::numeric_limits<float>::is_iec559
                        && sizeof(float) == sizeof(uint32_t), "IEEE 754 floating point expected.");

                    ccmd.args.emplace_back(reinterpret_cast<const float&>(*bf.fetch_u32(offset)));
                    offset += sizeof(uint32_t);
                }
                break;

            case 0x09: // Immediate 8-byte string (SA)
                ccmd.args.emplace_back(DecompiledString{ DecompiledString::Type::TextLabel8, std::move(*bf.fetch_chars(offset, 8)) });
                offset += 8;
                break;

            case 0x0F: // Immediate 16-byte string (SA)
                ccmd.args.emplace_back(DecompiledString{ DecompiledString::Type::TextLabel16, std::move(*bf.fetch_chars(offset, 16)) });
                offset += 16;
                break;

            case 0x0E: // Immediate variable-length string (SA)
            {
                auto count = *bf.fetch_u8(offset);
                ccmd.args.emplace_back(DecompiledString{ DecompiledString::Type::StringVar, std::move(*bf.fetch_chars(offset+1, count)) });
                offset += count + 1;
                break;
            }

            default:
                Unreachable();
        }
    }

    return DecompiledData(start_offset, std::move(ccmd));
}

void Disassembler::disassembly(size_t from_offset)
{
    std::vector<DecompiledData>& output = this->decompiled;

    output.reserve(this->hint_num_ops + 16); // +16 for unknown/hex areas

    while(auto opt_next = this->skip_custom_header(from_offset))
        from_offset = *opt_next;

    for(size_t offset = from_offset; offset < bf.size; )
    {
        if(this->label_offsets.count(offset))
        {
            output.emplace_back(DecompiledLabelDef{ offset });
        }

        if(this->offset_explored[offset])
        {
            output.emplace_back(opcode_to_data(offset));
            // offset was received by ref and mutated ^
        }
        else
        {
            auto begin_offset = offset++;
            for(; offset < bf.size; ++offset)
            {
                // repeat this loop until a label offset or a explored offset is found, then break.
                //
                // if a label offset is found, it'll be added at the beggining of the outer for loop,
                // and then (maybe) this loop will continue.

                if(this->offset_explored[offset] || this->label_offsets.count(offset))
                    break;
            }

            output.emplace_back(begin_offset, std::vector<uint8_t>(bf.bytes + begin_offset, bf.bytes + offset));
        }
    }
}

optional<DecompiledScmHeader> DecompiledScmHeader::from_bytecode(const void* bytecode, size_t bytecode_size, Version version)
{
    assert(version == DecompiledScmHeader::Version::Liberty
        || version == DecompiledScmHeader::Version::Miami
        || version == DecompiledScmHeader::Version::SanAndreas);

    bool is_sa = (version == DecompiledScmHeader::Version::SanAndreas);

    try
    {
        BinaryFetcher bf { bytecode, bytecode_size };

        auto seg1_offset = 0u;
        auto seg2_offset = bf.fetch_u32(seg1_offset + 3).value();
        auto seg3_offset = bf.fetch_u32(seg2_offset + 3).value();
        auto seg4_offset = bf.fetch_u32(seg3_offset + 3).value();
        auto code_offset = seg4_offset;

        uint32_t size_globals = seg2_offset;

        std::vector<std::string> models;
        size_t num_models = static_cast<size_t>((std::max)(1u, bf.fetch_u32(seg2_offset + 8 + 0).value()) - 1);
        models.reserve(num_models);
        for(size_t i = 0; i < num_models; ++i)
        {
            char buffer[32];
            bf.fetch_chars(seg2_offset + 8 + 4 + 24 + (24 * i), 24, buffer).value();
            models.emplace_back(buffer);
        }

        auto main_size    = bf.fetch_u32(seg3_offset + 8 + 0).value();
        auto num_missions = bf.fetch_u16(seg3_offset + 8 + 8).value();
        size_t incr       = is_sa? 4 : 0; // increment for u32 (num vars used in mission)

        std::vector<uint32_t> mission_offsets;
        for(size_t i = 0; i < num_missions; ++i)
        {
            mission_offsets.emplace_back(bf.fetch_u32(seg3_offset + 8 + 8 + 4 + incr + (4*i)).value());
        }

        std::vector<StreamedScript> streamed_scripts;
        if(is_sa)
        {
            auto seg5_offset = bf.fetch_u32(seg4_offset + 3).value();
            auto seg6_offset = bf.fetch_u32(seg5_offset + 3).value();
            auto seg7_offset = bf.fetch_u32(seg6_offset + 3).value();
            code_offset = seg7_offset;

            auto num_scripts = bf.fetch_u32(seg4_offset + 8 + 4).value();
            for(size_t i = 0; i < num_scripts; ++i)
            {
                char buffer[24];
                auto name = bf.fetch_chars(seg4_offset + 8 + 4 + 4 + (28 * i), 20, buffer).value();
                auto size = bf.fetch_u32(seg4_offset + 8 + 4 + 4 + (28 * i) + 20 + 4).value();
                streamed_scripts.emplace_back(StreamedScript { buffer, size });
            }
        }

        return DecompiledScmHeader {
            version, code_offset, size_globals,
            std::move(models), main_size, std::move(mission_offsets), std::move(streamed_scripts)
        };
    }
    catch(const bad_optional_access&)
    {
        // the header is incorrect or broken
        return nullopt;
    }
}

auto mission_scripts_fetcher(const void* bytecode_, size_t bytecode_size, const DecompiledScmHeader& header, ProgramContext& program)
    -> std::vector<BinaryFetcher>
{
    const uint8_t* bytecode = reinterpret_cast<const uint8_t*>(bytecode_);

    std::vector<BinaryFetcher> mission_segments;
    mission_segments.reserve(header.mission_offsets.size());

    std::vector<uint32_t> mission_offsets_sorted = header.mission_offsets;
    std::sort(mission_offsets_sorted.begin(), mission_offsets_sorted.end());

    for(size_t i = 0; i < header.mission_offsets.size(); ++i)
    {
        size_t mission_offset = header.mission_offsets[i];

        if(mission_offset < header.main_size)
            program.fatal_error(nocontext, "corrupted scm header");

        auto it = std::lower_bound(mission_offsets_sorted.begin(), mission_offsets_sorted.end(), mission_offset);
        if(it == mission_offsets_sorted.end() || *it != mission_offset)
            program.fatal_error(nocontext, "corrupted scm header");

        size_t next_mission_offset  = it+1 != mission_offsets_sorted.end()? *(it+1) : bytecode_size;

        if(next_mission_offset > bytecode_size)
            program.fatal_error(nocontext, "corrupted scm header");

        mission_segments.emplace_back(BinaryFetcher { (bytecode + mission_offset), (next_mission_offset - mission_offset) });
    }

    return mission_segments;
}

auto streamed_scripts_fetcher(const void* img_bytes_, size_t img_size, const DecompiledScmHeader& header, ProgramContext& program)
    -> std::vector<BinaryFetcher>
{
    const uint8_t* img_bytes = reinterpret_cast<const uint8_t*>(img_bytes_);

    std::vector<BinaryFetcher> scripts;

    if(img_size < sizeof(CdHeader))
        program.fatal_error(nocontext, "corrupted img file");

    auto& cdheader = reinterpret_cast<const CdHeader&>(*img_bytes);
    if(memcmp(cdheader.magic, "VER2", 4) != 0 || img_size < cdheader.num_entries * sizeof(CdEntry))
        program.fatal_error(nocontext, "corrupted img file");

    auto entries = reinterpret_cast<const CdEntry*>(img_bytes + sizeof(CdHeader));
    auto entries_end = entries + cdheader.num_entries;

    for(auto& script_pair : header.streamed_scripts)
    {
        auto filename = script_pair.name + ".scm";

        auto it = std::find_if(entries, entries_end, [&](const CdEntry& entry) {
            // TODO entry.filenames mayn't have a null terminator!
            return iequal_to()(filename, entry.filename);
        });

        if(it != entries_end)
        {
            auto offset = size_t(it->offset) * 2048;
            auto bsize  = size_t(it->streaming_size) * 2048; // <- this size contains useless 00s
            auto size   = script_pair.size;                  // <- but this size doesn't

            if(offset + bsize > img_size)
                program.error(nocontext, "entry for '{}' in img file is is sparse", filename);
            else if(offset + size > img_size)
                program.error(nocontext, "size for entry '{}' in img file does not match the size in the scm header", filename);
            else
                scripts.emplace_back(BinaryFetcher { img_bytes + offset, size });
        }
        else
        {
            program.error(nocontext, "streamed script '{}' not found in img file", filename);
        }
    }

    return scripts;
}

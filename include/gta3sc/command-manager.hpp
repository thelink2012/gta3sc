#pragma once
#include <gta3sc/arena-allocator.hpp>
#include <gta3sc/adt/span.hpp>
#include <string_view>
#include <unordered_map>

namespace gta3sc
{
class CommandManager
{
public:
    enum class ParamType : uint8_t
    {
        INT,
        FLOAT,

        VAR_INT,
        LVAR_INT,
        VAR_FLOAT,
        LVAR_FLOAT,
        VAR_TEXT_LABEL,
        LVAR_TEXT_LABEL,

        INPUT_INT,
        INPUT_FLOAT,
        OUTPUT_INT,
        OUTPUT_FLOAT,
        LABEL,
        TEXT_LABEL,
        STRING,
        
        VAR_INT_OPT,
        LVAR_INT_OPT,
        VAR_FLOAT_OPT,
        LVAR_FLOAT_OPT,
        VAR_TEXT_LABEL_OPT,
        LVAR_TEXT_LABEL_OPT,
        INPUT_OPT,
    };

    struct ParamDef
    {
        ParamType type;
        uint16_t entity_type = 0;
        uint16_t enum_type = 0;

        bool is_optional() const
        {
            switch(type)
            {
                case ParamType::VAR_INT_OPT:
                case ParamType::LVAR_INT_OPT:
                case ParamType::VAR_FLOAT_OPT:
                case ParamType::LVAR_FLOAT_OPT:
                case ParamType::VAR_TEXT_LABEL_OPT:
                case ParamType::LVAR_TEXT_LABEL_OPT:
                case ParamType::INPUT_OPT:
                    return true;
                default:
                    return false;
            }
        }
    };

    struct CommandDef
    {
        std::string_view name;
        adt::span<ParamDef> params;
    };

public:

    bool add_command(std::string_view name,
                     std::initializer_list<ParamDef> params)
    {
        if(command_by_name.count(name))
            return false;

        const auto num_params = params.size();

        auto a_name = allocate_string_upper(name);

        auto a_params = static_cast<ParamDef*>(
                arena.allocate(num_params * sizeof(ParamDef), 
                               alignof(ParamDef)));
        std::uninitialized_copy(params.begin(), params.end(), a_params);

        auto a_cmd = new (arena, alignof(CommandDef)) CommandDef { 
            a_name, 
            adt::span(a_params, num_params),
        };

        command_by_name.emplace(a_name, a_cmd);
        return true;
    }

    auto find_command(std::string_view name) const -> const CommandDef*
    {
        auto it = command_by_name.find(name);
        return it == command_by_name.end()? nullptr : it->second;
    }


private:

private:
    auto allocate_string_upper(std::string_view from)
        -> std::string_view
    {
        auto chars = allocate_string(from, arena);
        for(auto& c : chars)
        {
            if(c >= 'a' && c <= 'z')
                const_cast<char&>(c) = c - 32;
        }
        return chars;
    }

private:
    std::unordered_map<std::string_view, arena_ptr<const CommandDef>> 
        command_by_name;

    ArenaMemoryResource arena;

};
}

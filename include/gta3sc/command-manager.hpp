#pragma once
#include <gta3sc/arena-allocator.hpp>
#include <gta3sc/adt/span.hpp>
#include <string_view>
#include <unordered_map>
#include <algorithm>

namespace gta3sc
{
class CommandManager
{
public:
    enum class EnumId : uint16_t
    {
    };

    enum class EntityId : uint16_t
    {
    };

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
        EntityId entity_type {0};
        EnumId enum_type {0};

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
    
    struct AlternatorDef
    {
        adt::span<const CommandDef*> alternatives;
    };

    struct ConstantDef
    {
        EnumId enum_id;
        int32_t value;
        arena_ptr<ConstantDef> next;
    };

public:
    static constexpr EnumId global_enum = EnumId{0};
    static constexpr EntityId no_entity = EntityId{0};

    CommandManager()
    {
        const auto global_enum = add_enumeration("GLOBAL");
        assert(global_enum == this->global_enum);
    }

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

    bool add_alternator(std::string_view name,
                        std::initializer_list<const CommandDef*> alternatives)
    {
        assert(std::none_of(alternatives.begin(), alternatives.end(),
                            [](const auto* a) { return a == nullptr; }));

        if(alternator_by_name.count(name))
            return false;

        const auto num_alters = alternatives.size();

        auto a_name = allocate_string_upper(name);
        auto a_alters = static_cast<const CommandDef**>(
                arena.allocate(num_alters * sizeof(const CommandDef*),
                               alignof(const CommandDef*)));

        std::uninitialized_copy(alternatives.begin(), alternatives.end(), a_alters);

        auto a_alternator = new (arena, alignof(AlternatorDef)) AlternatorDef {
            adt::span(a_alters, num_alters)
        };

        alternator_by_name.emplace(a_name, a_alternator);
        return true;
    }

    auto find_alternator(std::string_view name) const -> const AlternatorDef*
    {
        auto it = alternator_by_name.find(name);
        return it == alternator_by_name.end()? nullptr : it->second;
    }

    auto add_enumeration(std::string_view name) -> EnumId
    {
        const std::underlying_type_t<EnumId> next_id = enum_by_name.size();
        assert(next_id < std::numeric_limits<decltype(next_id)>::max());
        auto [iter, inserted] = enum_by_name.try_emplace(name, EnumId{next_id});
        return iter->second;
    }

    auto find_enumeration(std::string_view name) const -> std::optional<EnumId>
    {
        auto it = enum_by_name.find(name);
        if(it == enum_by_name.end())
            return std::nullopt;
        return it->second;
    }

    bool add_constant(EnumId enum_id, std::string_view name, int32_t value)
    {
        auto [it, _] = constant_by_name.try_emplace(name, nullptr);
        
        ConstantDef** ptr_next = std::addressof(it->second);
        while(*ptr_next != nullptr)
        {
            if((*ptr_next)->enum_id == enum_id)
                return false;

            ptr_next = std::addressof((*ptr_next)->next);
        }

        *ptr_next = new (arena, alignof(ConstantDef)) ConstantDef { 
            enum_id, value, nullptr 
        };

        return true;
    }

    auto find_constant(EnumId enum_id, std::string_view name) const -> const ConstantDef*
    {
        auto it = constant_by_name.find(name);
        if(it == constant_by_name.end())
            return nullptr;

        for(ConstantDef* cdef = it->second; cdef != nullptr; cdef = cdef->next)
        {
            if(cdef->enum_id == enum_id)
                return cdef;
        }

        return nullptr;
    }

    auto find_constant_any_means(std::string_view name) const -> const ConstantDef*
    {
        auto it = constant_by_name.find(name);
        if(it == constant_by_name.end())
            return nullptr;
        assert(it->second != nullptr);

        for(ConstantDef* cdef = it->second; cdef != nullptr; cdef = cdef->next)
        {
            if(cdef->enum_id != global_enum)
                return cdef;
        }

        return nullptr;
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

    std::unordered_map<std::string_view, arena_ptr<const AlternatorDef>>
        alternator_by_name;

    std::unordered_map<std::string_view, EnumId>
        enum_by_name;

    std::unordered_map<std::string_view, arena_ptr<ConstantDef>>
        constant_by_name;

    ArenaMemoryResource arena;

};
}

#include <gta3sc/command-manager.hpp>

// FIXME none of the insertion commands here handle the case of the string
// key being lowercase, which shouldn't reinsert the command.

namespace gta3sc
{
// Iterator for the list of string constants stored in the constants map.
using ConstantListIterator
        = util::ConstIntrusiveListForwardIterator<CommandManager::ConstantDef>;

// Mutable iterator for the list of string constants stored in the constants
// map.
using MutableConstantListIterator
        = util::IntrusiveListForwardIterator<CommandManager::ConstantDef>;

CommandManager::CommandManager(Builder&& builder) :
    commands_map(std::move(builder.commands_map)),
    alternators_map(std::move(builder.alternators_map)),
    enums_map(std::move(builder.enums_map)),
    constants_map(std::move(builder.constants_map)),
    entities_map(std::move(builder.entities_map))
{}

bool CommandManager::ParamDef::is_optional() const
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

auto CommandManager::AlternatorDef::begin() const -> const_iterator
{
    return const_iterator(first, nullptr);
}

auto CommandManager::AlternatorDef::end() const -> const_iterator
{
    return const_iterator();
}

auto CommandManager::find_command(std::string_view name) const
        -> const CommandDef*
{
    return find_command(commands_map, name);
}

auto CommandManager::find_alternator(std::string_view name) const
        -> const AlternatorDef*
{
    return find_alternator(alternators_map, name);
}

auto CommandManager::find_enumeration(std::string_view name) const
        -> std::optional<EnumId>
{
    return find_enumeration(enums_map, name);
}

auto CommandManager::find_constant(EnumId enum_id, std::string_view name) const
        -> const ConstantDef*
{
    return find_constant(constants_map, enum_id, name);
}

auto CommandManager::find_constant_any_means(std::string_view name) const
        -> const ConstantDef*
{
    return find_constant_any_means(constants_map, name);
}

auto CommandManager::find_entity_type(std::string_view name) const
        -> std::optional<EntityId>
{
    return find_entity_type(entities_map, name);
}

auto CommandManager::find_command(const CommandsMap& commands_map,
                                  std::string_view name) -> const CommandDef*
{
    auto it = commands_map.find(name);
    return it == commands_map.end() ? nullptr : it->second;
}

auto CommandManager::find_alternator(const AlternatorsMap& alternators_map,
                                     std::string_view name)
        -> const AlternatorDef*
{
    auto it = alternators_map.find(name);
    return it == alternators_map.end() ? nullptr : it->second;
}

auto CommandManager::find_enumeration(const EnumsMap& enums_map,
                                      std::string_view name)
        -> std::optional<EnumId>
{
    if(auto it = enums_map.find(name); it != enums_map.end())
        return it->second;
    return std::nullopt;
}

auto CommandManager::find_constant(const ConstantsMap& constants_map,
                                   EnumId enum_id, std::string_view name)
        -> const ConstantDef*
{
    const auto cpair = constants_map.find(name);
    if(cpair == constants_map.end())
        return nullptr;

    assert(cpair->second != nullptr);

    for(ConstantListIterator it(cpair->second, nullptr), end; it != end; ++it)
    {
        if(it->enum_id == enum_id)
            return std::addressof(*it);
    }

    return nullptr;
}

auto CommandManager::find_constant_any_means(const ConstantsMap& constants_map,
                                             std::string_view name)
        -> const ConstantDef*
{
    const auto cdef = constants_map.find(name);
    if(cdef == constants_map.end())
        return nullptr;

    assert(cdef->second != nullptr);

    for(ConstantListIterator it(cdef->second, nullptr), end; it != end; ++it)
    {
        if(it->enum_id != global_enum)
            return std::addressof(*it);
    }

    return nullptr;
}

auto CommandManager::find_entity_type(const EntitiesMap& entities_map,
                                      std::string_view name)
        -> std::optional<EntityId>
{
    if(auto it = entities_map.find(name); it != entities_map.end())
        return it->second;
    return std::nullopt;
}

CommandManager::Builder::Builder(ArenaMemoryResource& arena) : arena(&arena)
{
    const auto global_enum = insert_enumeration("GLOBAL").first;
    const auto no_entity_type = insert_entity_type("NONE").first;
    assert(global_enum == CommandManager::global_enum);
    assert(no_entity_type == CommandManager::no_entity_type);
}

auto CommandManager::Builder::build() && -> CommandManager
{
    return CommandManager(std::move(*this));
}

auto CommandManager::Builder::find_command(std::string_view name) const
        -> const CommandDef*
{
    return CommandManager::find_command(commands_map, name);
}

auto CommandManager::Builder::find_command(std::string_view name) -> CommandDef*
{
    return const_cast<CommandDef*>(
            const_cast<const Builder&>(*this).find_command(name));
}

auto CommandManager::Builder::find_alternator(std::string_view name) const
        -> const AlternatorDef*
{
    return CommandManager::find_alternator(alternators_map, name);
}

auto CommandManager::Builder::find_alternator(std::string_view name)
        -> AlternatorDef*
{
    return const_cast<AlternatorDef*>(
            const_cast<const Builder&>(*this).find_alternator(name));
}

auto CommandManager::Builder::find_enumeration(std::string_view name) const
        -> std::optional<EnumId>
{
    return CommandManager::find_enumeration(enums_map, name);
}

auto CommandManager::Builder::find_constant(EnumId enum_id,
                                            std::string_view name) const
        -> const ConstantDef*
{
    return CommandManager::find_constant(constants_map, enum_id, name);
}

auto CommandManager::Builder::find_entity_type(std::string_view name) const
        -> std::optional<EntityId>
{
    return CommandManager::find_entity_type(entities_map, name);
}

auto CommandManager::Builder::insert_command(std::string_view name)
        -> std::pair<CommandDef*, bool>
{
    if(auto it = commands_map.find(name); it != commands_map.end())
        return {it->second, false};

    auto a_name = util::allocate_string_upper(name, *arena);
    auto* a_cmd = new(*arena, alignof(CommandDef)) CommandDef{a_name};

    auto [it, inserted] = commands_map.emplace(a_name, a_cmd);
    assert(inserted);
    return {it->second, true};
}

auto CommandManager::Builder::insert_alternator(std::string_view name)
        -> std::pair<AlternatorDef*, bool>
{
    if(auto it = alternators_map.find(name); it != alternators_map.end())
        return {it->second, false};

    auto a_name = util::allocate_string_upper(name, *arena);
    auto* a_alternator = new(*arena, alignof(AlternatorDef)) AlternatorDef{};

    auto [it, inserted] = alternators_map.emplace(a_name, a_alternator);
    assert(inserted);
    return {it->second, true};
}

auto CommandManager::Builder::insert_alternative(AlternatorDef& alternator,
                                                 const CommandDef& command)
        -> const AlternativeDef*
{
    auto* a_alternative = new(*arena, alignof(AlternativeDef))
            AlternativeDef(command);

    if(alternator.last)
    {
        alternator.last->next = a_alternative;
    }
    else
    {
        assert(!alternator.first);
        alternator.first = a_alternative;
    }

    alternator.last = a_alternative;
    return a_alternative;
}

auto CommandManager::Builder::insert_enumeration(std::string_view name)
        -> std::pair<EnumId, bool>
{
    if(auto it = enums_map.find(name); it != enums_map.end())
        return {it->second, false};

    auto a_name = util::allocate_string_upper(name, *arena);

    const auto next_id = static_cast<std::underlying_type_t<EnumId>>(
            enums_map.size());
    assert(next_id < std::numeric_limits<decltype(next_id)>::max());

    auto [it, inserted] = enums_map.emplace(a_name, EnumId{next_id});
    assert(inserted);
    return {it->second, true};
}

auto CommandManager::Builder::insert_or_assign_constant(EnumId enum_id,
                                                        std::string_view name,
                                                        int32_t value)
        -> std::pair<const ConstantDef*, bool>
{
    auto it = constants_map.find(name);
    if(it == constants_map.end())
    {
        auto a_name = util::allocate_string_upper(name, *arena);
        auto* cdef = new(*arena, alignof(ConstantDef))
                ConstantDef{enum_id, value};
        auto [_, inserted] = constants_map.emplace(a_name, cdef);
        assert(inserted);
        return {cdef, true};
    }
    else
    {
        MutableConstantListIterator prev_it{}, curr_it(it->second, nullptr);
        for(MutableConstantListIterator end{}; curr_it != end;
            prev_it = curr_it++)
        {
            if(curr_it->enum_id == enum_id)
            {
                curr_it->value = value;
                return {std::addressof(*curr_it), false};
            }
        }

        prev_it->next = new(*arena, alignof(ConstantDef))
                ConstantDef{enum_id, value};
        return {prev_it->next, true};
    }
}

auto CommandManager::Builder::insert_entity_type(std::string_view name)
        -> std::pair<EntityId, bool>
{
    if(auto it = entities_map.find(name); it != entities_map.end())
        return {it->second, false};

    auto a_name = util::allocate_string_upper(name, *arena);

    const auto next_id = static_cast<std::underlying_type_t<EntityId>>(
            entities_map.size());
    assert(next_id < std::numeric_limits<decltype(next_id)>::max());

    auto [it, inserted] = entities_map.emplace(a_name, EntityId{next_id});
    assert(inserted);
    return {it->second, true};
}
}

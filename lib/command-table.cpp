#include <gta3sc/command-table.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/memory.hpp>
#include <limits>

// FIXME none of the insertion commands here handle the case of the string
// key being lowercase, which shouldn't reinsert the command.

namespace linear_list = gta3sc::util::algorithm::linear_list;

namespace gta3sc
{
CommandTable::CommandTable(CommandMap&& commands_map,
                           AlternatorMap&& alternators_map, EnumMap&& enums_map,
                           ConstantMap&& constants_map,
                           EntityMap&& entities_map) noexcept :
    commands_map(std::move(commands_map)),
    alternators_map(std::move(alternators_map)),
    enums_map(std::move(enums_map)),
    constants_map(std::move(constants_map)),
    entities_map(std::move(entities_map))
{}

auto CommandTable::ParamDef::is_optional() const noexcept -> bool
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
        case ParamType::INT:
        case ParamType::FLOAT:
        case ParamType::VAR_INT:
        case ParamType::LVAR_INT:
        case ParamType::VAR_FLOAT:
        case ParamType::LVAR_FLOAT:
        case ParamType::VAR_TEXT_LABEL:
        case ParamType::LVAR_TEXT_LABEL:
        case ParamType::INPUT_INT:
        case ParamType::INPUT_FLOAT:
        case ParamType::OUTPUT_INT:
        case ParamType::OUTPUT_FLOAT:
        case ParamType::LABEL:
        case ParamType::TEXT_LABEL:
        case ParamType::STRING:
            return false;
    }
    assert(false);
}

auto CommandTable::AlternatorDef::begin() const noexcept -> const_iterator
{
    return const_iterator(first, nullptr);
}

auto CommandTable::AlternatorDef::end() const noexcept -> const_iterator
{
    return const_iterator();
}

auto CommandTable::find_command(std::string_view name) const noexcept
        -> const CommandDef*
{
    return find_command(commands_map, name);
}

auto CommandTable::find_alternator(std::string_view name) const noexcept
        -> const AlternatorDef*
{
    return find_alternator(alternators_map, name);
}

auto CommandTable::find_enumeration(std::string_view name) const noexcept
        -> std::optional<EnumId>
{
    return find_enumeration(enums_map, name);
}

auto CommandTable::find_constant(EnumId enum_id,
                                 std::string_view name) const noexcept
        -> const ConstantDef*
{
    return find_constant(constants_map, enum_id, name);
}

auto CommandTable::find_constant_any_means(std::string_view name) const noexcept
        -> const ConstantDef*
{
    return find_constant_any_means(constants_map, name);
}

auto CommandTable::find_entity_type(std::string_view name) const noexcept
        -> std::optional<EntityId>
{
    return find_entity_type(entities_map, name);
}

auto CommandTable::find_command(const CommandMap& commands_map,
                                std::string_view name) noexcept
        -> const CommandDef*
{
    const auto it = commands_map.find(name);
    return it == commands_map.end() ? nullptr : it->second;
}

auto CommandTable::find_alternator(const AlternatorMap& alternators_map,
                                   std::string_view name) noexcept
        -> const AlternatorDef*
{
    const auto it = alternators_map.find(name);
    return it == alternators_map.end() ? nullptr : it->second;
}

auto CommandTable::find_enumeration(const EnumMap& enums_map,
                                    std::string_view name) noexcept
        -> std::optional<EnumId>
{
    if(const auto it = enums_map.find(name); it != enums_map.end())
        return it->second;

    return std::nullopt;
}

auto CommandTable::find_constant(const ConstantMap& constants_map,
                                 EnumId enum_id, std::string_view name) noexcept
        -> const ConstantDef*
{
    const auto cpair = constants_map.find(name);
    if(cpair == constants_map.end())
        return nullptr;

    assert(cpair->second != nullptr);

    for(ConstantDef::ConstIterator it(*cpair->second), end; it != end; ++it)
    {
        if(it->enum_id() == enum_id)
            return std::addressof(*it);
    }

    return nullptr;
}

auto CommandTable::find_constant_any_means(const ConstantMap& constants_map,
                                           std::string_view name) noexcept
        -> const ConstantDef*
{
    const auto cdef = constants_map.find(name);
    if(cdef == constants_map.end())
        return nullptr;

    assert(cdef->second != nullptr);

    for(ConstantDef::ConstIterator it(*cdef->second), end; it != end; ++it)
    {
        if(it->enum_id() != global_enum)
            return std::addressof(*it);
    }

    return nullptr;
}

auto CommandTable::find_entity_type(const EntityMap& entities_map,
                                    std::string_view name) noexcept
        -> std::optional<EntityId>
{
    if(auto it = entities_map.find(name); it != entities_map.end())
        return it->second;
    return std::nullopt;
}

auto CommandTable::Builder::build() && -> CommandTable
{
    return CommandTable(std::move(commands_map), std::move(alternators_map),
                        std::move(enums_map), std::move(constants_map),
                        std::move(entities_map));
}

auto CommandTable::Builder::find_command(std::string_view name) const noexcept
        -> const CommandDef*
{
    return CommandTable::find_command(commands_map, name);
}

auto CommandTable::Builder::find_command(std::string_view name) noexcept
        -> CommandDef*
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Safe.
    return const_cast<CommandDef*>( // NOLINTNEXTLINE
            const_cast<const Builder&>(*this).find_command(name));
}

auto CommandTable::Builder::find_alternator(
        std::string_view name) const noexcept -> const AlternatorDef*
{
    return CommandTable::find_alternator(alternators_map, name);
}

auto CommandTable::Builder::find_alternator(std::string_view name) noexcept
        -> AlternatorDef*
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): Safe.
    return const_cast<AlternatorDef*>( // NOLINTNEXTLINE
            const_cast<const Builder&>(*this).find_alternator(name));
}

auto CommandTable::Builder::find_enumeration(
        std::string_view name) const noexcept -> std::optional<EnumId>
{
    return CommandTable::find_enumeration(enums_map, name);
}

auto CommandTable::Builder::find_constant(EnumId enum_id,
                                          std::string_view name) const noexcept
        -> const ConstantDef*
{
    return CommandTable::find_constant(constants_map, enum_id, name);
}

auto CommandTable::Builder::find_entity_type(
        std::string_view name) const noexcept -> std::optional<EntityId>
{
    return CommandTable::find_entity_type(entities_map, name);
}

auto CommandTable::Builder::insert_command(std::string_view name)
        -> std::pair<CommandDef*, bool>
{
    if(auto it = commands_map.find(name); it != commands_map.end())
        return {it->second, false};

    auto a_name = util::new_string(name, allocator, util::toupper);
    auto* a_cmd = allocator.new_object<CommandDef>(private_tag, a_name);

    auto [it, inserted] = commands_map.emplace(a_name, a_cmd);
    assert(inserted);
    return {it->second, true};
}

void CommandTable::Builder::set_command_id(CommandDef& command,
                                           std::optional<int16_t> target_id,
                                           bool target_handled)
{
    assert(!target_id.has_value() || *target_id >= 0);
    command.m_target_id = target_id;
    command.m_target_handled = target_handled;
}

auto CommandTable::Builder::insert_alternator(std::string_view name)
        -> std::pair<AlternatorDef*, bool>
{
    if(auto it = alternators_map.find(name); it != alternators_map.end())
        return {it->second, false};

    auto a_name = util::new_string(name, allocator, util::toupper);
    auto* a_alternator = allocator.new_object<AlternatorDef>(private_tag);

    auto [it, inserted] = alternators_map.emplace(a_name, a_alternator);
    assert(inserted);
    return {it->second, true};
}

auto CommandTable::Builder::insert_alternative(AlternatorDef& alternator,
                                               const CommandDef& command)
        -> const AlternativeDef*
{
    auto* a_alternative = allocator.new_object<AlternativeDef>(private_tag,
                                                               command);

    std::tie(alternator.first, alternator.last) = linear_list::push_back(
            *a_alternative, alternator.first, alternator.last);

    return a_alternative;
}

auto CommandTable::Builder::insert_enumeration(std::string_view name)
        -> std::pair<EnumId, bool>
{
    if(auto it = enums_map.find(name); it != enums_map.end())
        return {it->second, false};

    auto a_name = util::new_string(name, allocator, util::toupper);

    const auto next_id = static_cast<std::underlying_type_t<EnumId>>(
            enums_map.size() + 1);
    assert(next_id < std::numeric_limits<decltype(next_id)>::max());
    assert(EnumId{next_id} != global_enum);

    auto [it, inserted] = enums_map.emplace(a_name, EnumId{next_id});
    assert(inserted);
    return {it->second, true};
}

auto CommandTable::Builder::insert_or_assign_constant(EnumId enum_id,
                                                      std::string_view name,
                                                      int32_t value)
        -> std::pair<const ConstantDef*, bool>
{
    auto it = constants_map.find(name);
    if(it == constants_map.end())
    {
        auto a_name = util::new_string(name, allocator, util::toupper);
        auto* cdef = allocator.new_object<ConstantDef>(private_tag, enum_id,
                                                       value);
        auto [_, inserted] = constants_map.emplace(a_name, cdef);
        assert(inserted);
        return {cdef, true};
    }
    else
    {
        ConstantDef::Iterator prev_it{};
        ConstantDef::Iterator curr_it(it->second, nullptr);

        for(ConstantDef::Iterator end{}; curr_it != end; prev_it = curr_it++)
        {
            if(curr_it->enum_id() == enum_id)
            {
                curr_it->m_value = value;
                return {std::addressof(*curr_it), false};
            }
        }

        auto* a_constant = allocator.new_object<ConstantDef>(private_tag,
                                                             enum_id, value);

        linear_list::insert_after(*prev_it, *a_constant);

        return {a_constant, true};
    }
}

auto CommandTable::Builder::insert_entity_type(std::string_view name)
        -> std::pair<EntityId, bool>
{
    if(auto it = entities_map.find(name); it != entities_map.end())
        return {it->second, false};

    auto a_name = util::new_string(name, allocator, util::toupper);

    const auto next_id = static_cast<std::underlying_type_t<EntityId>>(
            entities_map.size() + 1);
    assert(next_id < std::numeric_limits<decltype(next_id)>::max());
    assert(EntityId{next_id} != no_entity_type);

    auto [it, inserted] = entities_map.emplace(a_name, EntityId{next_id});
    assert(inserted);
    return {it->second, true};
}
} // namespace gta3sc

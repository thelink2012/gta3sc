#pragma once
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/intrusive-forward-list-node.hpp>
#include <gta3sc/util/span.hpp>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace gta3sc
{
/// A table for storing command definitions.
///
/// This is a immutable data structure which holds information about commands,
/// alternators, enumerations, constants and entity types.
class CommandTable
{
private:
    /// This tag is used to make construction of the table elements private.
    struct PrivateTag
    {
    };
    static constexpr PrivateTag private_tag{};

public:
    class CommandDef;
    class AlternatorDef;
    class AlternativeDef;
    class ConstantDef;
    class Builder;

    struct ParamDef;

    /// Uniquely identifies an enumeration.
    ///
    /// See the language specification for details on enumerations:
    /// https://gtamodding.github.io/gta3script-specs/core.html#string-constants
    enum class EnumId : uint16_t
    { // strong typedef
    };

    /// Uniquely identifies an entity type.
    ///
    /// See the language specification for details on entities:
    /// https://gtamodding.github.io/gta3script-specs/core.html#entities
    enum class EntityId : uint16_t
    { // strong typedef
    };

    /// Type of a command parameter.
    ///
    /// See the language specification for details on parameters:
    /// https://gtamodding.github.io/gta3script-specs/core.html#parameters
    enum class ParamType : uint8_t
    {          // GTA3script types should be uppercase.
        INT,   // NOLINT(readability-identifier-naming)
        FLOAT, // NOLINT(readability-identifier-naming)

        VAR_INT,         // NOLINT(readability-identifier-naming)
        LVAR_INT,        // NOLINT(readability-identifier-naming)
        VAR_FLOAT,       // NOLINT(readability-identifier-naming)
        LVAR_FLOAT,      // NOLINT(readability-identifier-naming)
        VAR_TEXT_LABEL,  // NOLINT(readability-identifier-naming)
        LVAR_TEXT_LABEL, // NOLINT(readability-identifier-naming)

        INPUT_INT,    // NOLINT(readability-identifier-naming)
        INPUT_FLOAT,  // NOLINT(readability-identifier-naming)
        OUTPUT_INT,   // NOLINT(readability-identifier-naming)
        OUTPUT_FLOAT, // NOLINT(readability-identifier-naming)
        LABEL,        // NOLINT(readability-identifier-naming)
        TEXT_LABEL,   // NOLINT(readability-identifier-naming)
        STRING,       // NOLINT(readability-identifier-naming)

        VAR_INT_OPT,         // NOLINT(readability-identifier-naming)
        LVAR_INT_OPT,        // NOLINT(readability-identifier-naming)
        VAR_FLOAT_OPT,       // NOLINT(readability-identifier-naming)
        LVAR_FLOAT_OPT,      // NOLINT(readability-identifier-naming)
        VAR_TEXT_LABEL_OPT,  // NOLINT(readability-identifier-naming)
        LVAR_TEXT_LABEL_OPT, // NOLINT(readability-identifier-naming)
        INPUT_OPT,           // NOLINT(readability-identifier-naming)
    };

    /// Uniquely identifies the global string constant enumeration.
    ///
    /// The global enumeration is guarated to be the zero-initialized `EnumId`.
    static constexpr EnumId global_enum = EnumId{0};

    /// Uniquely identifies the none entity type.
    ///
    /// This entity type is used when a parameter (or variable) isn't
    /// associated with any entity.
    ///
    /// The none entity type is guaranted to be the zero-initialized `EntityId`.
    static constexpr EntityId no_entity_type = EntityId{0};

public:
    /// Constructs an empty command manager.
    ///
    /// Use `Builder` to construct a non-empty command manager.
    CommandTable() noexcept = default;

    CommandTable(const CommandTable&) = delete;
    auto operator=(const CommandTable&) -> CommandTable& = delete;

    CommandTable(CommandTable&&) noexcept = default;
    auto operator=(CommandTable&&) noexcept -> CommandTable& = default;

    ~CommandTable() noexcept = default;

    /// Finds the command with the specified name.
    ///
    /// The given name must be in uppercase or no command will be found.
    ///
    /// Returns a pointer to the command information or `nullptr` if not found.
    auto find_command(std::string_view name) const noexcept
            -> const CommandDef*;

    /// Finds the alternator with the specified name.
    ///
    /// The given name must be in uppercase or no alternator will be found.
    ///
    /// Returns a pointer to the alternator information or `nullptr` if not
    /// found.
    auto find_alternator(std::string_view name) const noexcept
            -> const AlternatorDef*;

    /// Finds the enumeration with the specified name.
    ///
    /// The given name must be in uppercase or no enumeration will be found.
    ///
    /// Returns the enumeration identifier or `std::nullopt` if not found.
    auto find_enumeration(std::string_view name) const noexcept
            -> std::optional<EnumId>;

    /// Finds a string constant of certain name in a given enumeration.
    ///
    /// The given name must be in uppercase or no string constant will be found.
    ///
    /// Returns the string constant information or `nullptr` if not found.
    auto find_constant(EnumId enum_id, std::string_view name) const noexcept
            -> const ConstantDef*;

    /// Finds a string constant of certain name in any enumeration (except the
    /// global enumeration).
    ///
    /// The given name must be in uppercase or no string constant will be found.
    ///
    /// If multiple string constants exist with the given name, the first one
    /// to be inserted into the table (during the building process)
    /// takes precedence over the latter ones.
    ///
    /// Returns the string constant information or `nullptr` if not found.
    auto find_constant_any_means(std::string_view name) const noexcept
            -> const ConstantDef*;

    /// Finds the entity type with the specified name in the table.
    ///
    /// The given name must be in uppercase or no entity will be found.
    ///
    /// Returns the entity type identifier or `std::nullopt` if not found.
    auto find_entity_type(std::string_view name) const noexcept
            -> std::optional<EntityId>;

private:
    /// Provides a mapping from a name to a command definition.
    using CommandMap
            = std::unordered_map<std::string_view, ArenaPtr<CommandDef>>;

    /// Provides a mapping from a name to a alternator definition.
    using AlternatorMap
            = std::unordered_map<std::string_view, ArenaPtr<AlternatorDef>>;

    /// Provides a mapping from a name to an enumeration identifier.
    using EnumMap = std::unordered_map<std::string_view, EnumId>;

    /// Provides a mapping from a name to a list of constant definitions.
    using ConstantMap
            = std::unordered_map<std::string_view, ArenaPtr<ConstantDef>>;

    /// Provides a mapping from a name to an entity identifier.
    using EntityMap = std::unordered_map<std::string_view, EntityId>;

    /// Static version of `this->find_command(name)`.
    static auto find_command(const CommandMap& commands_map,
                             std::string_view name) noexcept
            -> const CommandDef*;

    /// Static version of `this->find_alternator(name)`.
    static auto find_alternator(const AlternatorMap& alternators_map,
                                std::string_view name) noexcept
            -> const AlternatorDef*;

    /// Static version of `this->find_enumeration(name)`.
    static auto find_enumeration(const EnumMap& enums_map,
                                 std::string_view name) noexcept
            -> std::optional<EnumId>;

    /// Static version of `this->find_constant(enum_id, name)`.
    static auto find_constant(const ConstantMap& constants_map, EnumId enum_id,
                              std::string_view name) noexcept
            -> const ConstantDef*;

    /// Static version of `this->find_constant_any_means(name)`.
    static auto find_constant_any_means(const ConstantMap& constants_map,
                                        std::string_view name) noexcept
            -> const ConstantDef*;

    /// Static version of `this->find_entity_type(name)`.
    static auto find_entity_type(const EntityMap& entities_map,
                                 std::string_view name) noexcept
            -> std::optional<EntityId>;

private:
    CommandMap commands_map;
    AlternatorMap alternators_map;
    EnumMap enums_map;
    ConstantMap constants_map;
    EntityMap entities_map;

    CommandTable(CommandMap&& commands_map, AlternatorMap&& alternators_map,
                 EnumMap&& enums_map, ConstantMap&& constants_map,
                 EntityMap&& entities_map) noexcept;
};

/// Stores information about a parameter.
///
/// See the language specification for details on parameters:
/// https://gtamodding.github.io/gta3script-specs/core.html#parameters
struct CommandTable::ParamDef
{
    ParamDef() noexcept = default;

    explicit ParamDef(ParamType type) noexcept : type(type) {}

    explicit ParamDef(ParamType type, EntityId entity_type,
                      EnumId enum_type) noexcept :
        type(type), entity_type(entity_type), enum_type(enum_type)
    {}

    /// Checks whether this is an optional parameter.
    [[nodiscard]] auto is_optional() const noexcept -> bool;

    /// The typing of the parameter.
    ParamType type{ParamType::INPUT_INT};

    /// The entity type associated with the parameter.
    EntityId entity_type{0};

    /// The enumeration associated with the parameter.
    EnumId enum_type{0};
};

/// Stores information about a command.
class CommandTable::CommandDef : public ArenaObj
{
public:
    CommandDef(PrivateTag /*unused*/, std::string_view name) noexcept :
        m_name(name)
    {}

    /// Returns the name of the command.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
    }

    /// Returns the parameters of the command.
    [[nodiscard]] auto params() const noexcept -> util::span<const ParamDef>
    {
        return m_params;
    }

    /// Returns the i-th parameter of the command.
    [[nodiscard]] auto param(size_t i) const noexcept -> const ParamDef&
    {
        return m_params[i];
    }

    /// Returns the number of parameters (including optional parameters) of this
    /// command.
    [[nodiscard]] auto num_params() const noexcept -> size_t
    {
        return m_params.size();
    }

    /// Returns the number of non-optional parameters of this command.
    [[nodiscard]] auto num_min_params() const noexcept -> size_t
    {
        return num_params() - has_optional_param();
    }

    /// Checks whether the last parameter of the command is optional.
    [[nodiscard]] auto has_optional_param() const noexcept -> bool
    {
        return !m_params.empty() ? m_params.back().is_optional() : false;
    }

    /// Returns the opcode associated with the command in the target script
    /// engine.
    [[nodiscard]] auto target_id() const noexcept -> std::optional<int16_t>
    {
        return m_target_id;
    }

    /// Returns whether this command is handled by the target script engine.
    [[nodiscard]] auto target_handled() const noexcept -> bool
    {
        return m_target_handled;
    }

private:
    friend class CommandTable::Builder;
    std::string_view m_name;
    util::span<const ParamDef> m_params;
    std::optional<int16_t> m_target_id;
    bool m_target_handled{};
};

/// Stores information about an alternative command.
///
/// See `AlternatorDef` for details.
class CommandTable::AlternativeDef
    : public util::IntrusiveForwardListNode<AlternativeDef>
    , public ArenaObj
{
public:
    AlternativeDef(PrivateTag /*unused*/, const CommandDef& command) noexcept :
        m_command(&command)
    {}

    /// Returns the command associated with this alternative.
    [[nodiscard]] auto command() const noexcept -> const CommandDef&
    {
        return *m_command;
    }

private:
    const CommandDef* m_command;
};

/// Stores information about an alternator.
///
/// See the language specification for details on alternators:
/// https://gtamodding.github.io/gta3script-specs/core.html#alternators
class CommandTable::AlternatorDef : public ArenaObj
{
public:
    using iterator = typename AlternativeDef::ConstIterator;
    using const_iterator = iterator;
    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using const_reference = reference;
    using difference_type = typename iterator::difference_type;
    using size_type = size_t;

    explicit AlternatorDef(PrivateTag /*unused*/) {}

    /// Returns the front iterator for the alternatives in this alternator.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;

    /// Returns the past-the-end iterator for the alternatives in this
    /// alternator.
    [[nodiscard]] auto end() const noexcept -> const_iterator;

private:
    friend class CommandTable::Builder;
    ArenaPtr<AlternativeDef> first{};
    ArenaPtr<AlternativeDef> last{};
};

/// Stores information about a string constant.
///
/// See the language specification for details on string constants.
/// https://gtamodding.github.io/gta3script-specs/core.html#string-constants
class CommandTable::ConstantDef
    : public util::IntrusiveForwardListNode<ConstantDef>
    , public ArenaObj
{
public:
    ConstantDef(PrivateTag /*unused*/, EnumId enum_id, int32_t value) noexcept :
        m_enum_id(enum_id), m_value(value)
    {}

    /// Returns the enumeration this string constant is part of.
    [[nodiscard]] auto enum_id() const noexcept -> EnumId { return m_enum_id; }

    /// Returns the integer value of this string constant.
    [[nodiscard]] auto value() const noexcept -> int32_t { return m_value; }

private:
    friend class CommandTable::Builder;
    EnumId m_enum_id;
    int32_t m_value;
};

/// This is a builder capable of constructing a command table.
///
/// Since `CommandTable` is immutable this builder should be used to
/// prepare the command definitions before passing it onto the table.
class CommandTable::Builder
{
public:
    /// Constructs a builder.
    ///
    /// Allocations of command definitions, alternators, constants, and whatnot
    /// will be perfomed in the given arena. Therefore, the arena should be live
    /// as long as the resulting `CommandTable` is alive.
    explicit Builder(ArenaAllocator<> allocator) noexcept : allocator(allocator)
    {}

    Builder(const Builder&) = delete;
    auto operator=(const Builder&) -> Builder& = delete;

    Builder(Builder&&) noexcept = default;
    auto operator=(Builder&&) noexcept -> Builder& = default;

    ~Builder() noexcept = default;

    /// Builds the command table.
    auto build() && -> CommandTable;

    /// Behaves the same as `CommandTable::find_command`.
    auto find_command(std::string_view name) const noexcept
            -> const CommandDef*;

    /// Behaves the same as `CommandTable::find_command`.
    auto find_command(std::string_view name) noexcept -> CommandDef*;

    /// Behaves the same as `CommandTable::find_alternator`.
    auto find_alternator(std::string_view name) const noexcept
            -> const AlternatorDef*;

    /// Behaves the same as `CommandTable::find_alternator`.
    auto find_alternator(std::string_view name) noexcept -> AlternatorDef*;

    /// Behaves the same as `CommandTable::find_enumeration`.
    auto find_enumeration(std::string_view name) const noexcept
            -> std::optional<EnumId>;

    /// Behaves the same as `CommandTable::find_constant`.
    auto find_constant(EnumId enum_id, std::string_view name) const noexcept
            -> const ConstantDef*;

    /// Behaves the same as `CommandTable::find_entity_type`.
    auto find_entity_type(std::string_view name) const noexcept
            -> std::optional<EntityId>;

    /// Inserts a command with the given name into the table.
    ///
    /// The given name is internally converted to uppercase. If a command of
    /// same name already exists, does nothing.
    ///
    /// Returns the command and whether insertion took place.
    auto insert_command(std::string_view name) -> std::pair<CommandDef*, bool>;

    /// Sets the parameters of a command to the given parameters.
    ///
    /// If parameters are already associated with the command, replaces them
    /// with the given ones.
    ///
    /// \param command the command to set the parameters.
    /// \param params_begin the beggining iterator for the parameters.
    /// \params params_end the past-the-end iterator for the parameters.
    /// \params params_size the size of the `[params_begin, params_end)`
    /// sequence.
    template<typename ForwardIterator>
    void set_command_params(CommandDef& command, ForwardIterator params_begin,
                            ForwardIterator params_end, size_t params_size);

    /// Behaves the same as `set_command_params(command, params_begin,
    /// params_end, distance(params_begin, params_end))`.
    template<typename RandomAccessIterator>
    void set_command_params(CommandDef& command,
                            RandomAccessIterator params_begin,
                            RandomAccessIterator params_end);

    /// Sets the command id for a command and whether it is handled or
    /// not by the target platform.
    void set_command_id(CommandDef& command, std::optional<int16_t> target_id,
                        bool target_handled);

    /// Inserts an alternator with the given name into the table.
    ///
    /// The given name is internally converted to uppercase. If an alternator
    /// of same name already exists, does nothing.
    ///
    /// Returns the alternator and whether insertion took place.
    auto insert_alternator(std::string_view name)
            -> std::pair<AlternatorDef*, bool>;

    /// Inserts a command alternative into a given alternator.
    ///
    /// The behaviour of the method is left unspecified if the given command
    /// is already present in the alternator.
    ///
    /// Returns the produced alternative definition.
    auto insert_alternative(AlternatorDef& alternator,
                            const CommandDef& command) -> const AlternativeDef*;

    /// Inserts an enumeration with the given name into the table.
    ///
    /// The given name is internally converted to uppercase. If an enumeration
    /// of same name already exists, does nothing.
    ///
    /// Returns the enumeration identifier and whether insertion took place.
    auto insert_enumeration(std::string_view name) -> std::pair<EnumId, bool>;

    /// Inserts a string constant into a given enumeration.
    ///
    /// The given constant name is internally converted to uppercase. If a
    /// string constant of same name already exists in the enumeration,
    /// replaces its value with the new `value` and behaves as if no insertion
    /// took place.
    ///
    /// If a constant of same name already exists in another enumeration,
    /// the previous constant value takes precedence during a call to
    /// `CommandTable::find_constant_any_means`. See its documentation
    /// for further details.
    ///
    /// Returns the string constant information and whether insertion took
    /// place.
    auto insert_or_assign_constant(EnumId enum_id, std::string_view name,
                                   int32_t value)
            -> std::pair<const ConstantDef*, bool>;

    /// Inserts an entity type with the given name into the table.
    ///
    /// The given name is internally converted to uppercase. If an entity type
    /// of same name already exists, does nothing.
    ///
    /// Returns the entity identifier and whether insertion took place.
    auto insert_entity_type(std::string_view name) -> std::pair<EntityId, bool>;

private:
    ArenaAllocator<> allocator;
    CommandMap commands_map;
    AlternatorMap alternators_map;
    EnumMap enums_map;
    ConstantMap constants_map;
    EntityMap entities_map;
};

template<typename ForwardIterator>
void CommandTable::Builder::set_command_params(CommandDef& command,
                                               ForwardIterator params_begin,
                                               ForwardIterator params_end,
                                               size_t params_size)
{
    auto* a_params = allocator.allocate_object<ParamDef>(params_size);
    std::uninitialized_copy(params_begin, params_end, a_params);
    command.m_params = util::span(a_params, params_size);
}

template<typename RandomAccessIterator>
void CommandTable::Builder::set_command_params(
        CommandDef& command, RandomAccessIterator params_begin,
        RandomAccessIterator params_end)
{
    const size_t params_size = params_end - params_begin;
    return set_command_params(command, params_begin, params_end, params_size);
}

// Keep the size of these structures under control.
static_assert(sizeof(CommandTable::ParamDef) <= 8);
static_assert(sizeof(CommandTable::ConstantDef) <= 16);
static_assert(sizeof(CommandTable::AlternativeDef) <= 16);

// Those structures will be stored in an arena thus they need to be trivially
// destructible.
static_assert(std::is_trivially_destructible_v<CommandTable::ParamDef>);
static_assert(std::is_trivially_destructible_v<CommandTable::CommandDef>);
static_assert(std::is_trivially_destructible_v<CommandTable::AlternatorDef>);
static_assert(std::is_trivially_destructible_v<CommandTable::AlternativeDef>);
static_assert(std::is_trivially_destructible_v<CommandTable::ConstantDef>);
} // namespace gta3sc

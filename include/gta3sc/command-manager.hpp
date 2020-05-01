#pragma once
#include <algorithm>
#include <gta3sc/util/arena-allocator.hpp>
#include <gta3sc/util/intrusive-list-node.hpp>
#include <gta3sc/util/span.hpp>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace gta3sc
{
/// A repository for commands.
///
/// This is a immutable data structure which holds information about commands,
/// alternators, enumerations, constants and entity types.
class CommandManager
{
public:
    class Builder;

    struct ParamDef;
    struct CommandDef;
    struct AlternatorDef;
    struct AlternativeDef;
    struct ConstantDef;

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

    /// Stores information about a parameter.
    ///
    /// See the language specification for details on parameters:
    /// https://gtamodding.github.io/gta3script-specs/core.html#parameters
    struct ParamDef
    {
        ParamType type;
        EntityId entity_type{0};
        EnumId enum_type{0};

        explicit ParamDef(ParamType type) : type(type) {}

        explicit ParamDef(ParamType type, EntityId entity_type,
                          EnumId enum_type) :
            type(type), entity_type(entity_type), enum_type(enum_type)
        {}

        /// Checks whether this is an optional parameter.
        bool is_optional() const;
    };

    /// Stores information about a command.
    struct CommandDef
    {
        /// The name of the command.
        std::string_view name;
        /// The parameters of the command.
        util::span<ParamDef> params;
        /// The opcode associated with the command in the target script engine.
        std::optional<uint16_t> target_id;
        /// Whether this command is handled by the target script engine.
        bool target_handled = false;
    };

    /// Stores information about a alternator.
    ///
    /// See the language specification for details on alternators:
    /// https://gtamodding.github.io/gta3script-specs/core.html#alternators
    struct AlternatorDef
    {
    public:
        /// An iterator that iterates on alternative commands.
        using const_iterator
                = util::ConstIntrusiveListForwardIterator<AlternativeDef>;

        /// Returns the front iterator for the alternatives in this alternator.
        auto begin() const -> const_iterator;

        /// Returns the past-the-end iterator for the alternatives in this
        /// alternator.
        auto end() const -> const_iterator;

    protected:
        friend class CommandManager::Builder;
        void push_back(AlternativeDef& alternative);

    private:
        arena_ptr<AlternativeDef> first{};
        arena_ptr<AlternativeDef> last{};
    };

    /// Stores information about an alternative command.
    ///
    /// See `AlternatorDef` for details.
    struct AlternativeDef : util::IntrusiveForwardListNode<AlternativeDef>
    {
        const CommandDef* command;

        explicit AlternativeDef(const CommandDef& command) : command(&command)
        {}

        // Provide the outer class access to the node pointers.
        friend class CommandManager;
    };

    /// Stores information about a string constant.
    ///
    /// See the language specification for details on string constants.
    /// https://gtamodding.github.io/gta3script-specs/core.html#string-constants
    struct ConstantDef : util::IntrusiveForwardListNode<ConstantDef>
    {
        /// The enumeration this string constant is part of.
        EnumId enum_id;
        /// The integer value of this string constant.
        int32_t value;

        explicit ConstantDef(EnumId enum_id, int32_t value) :
            enum_id(enum_id), value(value)
        {}

        // Provide the outer class access to the node pointers.
        friend class CommandManager;
    };

    // Those structures will be stored in an arena thus they need to be
    // trivially destructible.
    static_assert(std::is_trivially_destructible_v<ParamDef>);
    static_assert(std::is_trivially_destructible_v<CommandDef>);
    static_assert(std::is_trivially_destructible_v<AlternatorDef>);
    static_assert(std::is_trivially_destructible_v<AlternativeDef>);
    static_assert(std::is_trivially_destructible_v<ConstantDef>);

    // Keep the size of these structures under control.
    static_assert(sizeof(ParamDef) <= 8);
    static_assert(sizeof(ConstantDef) <= 16);
    static_assert(sizeof(AlternativeDef) <= 16);

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
    CommandManager() noexcept = default;

    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    CommandManager(CommandManager&&) noexcept = default;
    CommandManager& operator=(CommandManager&&) noexcept = default;

    ~CommandManager() noexcept = default;

    /// Finds the command with the specified name in the repository.
    ///
    /// The given name must be in uppercase or no command will be found.
    ///
    /// Returns a pointer to the command information or `nullptr` if not found.
    auto find_command(std::string_view name) const -> const CommandDef*;

    /// Finds the alternator with the specified name in the repository.
    ///
    /// The given name must be in uppercase or no alternator will be found.
    ///
    /// Returns a pointer to the alternator information or `nullptr` if not
    /// found.
    auto find_alternator(std::string_view name) const -> const AlternatorDef*;

    /// Finds the enumeration with the specified name in the repository.
    ///
    /// The given name must be in uppercase or no enumeration will be found.
    ///
    /// Returns the enumeration identifier or `std::nullopt` if not found.
    auto find_enumeration(std::string_view name) const -> std::optional<EnumId>;

    /// Finds a string constant of certain name in a given enumeration.
    ///
    /// The given name must be in uppercase or no string constant will be found.
    ///
    /// Returns the string constant information or `nullptr` if not found.
    auto find_constant(EnumId enum_id, std::string_view name) const
            -> const ConstantDef*;

    /// Finds a string constant of certain name in any enumeration (except the
    /// global enumeration).
    ///
    /// The given name must be in uppercase or no string constant will be found.
    ///
    /// If multiple string constants exist with the given name, the first one
    /// to be inserted into the repository (during the building process)
    /// takes precedence over the latter ones.
    ///
    /// Returns the string constant information or `nullptr` if not found.
    auto find_constant_any_means(std::string_view name) const
            -> const ConstantDef*;

    /// Finds the entity type with the specified name in the repository.
    ///
    /// The given name must be in uppercase or no entity will be found.
    ///
    /// Returns the entity type identifier or `std::nullopt` if not found.
    auto find_entity_type(std::string_view name) const
            -> std::optional<EntityId>;

private:
    /// Provides a mapping from a name to a command definition.
    using CommandsMap
            = std::unordered_map<std::string_view, arena_ptr<CommandDef>>;

    /// Provides a mapping from a name to a alternator definition.
    using AlternatorsMap
            = std::unordered_map<std::string_view, arena_ptr<AlternatorDef>>;

    /// Provides a mapping from a name to an enumeration identifier.
    using EnumsMap = std::unordered_map<std::string_view, EnumId>;

    /// Provides a mapping from a name to a list of constant definitions.
    using ConstantsMap
            = std::unordered_map<std::string_view, arena_ptr<ConstantDef>>;

    /// Provides a mapping from a name to an entity identifier.
    using EntitiesMap = std::unordered_map<std::string_view, EntityId>;

    /// Static version of `this->find_command(name)`.
    static auto find_command(const CommandsMap& commands_map,
                             std::string_view name) -> const CommandDef*;

    /// Static version of `this->find_alternator(name)`.
    static auto find_alternator(const AlternatorsMap& alternators_map,
                                std::string_view name) -> const AlternatorDef*;

    /// Static version of `this->find_enumeration(name)`.
    static auto find_enumeration(const EnumsMap& enums_map,
                                 std::string_view name)
            -> std::optional<EnumId>;

    /// Static version of `this->find_constant(enum_id, name)`.
    static auto find_constant(const ConstantsMap& constants_map, EnumId enum_id,
                              std::string_view name) -> const ConstantDef*;

    /// Static version of `this->find_constant_any_means(name)`.
    static auto find_constant_any_means(const ConstantsMap& constants_map,
                                        std::string_view name)
            -> const ConstantDef*;

    /// Static version of `this->find_entity_type(name)`.
    static auto find_entity_type(const EntitiesMap& entities_map,
                                 std::string_view name)
            -> std::optional<EntityId>;

private:
    /// Map of command names to their command definition.
    CommandsMap commands_map;

    /// Map of alternative names to a list of alternatives.
    AlternatorsMap alternators_map;

    /// Map of enumeration names to enumeration identifiers.
    EnumsMap enums_map;

    /// Map of constant names to a list of possible definitions.
    ConstantsMap constants_map;

    /// Map of entity names to entity identifiers.
    EntitiesMap entities_map;

protected:
    CommandManager(CommandsMap&& commands_map, AlternatorsMap&& alternators_map,
                   EnumsMap&& enums_map, ConstantsMap&& constants_map,
                   EntitiesMap&& entities_map) noexcept;
};

/// This is a builder capable of constructing a command repository.
///
/// Since `CommandManager` is immutable this builder should be used to
/// prepare the command definitions before passing it onto the repository.
class CommandManager::Builder
{
public:
    /// Constructs a builder.
    ///
    /// Allocations of command definitions, alternators, constants, and whatnot
    /// will be perfomed in the given arena. Therefore, the arena should be live
    /// as long as the resulting `CommandManager` is alive.
    explicit Builder(ArenaMemoryResource& arena) noexcept;

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder(Builder&&) noexcept = default;
    Builder& operator=(Builder&&) noexcept = default;

    ~Builder() noexcept = default;

    /// Builds the command repository.
    auto build() && -> CommandManager;

    /// Behaves the same as `CommandManager::find_command`.
    auto find_command(std::string_view name) const -> const CommandDef*;

    /// Behaves the same as `CommandManager::find_command`.
    auto find_command(std::string_view name) -> CommandDef*;

    /// Behaves the same as `CommandManager::find_alternator`.
    auto find_alternator(std::string_view name) const -> const AlternatorDef*;

    /// Behaves the same as `CommandManager::find_alternator`.
    auto find_alternator(std::string_view name) -> AlternatorDef*;

    /// Behaves the same as `CommandManager::find_enumeration`.
    auto find_enumeration(std::string_view name) const -> std::optional<EnumId>;

    /// Behaves the same as `CommandManager::find_constant`.
    auto find_constant(EnumId enum_id, std::string_view name) const
            -> const ConstantDef*;

    /// Behaves the same as `CommandManager::find_entity_type`.
    auto find_entity_type(std::string_view name) const
            -> std::optional<EntityId>;

    /// Inserts a command with the given name into the repository.
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
    /// params_end, params_end - params_begin)`.
    template<typename RandomAccessIterator>
    void set_command_params(CommandDef& command,
                            RandomAccessIterator params_begin,
                            RandomAccessIterator params_end);

    /// Inserts an alternator with the given name into the repository.
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

    /// Inserts an enumeration with the given name into the repository.
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
    /// `CommandManager::find_constant_any_means`. See its documentation
    /// for further details.
    ///
    /// Returns the string constant information and whether insertion took
    /// place.
    auto insert_or_assign_constant(EnumId enum_id, std::string_view name,
                                   int32_t value)
            -> std::pair<const ConstantDef*, bool>;

    /// Inserts an entity type with the given name into the repository.
    ///
    /// The given name is internally converted to uppercase. If an entity type
    /// of same name already exists, does nothing.
    ///
    /// Returns the entity identifier and whether insertion took place.
    auto insert_entity_type(std::string_view name) -> std::pair<EntityId, bool>;

private:
    /// The arena used to allocate definitions.
    ArenaMemoryResource* arena;

    // Maps that are going to be moved to `CommandManager`.
    CommandsMap commands_map;
    AlternatorsMap alternators_map;
    EnumsMap enums_map;
    ConstantsMap constants_map;
    EntitiesMap entities_map;
};
} // namespace gta3sc

namespace gta3sc
{
template<typename ForwardIterator>
void CommandManager::Builder::set_command_params(CommandDef& command,
                                                 ForwardIterator params_begin,
                                                 ForwardIterator params_end,
                                                 size_t params_size)
{
    auto* a_params = static_cast<ParamDef*>(
            arena->allocate(params_size * sizeof(ParamDef), alignof(ParamDef)));
    std::uninitialized_copy(params_begin, params_end, a_params);
    command.params = util::span(a_params, params_size);
}

template<typename RandomAccessIterator>
void CommandManager::Builder::set_command_params(
        CommandDef& command, RandomAccessIterator params_begin,
        RandomAccessIterator params_end)
{
    const size_t params_size = params_end - params_begin;
    return set_command_params(command, params_begin, params_end, params_size);
}
} // namespace gta3sc

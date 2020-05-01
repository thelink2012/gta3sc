#pragma once
#include <gta3sc/command-manager.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/intrusive-list-node.hpp>
#include <variant>

namespace gta3sc
{
/// This is an intermediate representation for semantically valid GTA3script.
///
/// One should be able to perform a complete analysis of the input source
/// code without a reference to any other data structure other than a sequence
/// of instructions of this type.
///
/// This structure may hold structured statements (e.g. IF, REPEAT) following
/// the same rules as `ParserIR`, except this time the arguments are guaranteed
/// to be correct.
///
/// Counter setting commands (e.g. SET_PROGRESS_TOTAL) may have its argument
/// different from zero.
///
/// This IR is immutable (except for [1]) and preserves source code information
/// such as the location of each of its arguments.
///
/// [1]: The IR is mostly modelled as an intrusive linked list (see `LinkedIR`)
/// and the node pointers present in this structure have interior mutability.
class SemaIR : public util::IntrusiveBidirectionalListNode<SemaIR>
{
public:
    struct Command;
    struct Argument;
    class Builder;

public:
    const SymLabel* const label{};
    arena_ptr<const Command> const command{};

public:
    SemaIR() noexcept = delete;
    ~SemaIR() noexcept = default;

    SemaIR(const SemaIR&) = delete;
    SemaIR& operator=(const SemaIR&) = delete;

    SemaIR(SemaIR&&) noexcept = delete;
    SemaIR& operator=(SemaIR&&) noexcept = delete;

    // Creates an instruction.
    static auto create(const SymLabel* label, arena_ptr<const Command> command,
                       ArenaMemoryResource*) -> arena_ptr<SemaIR>;

    /// Creates an integer argument.
    static auto create_integer(int32_t value, SourceRange source,
                               ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a label argument.
    static auto create_label(const SymLabel& label, SourceRange source,
                             ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a text label argument.
    ///
    /// The text label value is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_text_label(std::string_view value, SourceRange source,
                                  ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view value, SourceRange source,
                              ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a variable reference argument.
    static auto create_variable(const SymVariable& var, SourceRange source,
                                ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates an array variable reference argument by using the given integer
    /// index.
    static auto create_variable(const SymVariable& var, int32_t index,
                                SourceRange source, ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates an array variable reference argument by using the given variable
    /// index.
    static auto create_variable(const SymVariable& var,
                                const SymVariable& index, SourceRange source,
                                ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a string constant argument.
    static auto create_constant(const CommandManager::ConstantDef& cdef,
                                SourceRange source, ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a used object argument.
    static auto create_used_object(const SymUsedObject& used_object,
                                   SourceRange source,
                                   ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    struct Command
    {
    public:
        SourceRange source; ///< Source code  location of the command.
        const CommandManager::CommandDef& def; ///< The command definition.
        util::span<arena_ptr<const Argument>>
                args;          ///< View into the arguments.
        bool not_flag = false; ///< Whether the result of the command is NOTed.

        /// Please use `SemaIR::Builder::build_command`.
        Command() noexcept = delete;
        ~Command() noexcept = default;

        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        Command(Command&&) noexcept = delete;
        Command& operator=(Command&&) noexcept = delete;

    protected:
        Command(SourceRange source, const CommandManager::CommandDef& def,
                util::span<arena_ptr<const Argument>> args, bool not_flag) :
            source(source), def(def), args(args), not_flag(not_flag)
        {}

        friend class Builder;
    };

    /// Arguments are immutable and may be shared by multiple commands.
    struct Argument
    {
    public:
        struct VarRef;

    public:
        SourceRange source;

        /// Please use `SemaIR` creation methods.
        Argument() noexcept = delete;
        ~Argument() noexcept = default;

        Argument(const Argument&) = delete;
        Argument& operator=(const Argument&) = delete;

        Argument(Argument&&) noexcept = delete;
        Argument& operator=(Argument&&) noexcept = delete;

        /// Returns the contained integer or `nullptr` if this argument is not
        /// an integer.
        auto as_integer() const -> const int32_t*;

        /// Returns the contained float or `nullptr` if this argument is not
        /// a float.
        auto as_float() const -> const float*;

        /// Returns the contained text label value or `nullptr` if this argument
        /// is not a text label.
        auto as_text_label() const -> const std::string_view*;

        /// Returns the contained string or `nullptr` if this argument is not
        /// a string.
        auto as_string() const -> const std::string_view*;

        /// Returns the contained variable reference or `nullptr` if this
        /// argument is not a variable reference.
        auto as_var_ref() const -> const VarRef*;

        /// Returns the contained label reference or `nullptr` if this argument
        /// is not a label reference.
        auto as_label() const -> const SymLabel*;

        /// Returns the contained string constant or `nullptr` if this argument
        /// is not a string constant.
        auto as_constant() const -> const CommandManager::ConstantDef*;

        /// Returns the contained used object or `nullptr` if this argument is
        /// not a used object.
        auto as_used_object() const -> const SymUsedObject*;

        /// Type-puns the contained integer or string constant as an integer.
        ///
        /// \note This does not type-pun used objects as there is no guarantee
        /// (speaking formally) that it can actually be represented as an
        /// integer.
        auto pun_as_integer() const -> std::optional<int32_t>;

        /// Type-puns the contained float (or, in the future,
        /// user defined floating-point constant) as a float.
        auto pun_as_float() const -> std::optional<float>;

    public:
        struct VarRef
        {
            const SymVariable& def; ///< The variable being referenced.
            std::variant<std::monostate, int32_t, const SymVariable*>
                    index; ///< The index of an array subscript.

            /// Checks whether this variable reference is an array reference.
            bool has_index() const;

            /// Returns the integer in the array subscript or `nullptr` if
            /// either this is not an array or the index is not an integer.
            auto index_as_integer() const -> const int32_t*;

            /// Returns the variable in the array subscript or `nullptr` if
            /// either this is not an array or the index is not a variable.
            auto index_as_variable() const -> const SymVariable*;
        };

    protected:
        friend class SemaIR;

        enum class TextLabelTag
        {
        };
        enum class StringTag
        {
        };

        // Tagging adds one word of overhead to the memory used by an
        // argument, but is cleaner than a EqualityComparable wrapper.
        using TextLabel = std::pair<TextLabelTag, std::string_view>;
        using String = std::pair<StringTag, std::string_view>;

        template<typename T>
        explicit Argument(T&& value, SourceRange source) :
            source(source), value(std::forward<T>(value))
        {}

        template<typename Tag>
        explicit Argument(Tag tag, std::string_view value, SourceRange source) :
            source(source), value(std::pair{tag, value})
        {}

    private:
        const std::variant<int32_t, float, TextLabel, String, VarRef,
                           const SymLabel*, const SymUsedObject*,
                           const CommandManager::ConstantDef*>
                value;

        // Keep the size of this structure small.
        static_assert(sizeof(value) <= 4 * sizeof(void*));
    };

private:
    explicit SemaIR(const SymLabel* label, arena_ptr<const Command> command) :
        label(label), command(command)
    {}
};

/// This is a builder capable of constructing a SemaIR instruction.
///
/// The purpose of this builder is to make the construction of SemaIR
/// instructions as little verbose as possible. It's simply an wrapper
/// around the create methods of `SemaIR`.
class SemaIR::Builder
{
public:
    static constexpr SourceRange no_source = SourceManager::no_source_range;

    /// Constructs a builder to create instructions allocating any necessary
    /// data in the given arena.
    explicit Builder(ArenaMemoryResource& arena) noexcept : arena(&arena) {}

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder(Builder&&) noexcept = default;
    Builder& operator=(Builder&&) noexcept = default;

    ~Builder() noexcept = default;

    /// Sets (or unsets if `nullptr`) the instruction in construction to
    /// define the specified label.
    auto label(const SymLabel*) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    ///
    /// No other instruction to construct commands should have been called
    /// before this (i.e. only `Builder::label` could have been called).
    auto command(arena_ptr<const Command>) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    auto command(const CommandManager::CommandDef&,
                 SourceRange source = no_source) -> Builder&&;

    /// Sets the not flag of the command being constructed.
    auto not_flag(bool not_flag_value = true) -> Builder&&;

    /// Appends the given argument to the command in construction.
    auto arg(arena_ptr<const Argument> value) -> Builder&&;

    /// Appends the given integer argument to the command in construction.
    auto arg_int(int32_t value, SourceRange source = no_source) -> Builder&&;

    /// Appends the given float argument to the command in construction.
    auto arg_float(float value, SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing the given label to the command in
    /// construction.
    auto arg_label(const SymLabel& label, SourceRange source = no_source)
            -> Builder&&;

    /// Appends the given string argument to the command in construction.
    auto arg_text_label(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends the given string argument to the command in construction.
    auto arg_string(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends an argument referencing to the given variable to the command in
    /// construction.
    auto arg_var(const SymVariable& var, SourceRange source = no_source)
            -> Builder&&;

    /// Appends an argument referencing to the given variable and given
    /// array subscript index to the command in construction.
    auto arg_var(const SymVariable& var, int32_t index,
                 SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given variable and given
    /// array subscript index to the command in construction.
    auto arg_var(const SymVariable& var, const SymVariable& index,
                 SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given string constant to the
    /// command in construction.
    auto arg_const(const CommandManager::ConstantDef& cdef,
                   SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given used object to the
    /// command in construction.
    auto arg_object(const SymUsedObject& used_object,
                    SourceRange source = no_source) -> Builder&&;

    /// Builds an instruction from the attributes in the builder.
    auto build() && -> arena_ptr<SemaIR>;

    /// Builds a command from the attributes in the builder.
    auto build_command() && -> arena_ptr<const SemaIR::Command>;

private:
    /// Consumes some of the attributes in this builder and transforms it
    /// into an `Command` by setting `command_ptr`.
    void create_command_from_attributes();

private:
    ArenaMemoryResource* arena;

    bool has_command_def = false;
    bool has_not_flag = false;
    bool not_flag_value = false;

    const SymLabel* label_ptr{};
    arena_ptr<const Command> command_ptr{};

    const CommandManager::CommandDef* command_def{};
    SourceRange command_source;

    size_t args_capacity = 0;
    util::span<arena_ptr<const Argument>> args;
};

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<SemaIR>);
static_assert(std::is_trivially_destructible_v<SemaIR::Command>);
static_assert(std::is_trivially_destructible_v<SemaIR::Argument>);
} // namespace gta3sc

// TODO replace arena* with arena& (in parserir too)
// TODO the builder should probably not be returning an rvalue to itself unless
// && is specified in the method specification (in parserir too).
// TODO use some kind of strong_typedef instead of std::pair in Argument

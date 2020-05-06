#pragma once
#include <gta3sc/command-manager.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/util/intrusive-bidirectional-list-node.hpp>
#include <gta3sc/util/random-access-view.hpp>
#include <gta3sc/util/string-vieweable.hpp>
#include <gta3sc/util/visit.hpp>
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
/// and the node pointers present in this structure need to change to manipulate
/// the list.
class SemaIR : public util::IntrusiveBidirectionalListNode<SemaIR>
{
private:
    /// This tag is used to make construction of the IR elements private.
    struct PrivateTag
    {
    };
    static constexpr PrivateTag private_tag{};

public:
    class Command;
    class Argument;
    class ArgumentView;
    class Builder;

    struct TextLabel;
    struct String;
    class VarRef;

public:
    /// Please use `create` instead.
    SemaIR(PrivateTag /*unused*/, const SymbolTable::Label* label,
           const Command* command) noexcept :
        m_label(label), m_command(command)
    {}

    SemaIR(const SemaIR&) = delete;
    auto operator=(const SemaIR&) -> SemaIR& = delete;

    SemaIR(SemaIR&&) noexcept = delete;
    auto operator=(SemaIR&&) noexcept -> SemaIR& = delete;

    ~SemaIR() noexcept = default;

    /// Checks whether there is a command associated with this instruction.
    [[nodiscard]] auto has_command() const noexcept -> bool
    {
        return m_command != nullptr;
    }

    /// Checks whether there is a label associated with this instruction.
    [[nodiscard]] auto has_label() const noexcept -> bool
    {
        return m_label != nullptr;
    }

    /// Returns the label associated with this instruction.
    [[nodiscard]] auto label() const noexcept -> const SymbolTable::Label&
    {
        return *m_label;
    }

    /// Returns the label associated with this instruction or `nullptr` if none.
    [[nodiscard]] auto label_or_null() const noexcept
            -> const SymbolTable::Label*
    {
        return m_label ? m_label : nullptr;
    }

    /// Returns the command associated with this instruction.
    [[nodiscard]] auto command() const noexcept -> const Command&
    {
        return *m_command;
    }

    /// Returns the command associated with this instruction or `nullptr` if
    /// none.
    [[nodiscard]] auto command_or_null() const noexcept -> const Command*
    {
        return m_command ? m_command : nullptr;
    }

    /// Checks whether a given IR is equivalent to another IR.
    friend auto operator==(const SemaIR& lhs, const SemaIR& rhs) noexcept
            -> bool;
    friend auto operator!=(const SemaIR& lhs, const SemaIR& rhs) noexcept
            -> bool;

    //
    // Factory methods
    //

    // Creates an instruction.
    static auto create(const SymbolTable::Label* label, const Command* command,
                       ArenaAllocator<> allocator) -> ArenaPtr<SemaIR>;

    /// Creates an integer argument.
    static auto create_int(int32_t value, SourceRange source,
                           ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a label argument.
    static auto create_label(const SymbolTable::Label& label,
                             SourceRange source, ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a text label argument.
    ///
    /// The text label value is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_text_label(std::string_view value, SourceRange source,
                                  ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view value, SourceRange source,
                              ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a variable reference argument.
    static auto create_variable(const SymbolTable::Variable& var,
                                SourceRange source, ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates an array variable reference argument by using the given integer
    /// index.
    static auto create_variable(const SymbolTable::Variable& var, int32_t index,
                                SourceRange source, ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates an array variable reference argument by using the given variable
    /// index.
    static auto create_variable(const SymbolTable::Variable& var,
                                const SymbolTable::Variable& index,
                                SourceRange source, ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a string constant argument.
    static auto create_constant(const CommandManager::ConstantDef& cdef,
                                SourceRange source, ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a used object argument.
    static auto create_used_object(const SymbolTable::UsedObject& used_object,
                                   SourceRange source,
                                   ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

private:
    enum class TextLabelTag
    {
    };
    enum class StringTag
    {
    };

    /// Same as `util::WeakStringVieweable` but with private construction.
    template<typename Tag>
    struct WeakStringVieweable : util::WeakStringVieweable<Tag>
    {
        constexpr WeakStringVieweable(PrivateTag /*unused*/,
                                      std::string_view value) noexcept :
            util::WeakStringVieweable<Tag>(value)
        {}
    };

private:
    const SymbolTable::Label* const m_label{};
    const Command* const m_command{};
};

/// Represents an text label argument.
///
/// \note This is nothing but a view to a sequence of characters, hence
/// it should live at most as long as the IR is alive.
///
/// Implicitly convertible to `std::string_view`.
struct SemaIR::TextLabel : WeakStringVieweable<TextLabelTag>
{
    using WeakStringVieweable::WeakStringVieweable;
};

/// Represents a string argument.
///
/// \note This is nothing but a view to a sequence of characters, hence
/// it should live at most as long as the IR is alive.
///
/// Implicitly convertible to `std::string_view`.
struct SemaIR::String : WeakStringVieweable<StringTag>
{
    using WeakStringVieweable::WeakStringVieweable;
};

/// Represents a variable reference.
class SemaIR::VarRef
{
public:
    VarRef(PrivateTag /*unused*/, const SymbolTable::Variable& def) :
        m_def(&def)
    {}

    VarRef(PrivateTag /*unused*/, const SymbolTable::Variable& def,
           int32_t index) :
        m_def(&def), m_index(index)
    {}

    VarRef(PrivateTag /*unused*/, const SymbolTable::Variable& def,
           const SymbolTable::Variable& index) :
        m_def(&def), m_index(&index)
    {}

    /// Returns the variable referenced by this.
    [[nodiscard]] auto def() const noexcept -> const SymbolTable::Variable&;

    /// Checks whether this variable reference is an array reference.
    [[nodiscard]] auto has_index() const noexcept -> bool;

    /// Returns the integer in the array subscript or `std::nullopt` if
    /// either this is not an array or the index is not an integer.
    [[nodiscard]] auto index_as_int() const noexcept -> std::optional<int32_t>;

    /// Returns the variable in the array subscript or `nullptr` if
    /// either this is not an array or the index is not a variable.
    [[nodiscard]] auto index_as_variable() const noexcept
            -> const SymbolTable::Variable*;

    /// Compares whether two variable references are equal.
    friend auto operator==(const VarRef& lhs, const VarRef& rhs) noexcept
            -> bool;
    friend auto operator!=(const VarRef& lhs, const VarRef& rhs) noexcept
            -> bool;

private:
    const SymbolTable::Variable* m_def;
    std::variant<std::monostate, int32_t, const SymbolTable::Variable*> m_index;
};

/// Represents a view to a sequence of arguments.
class SemaIR::ArgumentView
    : public util::RandomAccessView<const Argument*,
                                    util::iterator_adaptor::DereferenceAdaptor>
{
public:
    ArgumentView(PrivateTag /*unused*/, const Argument** begin, size_t count) :
        util::RandomAccessView<const Argument*,
                               util::iterator_adaptor::DereferenceAdaptor>(
                begin, count)
    {}
};

class SemaIR::Command
{
public:
    /// Please use `SemaIR::Builder::build_command`.
    Command(PrivateTag /*unused*/, SourceRange source,
            const CommandManager::CommandDef& def,
            util::span<const Argument*> args, bool not_flag) noexcept :
        m_source(source), m_def(&def), m_args(args), m_not_flag(not_flag)
    {}

    Command(const Command&) = delete;
    auto operator=(const Command&) -> Command& = delete;

    Command(Command&&) noexcept = delete;
    auto operator=(Command&&) noexcept -> Command& = delete;

    ~Command() noexcept = default;

    /// Checks whether the not flag of this command is active.
    [[nodiscard]] auto not_flag() const noexcept -> bool { return m_not_flag; }

    /// Returns the source code range of this command.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the definition of this command.
    [[nodiscard]] auto def() const noexcept -> const CommandManager::CommandDef&
    {
        return *m_def;
    }

    /// Checks whether th ecommand has at least one argument.
    [[nodiscard]] auto has_args() const noexcept -> bool
    {
        return !m_args.empty();
    }

    /// Returns the number of arguments of this command.
    [[nodiscard]] auto num_args() const noexcept -> size_t
    {
        return m_args.size();
    }

    /// Returns a view to the arguments of the command.
    [[nodiscard]] auto args() const noexcept -> ArgumentView
    {
        return ArgumentView(private_tag, m_args.data(), m_args.size());
    }

    /// Returns the i-th argument of this command.
    [[nodiscard]] auto arg(size_t i) const noexcept -> const Argument&
    {
        return *m_args[i];
    }

    /// Checks whether a given command is equivalent to another command.
    friend auto operator==(const Command& lhs, const Command& rhs) noexcept
            -> bool;
    friend auto operator!=(const Command& lhs, const Command& rhs) noexcept
            -> bool;

private:
    SourceRange m_source;
    const CommandManager::CommandDef* m_def;
    util::span<const Argument*> m_args;
    bool m_not_flag{};
};

class SemaIR::Argument
{
public:
    enum class Type
    {                // GTA3script types should be uppercase.
        INT,         // NOLINT(readability-identifier-naming)
        FLOAT,       // NOLINT(readability-identifier-naming)
        TEXT_LABEL,  // NOLINT(readability-identifier-naming)
        STRING,      // NOLINT(readability-identifier-naming)
        VARIABLE,    // NOLINT(readability-identifier-naming)
        LABEL,       // NOLINT(readability-identifier-naming)
        USED_OBJECT, // NOLINT(readability-identifier-naming)
        CONSTANT,    // NOLINT(readability-identifier-naming)
    };

public:
    /// Please use `SemaIR` creation methods.
    template<typename T>
    explicit Argument(PrivateTag /*unused*/, T&& value, SourceRange source) :
        m_source(source), m_value(std::forward<T>(value))
    {}

    Argument(const Argument&) = delete;
    auto operator=(const Argument&) -> Argument& = delete;

    Argument(Argument&&) noexcept = delete;
    auto operator=(Argument&&) noexcept -> Argument& = delete;

    ~Argument() noexcept = default;

    /// Returns the source code range of this argument.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the type of this argument.
    [[nodiscard]] auto type() const noexcept -> Type
    {
        return static_cast<Type>(m_value.index());
    }

    /// Returns the contained integer or `std::nullopt` if this argument is not
    /// an integer.
    [[nodiscard]] auto as_int() const noexcept -> std::optional<int32_t>;

    /// Returns the contained float or `std::nullopt` if this argument is not
    /// a float.
    [[nodiscard]] auto as_float() const noexcept -> std::optional<float>;

    /// Returns the contained text label value or `nullptr` if this argument
    /// is not a text label.
    [[nodiscard]] auto as_text_label() const noexcept
            -> std::optional<TextLabel>;

    /// Returns the contained string or `nullptr` if this argument is not
    /// a string.
    [[nodiscard]] auto as_string() const noexcept -> std::optional<String>;

    /// Returns the contained variable reference or `nullptr` if this
    /// argument is not a variable reference.
    [[nodiscard]] auto as_var_ref() const noexcept -> std::optional<VarRef>;

    /// Returns the contained label reference or `nullptr` if this argument
    /// is not a label reference.
    [[nodiscard]] auto as_label() const noexcept -> const SymbolTable::Label*;

    /// Returns the contained string constant or `nullptr` if this argument
    /// is not a string constant.
    [[nodiscard]] auto as_constant() const noexcept
            -> const CommandManager::ConstantDef*;

    /// Returns the contained used object or `nullptr` if this argument is
    /// not a used object.
    [[nodiscard]] auto as_used_object() const noexcept
            -> const SymbolTable::UsedObject*;

    /// Type-puns the contained integer or string constant as an integer.
    ///
    /// \note This does not type-pun used objects as there is no guarantee
    /// (speaking formally) that it can actually be represented as an
    /// integer.
    [[nodiscard]] auto pun_as_int() const noexcept -> std::optional<int32_t>;

    /// Type-puns the contained float (or, in the future,
    /// user defined floating-point constant) as a float.
    [[nodiscard]] auto pun_as_float() const noexcept -> std::optional<float>;

    /// Checks whether a given argument is equivalent to another argument.
    friend auto operator==(const Argument& lhs, const Argument& rhs) noexcept
            -> bool;
    friend auto operator!=(const Argument& lhs, const Argument& rhs) noexcept
            -> bool;

private:
    SourceRange m_source;
    const std::variant<int32_t, float, TextLabel, String, VarRef,
                       const SymbolTable::Label*,
                       const SymbolTable::UsedObject*,
                       const CommandManager::ConstantDef*>
            m_value;
    // FIXME cannot change the order of the variant or type() will break.

    // Keep the size of this structure small.
    static_assert(sizeof(m_value) <= 4 * sizeof(void*));
};

/// This is a builder capable of constructing a SemaIR instruction.
///
/// The purpose of this builder is to make the construction of SemaIR
/// instructions as little verbose as possible. It's simply an wrapper
/// around the create methods of `SemaIR`.
///
/// This builder allocates memory on the arena in most method calls,
/// therefore it is discouraged, and in some cases disallowed, to
/// replace attributes to avoid users inadvertently allocating more
/// memory than necessary.
class SemaIR::Builder
{
public:
    static constexpr SourceRange no_source = SourceManager::no_source_range;

    /// Constructs a builder to create instructions allocating any necessary
    /// data in the given arena.
    explicit Builder(ArenaAllocator<> allocator) noexcept : allocator(allocator)
    {}

    Builder(const Builder&) = delete;
    auto operator=(const Builder&) -> Builder& = delete;

    Builder(Builder&&) noexcept = default;
    auto operator=(Builder&&) noexcept -> Builder& = default;

    ~Builder() noexcept = default;

    /// Sets (or unsets if `nullptr`) the instruction in construction to
    /// define the specified label.
    auto label(const SymbolTable::Label* label_ptr) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    ///
    /// No other instruction to construct commands should have been called
    /// before this (i.e. only `Builder::label` could have been called).
    auto command(const Command* command_ptr) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    auto command(const CommandManager::CommandDef& command_def,
                 SourceRange source = no_source) -> Builder&&;

    /// Sets the not flag of the command being constructed.
    auto not_flag(bool not_flag_value = true) -> Builder&&;

    /// Appends the given argument to the command in construction.
    auto arg(const Argument* value) -> Builder&&;

    /// Appends the given integer argument to the command in construction.
    auto arg_int(int32_t value, SourceRange source = no_source) -> Builder&&;

    /// Appends the given float argument to the command in construction.
    auto arg_float(float value, SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing the given label to the command in
    /// construction.
    auto arg_label(const SymbolTable::Label& label,
                   SourceRange source = no_source) -> Builder&&;

    /// Appends the given string argument to the command in construction.
    auto arg_text_label(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends the given string argument to the command in construction.
    auto arg_string(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends an argument referencing to the given variable to the command in
    /// construction.
    auto arg_var(const SymbolTable::Variable& var,
                 SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given variable and given
    /// array subscript index to the command in construction.
    auto arg_var(const SymbolTable::Variable& var, int32_t index,
                 SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given variable and given
    /// array subscript index to the command in construction.
    auto arg_var(const SymbolTable::Variable& var,
                 const SymbolTable::Variable& index,
                 SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given string constant to the
    /// command in construction.
    auto arg_const(const CommandManager::ConstantDef& cdef,
                   SourceRange source = no_source) -> Builder&&;

    /// Appends an argument referencing to the given used object to the
    /// command in construction.
    auto arg_object(const SymbolTable::UsedObject& used_object,
                    SourceRange source = no_source) -> Builder&&;

    /// Tells the builder the amount of arguments that follows.
    ///
    /// This is helpful (but not necessary) to pre-allocate the necessary
    /// storage for the argument table.
    ///
    /// \note must be called before any of the `arg` methods.
    /// \note `num_args` must be greater or equal to the number of `arg` method
    /// calls.
    auto with_num_args(size_t num_args) -> Builder&&;

    /// Equivalent to calling `with_num_args(distance(begin, end))` followed by
    /// `arg(a)` for each element in the sequence described by `begin...end`.
    template<typename InputIterator>
    auto with_args(InputIterator begin, InputIterator end) -> Builder&&;

    /// Builds an instruction from the attributes in the builder.
    auto build() && -> ArenaPtr<SemaIR>;

    /// Builds a command from the attributes in the builder.
    auto build_command() && -> ArenaPtr<const SemaIR::Command>;

private:
    /// Consumes some of the attributes in this builder and transforms it
    /// into an `Command` by setting `command_ptr`.
    void create_command_from_attributes();

private:
    ArenaAllocator<> allocator;

    bool has_command_def = false;
    bool has_not_flag = false;
    bool not_flag_value = false;

    const SymbolTable::Label* label_ptr{};
    const Command* command_ptr{};

    const CommandManager::CommandDef* command_def{};
    SourceRange command_source;

    size_t args_hint = -1;
    size_t args_capacity = 0;
    util::span<const Argument*> args;
};

template<typename InputIterator>
auto SemaIR::Builder::with_args(InputIterator begin, InputIterator end)
        -> Builder&&
{
    with_num_args(std::distance(begin, end));
    for(auto it = begin; it != end; ++it)
        arg(*it);
    return std::move(*this);
}

/// Applies the visitor `vis` to the arguments `arg, other_args...`.
///
/// This is equivalent to `vis(*arg.as_TYPE(), *other_args.as_TYPEN()...)` where
/// `TYPE` is the underlying type of the argument.
template<typename Visitor, typename... OtherArgs>
inline auto visit(Visitor&& vis, const SemaIR::Argument& arg,
                  OtherArgs&&... other_args)
{
    switch(arg.type())
    {
        using Type = SemaIR::Argument::Type;
        case Type::INT:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_int(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::FLOAT:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_float(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::TEXT_LABEL:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_text_label(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::STRING:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_string(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::VARIABLE:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_var_ref(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::LABEL:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_label(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::USED_OBJECT:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_used_object(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::CONSTANT:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_constant(),
                               std::forward<OtherArgs>(other_args)...);
        }
        default:
            assert(false);
    }
}

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<SemaIR>);
static_assert(std::is_trivially_destructible_v<SemaIR::Command>);
static_assert(std::is_trivially_destructible_v<SemaIR::Argument>);
} // namespace gta3sc

// TODO the builder should probably not be returning an rvalue to itself unless
// && is specified in the method specification (in parserir too).

#pragma once
#include <cstdint>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena-allocator.hpp>
#include <gta3sc/util/intrusive-bidirectional-list-node.hpp>
#include <gta3sc/util/span.hpp>
#include <gta3sc/util/string-vieweable.hpp>
#include <gta3sc/util/visit.hpp>
#include <string_view>
#include <variant>

namespace gta3sc
{
/// This is an intermediate representation for syntactically valid GTA3script.
///
/// The implication of this being syntatically valid is that e.g.
/// for every WHILE command, there is a matching ENDWHILE one.
///
/// Structured statements such as IF and WHILE also become IR (they
/// are commands after all) where their first argument determines how
/// many of the following instructions are part of the conditional
/// list, much like the unstructured ANDOR command.
///
/// Do note however that no argument matching is performed whatsoever.
/// This implies commands with a syntactical specification (e.g. GOSUB_FILE,
/// VAR_INT) might not follow it (e.g. the first argument of GOSUB_FILE might
/// not be an identifier). The amount of arguments of these commands however
/// are guaranted to be correct.
///
/// This IR is immutable (except for [1]) and preserves source code information
/// such as the location of each of its identifiers.
///
/// [1]: The IR is mostly modelled as an intrusive linked list (see `LinkedIR`)
/// and the node pointers present in this structure need to change to manipulate
/// the list.
class ParserIR : public util::IntrusiveBidirectionalListNode<ParserIR>
{
private:
    /// This tag is used to make construction of the IR elements private.
    struct PrivateTag
    {
    };
    static constexpr PrivateTag private_tag{};

public:
    class LabelDef;
    class Command;
    class Argument;
    class Builder;

    struct Identifier;
    struct Filename;
    struct String;

public:
    /// Please use `create` instead.
    ParserIR(PrivateTag /*unused*/, const LabelDef* label,
             const Command* command) noexcept :
        m_label(label), m_command(command)
    {}

    ParserIR(const ParserIR&) = delete;
    auto operator=(const ParserIR&) -> ParserIR& = delete;

    ParserIR(ParserIR&&) noexcept = delete;
    auto operator=(ParserIR&&) noexcept -> ParserIR& = delete;

    ~ParserIR() noexcept = default;

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
    [[nodiscard]] auto label() const noexcept -> const LabelDef&
    {
        return *m_label;
    }

    /// Returns the label associated with this instruction or `nullptr` if none.
    [[nodiscard]] auto label_or_null() const noexcept -> const LabelDef*
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
    friend auto operator==(const ParserIR& lhs, const ParserIR& rhs) noexcept
            -> bool;
    friend auto operator!=(const ParserIR& lhs, const ParserIR& rhs) noexcept
            -> bool;

    //
    // Factory methods
    //

    // Creates an instruction.
    static auto create(const LabelDef* label, const Command* command,
                       ArenaAllocator<> allocator) -> ArenaPtr<ParserIR>;

    /// Creates an integer argument.
    static auto create_int(int32_t value, SourceRange source,
                           ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates an identifier argument.
    ///
    /// The identifier is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_identifier(std::string_view name, SourceRange source,
                                  ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a filename argument.
    ///
    /// The filename is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_filename(std::string_view filename, SourceRange source,
                                ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view string, SourceRange source,
                              ArenaAllocator<> allocator)
            -> ArenaPtr<const Argument>;

private:
    struct IdentifierTag
    {
    };

    struct FilenameTag
    {
    };

    struct StringTag
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
    const LabelDef* const m_label{};
    const Command* const m_command{};
};

/// Represents an identifier argument.
///
/// \note This is nothing but a view to a sequence of characters, hence
/// it should live at most as long as the IR is alive.
///
/// Implicitly convertible to `std::string_view`.
struct ParserIR::Identifier : WeakStringVieweable<IdentifierTag>
{
    using WeakStringVieweable::WeakStringVieweable;
};

/// Represents a filename argument.
///
/// \note This is nothing but a view to a sequence of characters, hence
/// it should live at most as long as the IR is alive.
///
/// Implicitly convertible to `std::string_view`.
struct ParserIR::Filename : WeakStringVieweable<FilenameTag>
{
    using WeakStringVieweable::WeakStringVieweable;
};

/// Represents a string argument.
///
/// \note This is nothing but a view to a sequence of characters, hence
/// it should live at most as long as the IR is alive.
///
/// Implicitly convertible to `std::string_view`.
struct ParserIR::String : WeakStringVieweable<StringTag>
{
    using WeakStringVieweable::WeakStringVieweable;
};

class ParserIR::LabelDef
{
public:
    /// Please ues `create` instead.
    LabelDef(PrivateTag /*unused*/, SourceRange source,
             std::string_view name) noexcept :
        m_source(source), m_name(name)
    {}

    LabelDef(const LabelDef&) = delete;
    auto operator=(const LabelDef&) -> LabelDef& = delete;

    LabelDef(LabelDef&&) noexcept = delete;
    auto operator=(LabelDef&&) noexcept -> LabelDef& = delete;

    ~LabelDef() noexcept = default;

    /// Returns the source code range of this label definition.
    [[nodiscard]] auto source() const noexcept -> SourceRange
    {
        return m_source;
    }

    /// Returns the name of the label defined by this.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
    }

    /// Compares whether a given label definition is equivalent to another.
    friend auto operator==(const LabelDef& lhs, const LabelDef& rhs) noexcept
            -> bool;
    friend auto operator!=(const LabelDef& lhs, const LabelDef& rhs) noexcept
            -> bool;

    /// Creates a label definition.
    ///
    /// The name of the label is automatically made uppercase.
    static auto create(std::string_view name, SourceRange source,
                       ArenaAllocator<> allocator) -> ArenaPtr<const LabelDef>;

private:
    SourceRange m_source;
    std::string_view m_name;
};

class ParserIR::Command
{
public:
    /// Please use `ParserIR::Builder::build_command`.
    Command(PrivateTag /*unused*/, SourceRange source, std::string_view name,
            util::span<const Argument*> args, bool not_flag) noexcept :
        m_source(source), m_name(name), m_args(args), m_not_flag(not_flag)
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

    /// Returns the name of the command.
    [[nodiscard]] auto name() const noexcept -> std::string_view
    {
        return m_name;
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
    [[nodiscard]] auto args() const noexcept -> util::span<const Argument*>
    {
        return m_args;
    }

    /// Returns the i-th argument of this command.
    [[nodiscard]] auto arg(size_t i) const noexcept -> const Argument&
    {
        return *m_args[i];
    }

    /// Compares whether a given command is equivalent to another command.
    friend auto operator==(const Command& lhs, const Command& rhs) noexcept
            -> bool;
    friend auto operator!=(const Command& lhs, const Command& rhs) noexcept
            -> bool;

private:
    SourceRange m_source;
    std::string_view m_name;
    util::span<const Argument*> m_args;
    bool m_not_flag{};
};

class ParserIR::Argument
{
public:
    enum class Type
    {               // GTA3script types should be uppercase.
        INT,        // NOLINT(readability-identifier-naming)
        FLOAT,      // NOLINT(readability-identifier-naming)
        IDENTIFIER, // NOLINT(readability-identifier-naming)
        FILENAME,   // NOLINT(readability-identifier-naming)
        STRING      // NOLINT(readability-identifier-naming)
    };

public:
    /// Please use `ParserIR` creation methods.
    template<typename T>
    Argument(PrivateTag /*unused*/, T&& value, SourceRange source) noexcept :
        m_source(source), m_value(std::forward<T>(value))
    {}

    ~Argument() noexcept = default;

    Argument(const Argument&) = delete;
    auto operator=(const Argument&) -> Argument& = delete;

    Argument(Argument&&) noexcept = delete;
    auto operator=(Argument&&) noexcept -> Argument& = delete;

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
    /// an float.
    [[nodiscard]] auto as_float() const noexcept -> std::optional<float>;

    /// Returns the contained identifier or `std::nullopt` if this argument is
    /// not an identifier.
    [[nodiscard]] auto as_identifier() const noexcept
            -> std::optional<Identifier>;

    /// Returns the contained filename or `std::nullopt` if this argument is not
    /// a filename.
    [[nodiscard]] auto as_filename() const noexcept -> std::optional<Filename>;

    /// Returns the contained string or `std::nullopt` if this argument is not
    /// a string.
    [[nodiscard]] auto as_string() const noexcept -> std::optional<String>;

    /// Returns whether the value of this is equal the value of another
    /// argument (i.e. same as `operator==` without comparing source
    /// location).
    [[nodiscard]] auto is_same_value(const Argument& other) const noexcept
            -> bool;

    /// Compares whether a given argument is equivalent to another.
    friend auto operator==(const Argument& lhs, const Argument& rhs) noexcept
            -> bool;
    friend auto operator!=(const Argument& lhs, const Argument& rhs) noexcept
            -> bool;

private:
    SourceRange m_source;
    const std::variant<int32_t, float, Identifier, Filename, String> m_value;
    // FIXME cannot change the order of the variant or type() will break.

    // Keep the size of this structure small.
    // This would improve if we used a strong typedef over std::pair.
    static_assert(sizeof(m_value) <= 4 * sizeof(void*));
};

/// This is a builder capable of constructing a ParserIR instruction.
///
/// The purpose of this builder is to make the construction of ParserIR
/// instructions as little verbose as possible. It's simply an wrapper
/// around the create methods of `ParserIR`.
///
/// This builder allocates memory on the arena in most method calls,
/// therefore it is discouraged, and in some cases disallowed, to
/// replace attributes to avoid users inadvertently allocating more
/// memory than necessary.
class ParserIR::Builder
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

    /// Sets (or unsets) the instruction in construction to define the
    /// specified label.
    auto label(const LabelDef* label_ptr) -> Builder&&;

    /// Sets the instruction in construction to define a label.
    auto label(std::string_view name, SourceRange source = no_source)
            -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    ///
    /// No other instruction to construct commands should have been called
    /// before this (i.e. only `Builder::label` could have been called).
    auto command(const Command* command_ptr) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    auto command(std::string_view name, SourceRange source = no_source)
            -> Builder&&;

    /// Sets the not flag of the command being constructed.
    auto not_flag(bool not_flag_value = true) -> Builder&&;

    /// Appends the given argument to the command in construction.
    auto arg(const Argument* value) -> Builder&&;

    /// Appends the given integer argument to the command in construction.
    auto arg_int(int32_t value, SourceRange source = no_source) -> Builder&&;

    /// Appends the given float argument to the command in construction.
    auto arg_float(float value, SourceRange source = no_source) -> Builder&&;

    /// Appends the given identifier argument to the command in construction.
    auto arg_ident(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends the given filename argument to the command in construction.
    auto arg_filename(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

    /// Appends the given string argument to the command in construction.
    auto arg_string(std::string_view value, SourceRange source = no_source)
            -> Builder&&;

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
    auto build() && -> ArenaPtr<ParserIR>;

    /// Builds a command from the attributes in the builder.
    auto build_command() && -> ArenaPtr<const ParserIR::Command>;

private:
    /// Consumes some of the attributes in this builder and transforms it
    /// into an `Command` by setting `command_ptr`.
    void create_command_from_attributes();

private:
    ArenaAllocator<> allocator;

    bool has_command_name = false;
    bool has_not_flag = false;
    bool not_flag_value = false;

    const LabelDef* label_ptr{};
    const Command* command_ptr{};

    std::string_view command_name;
    SourceRange command_source;

    size_t args_hint = -1;
    size_t args_capacity = 0;
    util::span<const Argument*> args;
};

template<typename InputIterator>
auto ParserIR::Builder::with_args(InputIterator begin, InputIterator end)
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
inline auto visit(Visitor&& vis, const ParserIR::Argument& arg,
                  OtherArgs&&... other_args)
{
    switch(arg.type())
    {
        using Type = ParserIR::Argument::Type;
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
        case Type::IDENTIFIER:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_identifier(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::FILENAME:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_filename(),
                               std::forward<OtherArgs>(other_args)...);
        }
        case Type::STRING:
        {
            return util::visit(std::forward<Visitor>(vis), util::visit_expand,
                               *arg.as_string(),
                               std::forward<OtherArgs>(other_args)...);
        }
        default:
            assert(false);
    }
}

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<ParserIR>);
static_assert(std::is_trivially_destructible_v<ParserIR::Command>);
static_assert(std::is_trivially_destructible_v<ParserIR::LabelDef>);
static_assert(std::is_trivially_destructible_v<ParserIR::Argument>);
} // namespace gta3sc

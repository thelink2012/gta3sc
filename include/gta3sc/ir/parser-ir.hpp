#pragma once
#include <cstdint>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena-allocator.hpp>
#include <gta3sc/util/intrusive-bidirectional-list-node.hpp>
#include <gta3sc/util/span.hpp>
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
/// and the node pointers present in this structure have interior mutability.
class ParserIR : public util::IntrusiveBidirectionalListNode<ParserIR>
{
public:
    struct LabelDef;
    struct Command;
    struct Argument;
    class Builder;

public:
    arena_ptr<const LabelDef> const label{};
    arena_ptr<const Command> const command{};

public:
    ParserIR() noexcept = delete;
    ~ParserIR() noexcept = default;

    ParserIR(const ParserIR&) = delete;
    auto operator=(const ParserIR&) -> ParserIR& = delete;

    ParserIR(ParserIR&&) noexcept = delete;
    auto operator=(ParserIR&&) noexcept -> ParserIR& = delete;

    // Creates an instruction.
    static auto create(arena_ptr<const LabelDef> label,
                       arena_ptr<const Command> command,
                       ArenaMemoryResource* arena) -> arena_ptr<ParserIR>;

    /// Creates an integer argument.
    static auto create_integer(int32_t value, SourceRange source,
                               ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates an identifier argument.
    ///
    /// The identifier is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_identifier(std::string_view name, SourceRange source,
                                  ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a filename argument.
    ///
    /// The filename is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_filename(std::string_view filename, SourceRange source,
                                ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view string, SourceRange source,
                              ArenaMemoryResource* arena)
            -> arena_ptr<const Argument>;

    /// Compares whether a given IR is equivalent to another IR.
    friend auto operator==(const ParserIR& lhs, const ParserIR& rhs) -> bool;
    friend auto operator!=(const ParserIR& lhs, const ParserIR& rhs) -> bool;

    struct Command
    {
    public:
        SourceRange source;    ///< Source code location of the argument.
        std::string_view name; ///< The name of this command.
        util::span<arena_ptr<const Argument>>
                args;          ///< View into the arguments.
        bool not_flag = false; ///< Whether the result of the command is NOTed.

        /// Please use `ParserIR::Builder::build_command`.
        Command() noexcept = delete;
        ~Command() noexcept = default;

        Command(const Command&) = delete;
        auto operator=(const Command&) -> Command& = delete;

        Command(Command&&) noexcept = delete;
        auto operator=(Command&&) noexcept -> Command& = delete;

        /// Compares whether a given command is equivalent to another command.
        friend auto operator==(const Command& lhs, const Command& rhs) -> bool;
        friend auto operator!=(const Command& lhs, const Command& rhs) -> bool;

    protected:
        Command(SourceRange source, std::string_view name,
                util::span<arena_ptr<const Argument>> args, bool not_flag) :
            source(source), name(name), args(args), not_flag(not_flag)
        {}

        friend class ParserIR::Builder;
    };

    struct LabelDef
    {
    public:
        SourceRange source;
        std::string_view name;

        /// Please use `LabelDef::create`.
        LabelDef() noexcept = delete;
        ~LabelDef() noexcept = default;

        LabelDef(const LabelDef&) = delete;
        auto operator=(const LabelDef&) -> LabelDef& = delete;

        LabelDef(LabelDef&&) noexcept = delete;
        auto operator=(LabelDef&&) noexcept -> LabelDef& = delete;

        /// Creates a label definition.
        ///
        /// The name of the label is automatically made uppercase.
        static auto create(std::string_view name, SourceRange source,
                           ArenaMemoryResource* arena)
                -> arena_ptr<const LabelDef>;

        /// Compares whether a given label definition is equivalent to another.
        friend auto operator==(const LabelDef& lhs, const LabelDef& rhs)
                -> bool;
        friend auto operator!=(const LabelDef& lhs, const LabelDef& rhs)
                -> bool;

    private:
        explicit LabelDef(SourceRange source, std::string_view name) :
            source(source), name(name)
        {}
    };

    /// Arguments are immutable and may be shared by multiple commands.
    struct Argument
    {
    public:
        /// Source code location of the argument.
        SourceRange source; // NOLINT: FIXME

        /// Please use `ParserIR` creation methods.
        Argument() noexcept = delete;
        ~Argument() noexcept = default;

        Argument(const Argument&) = delete;
        auto operator=(const Argument&) -> Argument& = delete;

        Argument(Argument&&) noexcept = delete;
        auto operator=(Argument&&) noexcept -> Argument& = delete;

        /// Returns the contained integer or `nullptr` if this argument is not
        /// an integer.
        auto as_integer() const -> const int32_t*;

        /// Returns the contained float or `nullptr` if this argument is not
        /// an float.
        auto as_float() const -> const float*;

        /// Returns the contained identifier or `nullptr` if this argument is
        /// not an identifier.
        auto as_identifier() const -> const std::string_view*;

        /// Returns the contained filename or `nullptr` if this argument is not
        /// an filename.
        auto as_filename() const -> const std::string_view*;

        /// Returns the contained string or `nullptr` if this argument is not
        /// an string.
        auto as_string() const -> const std::string_view*;

        /// Returns whether the value of this is equal the value of another
        /// argument (i.e. same as `operator==` without comparing source
        /// location).
        auto is_same_value(const Argument& other) const -> bool;

        /// Compares whether a given argument is equivalent to another.
        friend auto operator==(const Argument& lhs, const Argument& rhs)
                -> bool;
        friend auto operator!=(const Argument& lhs, const Argument& rhs)
                -> bool;

    protected:
        enum class IdentifierTag
        {
        };
        enum class FilenameTag
        {
        };
        enum class StringTag
        {
        };

        // Tagging adds one word of overhead to the memory used by an
        // argument, but is cleaner than a EqualityComparable wrapper.
        using Identifier = std::pair<IdentifierTag, std::string_view>;
        using Filename = std::pair<FilenameTag, std::string_view>;
        using String = std::pair<StringTag, std::string_view>;

        template<typename T>
        explicit Argument(T&& value, SourceRange source) :
            source(source), value(std::forward<T>(value))
        {}

        template<typename Tag>
        explicit Argument(Tag tag, std::string_view value, SourceRange source) :
            source(source), value(std::pair{tag, value})
        {}

        friend class ParserIR;

    private:
        const std::variant<int32_t, float, Identifier, Filename, String> value;

        // Keep the size of this structure small.
        // This would improve if we used a strong typedef over std::pair.
        static_assert(sizeof(value) <= 4 * sizeof(void*));
    };

private:
    explicit ParserIR(arena_ptr<const LabelDef> label,
                      arena_ptr<const Command> command) :
        label(label), command(command)
    {}
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
    explicit Builder(ArenaMemoryResource& arena) noexcept : arena(&arena) {}

    Builder(const Builder&) = delete;
    auto operator=(const Builder&) -> Builder& = delete;

    Builder(Builder&&) noexcept = default;
    auto operator=(Builder&&) noexcept -> Builder& = default;

    ~Builder() noexcept = default;

    /// Sets (or unsets) the instruction in construction to define the
    /// specified label.
    auto label(arena_ptr<const LabelDef> label_ptr) -> Builder&&;

    /// Sets the instruction in construction to define a label.
    auto label(std::string_view name, SourceRange source = no_source)
            -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    ///
    /// No other instruction to construct commands should have been called
    /// before this (i.e. only `Builder::label` could have been called).
    auto command(arena_ptr<const Command> command_ptr) -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    auto command(std::string_view name, SourceRange source = no_source)
            -> Builder&&;

    /// Sets the not flag of the command being constructed.
    auto not_flag(bool not_flag_value = true) -> Builder&&;

    /// Appends the given argument to the command in construction.
    auto arg(arena_ptr<const Argument> value) -> Builder&&;

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

    /// Builds an instruction from the attributes in the builder.
    auto build() && -> arena_ptr<ParserIR>;

    /// Builds a command from the attributes in the builder.
    auto build_command() && -> arena_ptr<const ParserIR::Command>;

private:
    /// Consumes some of the attributes in this builder and transforms it
    /// into an `Command` by setting `command_ptr`.
    void create_command_from_attributes();

private:
    ArenaMemoryResource* arena;

    bool has_command_name = false;
    bool has_not_flag = false;
    bool not_flag_value = false;

    arena_ptr<const LabelDef> label_ptr{};
    arena_ptr<const Command> command_ptr{};

    std::string_view command_name;
    SourceRange command_source;

    size_t args_capacity = 0;
    util::span<arena_ptr<const Argument>> args;
};

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<ParserIR>);
static_assert(std::is_trivially_destructible_v<ParserIR::Command>);
static_assert(std::is_trivially_destructible_v<ParserIR::LabelDef>);
static_assert(std::is_trivially_destructible_v<ParserIR::Argument>);
} // namespace gta3sc

// TODO use some kind of strong_typedef instead of std::pair in Argument

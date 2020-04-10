#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <gta3sc/adt/span.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena-allocator.hpp>
#include <string_view>
#include <variant>

namespace gta3sc
{
/// This is an intermediate representation for syntactically
/// valid GTA3script.
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
/// VAR_INT, ENDWHILE) might not follow it (e.g. the first argument of
/// GOSUB_FILE might not be an identifier). The amount of arguments might be
/// wrong as well (e.g. ENDWHILE with two arguments).
///
/// This IR is immutable and preserves source code information such as the
/// location of each of its identifiers.
class ParserIR
{
public:
    struct LabelDef;
    struct Command;
    struct Argument;
    class Builder;

public:
    arena_ptr<const LabelDef> const label = nullptr;
    arena_ptr<const Command> const command = nullptr;

public:
    ParserIR() = delete;

    ParserIR(const ParserIR&) = delete;
    ParserIR& operator=(const ParserIR&) = delete;

    ParserIR(ParserIR&&) = delete;
    ParserIR& operator=(ParserIR&&) = delete;

    // Creates an instruction.
    static auto create(arena_ptr<const LabelDef>, arena_ptr<const Command>,
                       ArenaMemoryResource*) -> arena_ptr<ParserIR>;

    /// Creates an integer argument.
    static auto create_integer(int32_t value, SourceRange source,
                               ArenaMemoryResource*)
            -> arena_ptr<const Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaMemoryResource*) -> arena_ptr<const Argument>;

    /// Creates an identifier argument.
    ///
    /// The identifier is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_identifier(std::string_view value, SourceRange source,
                                  ArenaMemoryResource*)
            -> arena_ptr<const Argument>;

    /// Creates a filename argument.
    ///
    /// The filename is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_filename(std::string_view filename, SourceRange source,
                                ArenaMemoryResource*)
            -> arena_ptr<const Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view string, SourceRange source,
                              ArenaMemoryResource*)
            -> arena_ptr<const Argument>;

    struct Command
    {
    public:
        SourceRange source;    ///< Source code location of the argument.
        std::string_view name; ///< The name of this command.
        adt::span<arena_ptr<const Argument>> args; ///< View into the arguments.
        bool not_flag = false; ///< Whether the result of the command is NOTed.

        /// Please use `ParserIR::Builder::build_command`.
        Command() = delete;

    protected:
        explicit Command(SourceRange source, std::string_view name,
                         adt::span<arena_ptr<const Argument>> args,
                         bool not_flag) :
            source(source),
            name(name),
            args(std::move(args)),
            not_flag(not_flag)
        {}

        friend class ParserIR::Builder;
    };

    struct LabelDef
    {
    public:
        SourceRange source;
        std::string_view name;

        /// Please use `LabelDef::create`.
        LabelDef() = delete;

        /// Creates a label definition.
        ///
        /// The name of the label is automatically made uppercase.
        static auto create(std::string_view name, SourceRange source,
                           ArenaMemoryResource*) -> arena_ptr<const LabelDef>;

    private:
        explicit LabelDef(SourceRange source, std::string_view name) :
            source(source), name(name)
        {}
    };

    /// Arguments are immutable and may be shared by multiple commands.
    struct Argument
    {
    public:
        SourceRange source; ///< Source code location of the argument.

        /// Please use `ParserIR` creation methods.
        Argument() = delete;

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
        /// argument.
        bool is_same_value(const Argument& other) const;

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
    };

protected:
    friend struct Command;
    friend struct LabelDef;
    friend struct Argument;

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
    explicit Builder(ArenaMemoryResource& arena) : arena(&arena) {}

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder(Builder&&) = default;
    Builder& operator=(Builder&&) = default;

    /// Sets the instruction in construction to define the specified label.
    auto label(arena_ptr<const LabelDef>) -> Builder&&;

    /// Sets the instruction in construction to define a label.
    auto label(std::string_view name, SourceRange source = no_source)
            -> Builder&&;

    /// Sets the instruction in construction to be the specified command.
    ///
    /// No other instruction to construct commands should have been called
    /// before this (i.e. only `Builder::label` could have been called).
    auto command(arena_ptr<const Command>) -> Builder&&;

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
    adt::span<arena_ptr<const Argument>> args;
};

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<ParserIR>);
static_assert(std::is_trivially_destructible_v<ParserIR::Command>);
static_assert(std::is_trivially_destructible_v<ParserIR::LabelDef>);
static_assert(std::is_trivially_destructible_v<ParserIR::Argument>);

inline auto ParserIR::create(arena_ptr<const LabelDef> label,
                             arena_ptr<const Command> command,
                             ArenaMemoryResource* arena) -> arena_ptr<ParserIR>
{
    return new(*arena, alignof(ParserIR)) ParserIR(label, command);
}

inline auto ParserIR::create_integer(int32_t value, SourceRange source,
                                     ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(value, source);
}

inline auto ParserIR::create_float(float value, SourceRange source,
                                   ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(value, source);
}

inline auto ParserIR::create_identifier(std::string_view name,
                                        SourceRange source,
                                        ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::IdentifierTag{},
                           util::allocate_string_upper(name, *arena), source);
}

inline auto ParserIR::create_filename(std::string_view name, SourceRange source,
                                      ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::FilenameTag{},
                           util::allocate_string_upper(name, *arena), source);
}

inline auto ParserIR::create_string(std::string_view string, SourceRange source,
                                    ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::StringTag{}, string, source);
}

inline auto ParserIR::LabelDef::create(std::string_view name,
                                       SourceRange source,
                                       ArenaMemoryResource* arena)
        -> arena_ptr<const LabelDef>
{
    return new(*arena, alignof(LabelDef))
            const LabelDef{source, util::allocate_string_upper(name, *arena)};
}

inline auto ParserIR::Argument::as_integer() const -> const int32_t*
{
    return std::get_if<int32_t>(&this->value);
}

inline auto ParserIR::Argument::as_float() const -> const float*
{
    return std::get_if<float>(&this->value);
}

inline auto ParserIR::Argument::as_identifier() const -> const std::string_view*
{
    if(auto ident = std::get_if<Identifier>(&this->value))
        return &ident->second;
    return nullptr;
}

inline auto ParserIR::Argument::as_filename() const -> const std::string_view*
{
    if(auto fi = std::get_if<Filename>(&this->value))
        return &fi->second;
    return nullptr;
}

inline auto ParserIR::Argument::as_string() const -> const std::string_view*
{
    if(auto s = std::get_if<String>(&this->value))
        return &s->second;
    return nullptr;
}

inline bool ParserIR::Argument::is_same_value(const Argument& other) const
{
    return this->value == other.value;
}

inline auto ParserIR::Builder::label(arena_ptr<const LabelDef> label_ptr)
        -> Builder&&
{
    this->label_ptr = label_ptr;
    return std::move(*this);
}

inline auto ParserIR::Builder::label(std::string_view name, SourceRange source)
        -> Builder&&
{
    return this->label(LabelDef::create(name, source, arena));
}

inline auto ParserIR::Builder::command(arena_ptr<const Command> command_ptr)
        -> Builder&&
{
    assert(!this->has_not_flag && !this->has_command_name
           && this->args.empty());
    this->command_ptr = command_ptr;
    return std::move(*this);
}

inline auto ParserIR::Builder::command(std::string_view name,
                                       SourceRange source) -> Builder&&
{
    this->command_ptr = nullptr;
    this->has_command_name = true;
    this->command_name = name;
    this->command_source = source;
    return std::move(*this);
}

inline auto ParserIR::Builder::not_flag(bool not_flag_value) -> Builder&&
{
    this->has_not_flag = true;
    this->not_flag_value = not_flag_value;
    return std::move(*this);
}

inline auto ParserIR::Builder::arg(arena_ptr<const Argument> value) -> Builder&&
{
    assert(value != nullptr);

    if(this->args.size() >= static_cast<std::ptrdiff_t>(args_capacity))
    {
        const auto new_caps = !args_capacity ? 6 : args_capacity * 2;
        const auto new_args = new(*arena) arena_ptr<const Argument>[new_caps];
        std::move(this->args.begin(), this->args.end(), new_args);

        this->args = adt::span(new_args, args.size());
        this->args_capacity = new_caps;
    }

    this->args = adt::span(this->args.data(), this->args.size() + 1);
    *(this->args.rbegin()) = value;

    return std::move(*this);
}

inline auto ParserIR::Builder::arg_int(int32_t value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_integer(value, source, arena));
}

inline auto ParserIR::Builder::arg_float(float value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_float(value, source, arena));
}

inline auto ParserIR::Builder::arg_ident(std::string_view value,
                                         SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_identifier(value, source, arena));
}

inline auto ParserIR::Builder::arg_filename(std::string_view value,
                                            SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_filename(value, source, arena));
}

inline auto ParserIR::Builder::arg_string(std::string_view value,
                                          SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_string(value, source, arena));
}

inline auto ParserIR::Builder::build() && -> arena_ptr<ParserIR>
{
    if(this->has_command_name)
    {
        this->create_command_from_attributes();
        assert(this->command_ptr != nullptr);
    }
    else
    {
        assert(!this->has_not_flag && this->args.empty());
    }
    return ParserIR::create(this->label_ptr, this->command_ptr, arena);
}

inline auto
ParserIR::Builder::build_command() && -> arena_ptr<const ParserIR::Command>
{
    if(this->has_command_name)
    {
        this->create_command_from_attributes();
        assert(this->command_ptr != nullptr);
    }
    else
    {
        assert(!this->has_not_flag && this->args.empty());
    }
    return this->command_ptr;
}

inline void ParserIR::Builder::create_command_from_attributes()
{
    assert(this->has_command_name);

    this->command_ptr = new(*arena, alignof(Command)) const Command{
            this->command_source,
            util::allocate_string_upper(this->command_name, *arena),
            std::move(this->args),
            this->has_not_flag ? this->not_flag_value : false};

    this->has_command_name = false;
    this->has_not_flag = false;
}
}

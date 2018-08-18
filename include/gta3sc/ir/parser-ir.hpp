#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <gta3sc/adt/span.hpp>
#include <gta3sc/arena-allocator.hpp>
#include <gta3sc/sourceman.hpp>
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
/// This IR preserves source code information such as the location of
/// each of its identifiers.
class ParserIR
{
public:
    struct LabelDef;
    struct Command;
    struct Argument;

public:
    arena_ptr<LabelDef> label = nullptr;
    arena_ptr<Command> command = nullptr;

public:
    ParserIR() = delete;

    ParserIR(const ParserIR&) = delete;
    ParserIR& operator=(const ParserIR&) = delete;

    ParserIR(ParserIR&&) = delete;
    ParserIR& operator=(ParserIR&&) = delete;

    /// Creates an empty instruction.
    static auto create(ArenaMemoryResource*) -> arena_ptr<ParserIR>;

    /// Creates an instruction containing a command.
    ///
    /// The name of the command is automatically converted to uppercase
    /// during the creation of the object.
    static auto create_command(std::string_view name, SourceRange source,
                               ArenaMemoryResource*) -> arena_ptr<ParserIR>;

    /// Creates an integer argument.
    static auto create_integer(int32_t value, SourceRange source,
                               ArenaMemoryResource*) -> arena_ptr<Argument>;

    /// Creates a floating-point argument.
    static auto create_float(float value, SourceRange source,
                             ArenaMemoryResource*) -> arena_ptr<Argument>;

    /// Creates an identifier argument.
    ///
    /// The identifier is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_identifier(std::string_view value, SourceRange source,
                                  ArenaMemoryResource*) -> arena_ptr<Argument>;

    /// Creates a filename argument.
    ///
    /// The filename is automatically converted to uppercase during the
    /// creation of the object.
    static auto create_filename(std::string_view filename, SourceRange source,
                                ArenaMemoryResource*) -> arena_ptr<Argument>;

    /// Creates a string argument.
    ///
    /// The quotation marks that surrounds the string should not be present
    /// in `string`. The string is not converted to uppercase.
    static auto create_string(std::string_view string, SourceRange source,
                              ArenaMemoryResource*) -> arena_ptr<Argument>;

    struct Command
    {
    public:
        const SourceRange source;    ///< Source code location of the argument.
        const std::string_view name; ///< The name of this command.
        adt::span<arena_ptr<Argument>> args; ///< View into the arguments.
        bool not_flag = false; ///< Whether the result of the command is NOTed.

        Command() = delete;

        /// Creates a command instance.
        ///
        /// This is useful over `ParserIR::create_command` to replace the
        /// command in an already existing IR instruction.
        //
        /// The name of the command is automatically made uppercase.
        static auto create(std::string_view name, SourceRange source,
                           ArenaMemoryResource*) -> arena_ptr<Command>;

        /// Adds a new argument to the command.
        void push_arg(arena_ptr<Argument>, ArenaMemoryResource*);

    private:
        size_t args_capacity = 0;

        explicit Command(SourceRange source, std::string_view name) :
            source(source),
            name(name)
        {}
    };

    struct LabelDef
    {
    public:
        const SourceRange source;
        const std::string_view name;

        LabelDef() = delete;

        /// Creates a label definition.
        ///
        /// The definition should be assigned to a command by replacing
        /// the `ParserIR::label` variable of an IR instruction.
        ///
        /// The name of the label is automatically made uppercase.
        static auto create(std::string_view name, SourceRange source,
                           ArenaMemoryResource*) -> arena_ptr<LabelDef>;

    private:
        explicit LabelDef(SourceRange source, std::string_view name) :
            source(source),
            name(name)
        {}
    };

    /// Arguments are immutable and may be shared by multiple commands.
    struct Argument
    {
    public:
        const SourceRange source; ///< Source code location of the argument.

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
            source(source),
            value(std::forward<T>(value))
        {}

        template<typename Tag>
        explicit Argument(Tag tag, std::string_view value, SourceRange source) :
            source(source),
            value(std::pair{tag, value})
        {}

        const std::variant<int32_t, float, Identifier, Filename, String> value;

        friend class ParserIR;
    };


protected:
    friend struct Command;
    friend struct LabelDef;
    friend struct Argument;

    static auto create_chars(std::string_view from, ArenaMemoryResource*)
            -> std::string_view;

    static auto create_chars_upper(std::string_view from, ArenaMemoryResource*)
            -> std::string_view;

private:
    explicit ParserIR(arena_ptr<Command> command) : command(command) {}

};

// These resources are stored in a memory arena. Disposing storage
// **must** be enough for destruction of these objects.
static_assert(std::is_trivially_destructible_v<ParserIR>);
static_assert(std::is_trivially_destructible_v<ParserIR::Command>);
static_assert(std::is_trivially_destructible_v<ParserIR::LabelDef>);
static_assert(std::is_trivially_destructible_v<ParserIR::Argument>);

inline auto ParserIR::create(ArenaMemoryResource* arena) -> arena_ptr<ParserIR>
{
    return new(*arena, alignof(ParserIR)) ParserIR(nullptr);
}

inline auto ParserIR::create_command(std::string_view name, SourceRange source,
                                     ArenaMemoryResource* arena)
        -> arena_ptr<ParserIR>
{
    auto command = Command::create(name, source, arena);
    return new(*arena, alignof(ParserIR)) ParserIR(command);
}

inline auto ParserIR::create_integer(int32_t value, SourceRange source,
                                     ArenaMemoryResource* arena)
        -> arena_ptr<Argument>
{
    return new(*arena, alignof(Argument)) Argument(value, source);
}

inline auto ParserIR::create_float(float value, SourceRange source,
                                   ArenaMemoryResource* arena)
        -> arena_ptr<Argument>
{
    return new(*arena, alignof(Argument)) Argument(value, source);
}

inline auto ParserIR::create_identifier(std::string_view name,
                                        SourceRange source,
                                        ArenaMemoryResource* arena)
        -> arena_ptr<Argument>
{
    return new(*arena, alignof(Argument)) Argument(
            Argument::IdentifierTag{}, create_chars_upper(name, arena), source);
}

inline auto ParserIR::create_filename(std::string_view name, SourceRange source,
                                      ArenaMemoryResource* arena)
        -> arena_ptr<Argument>
{
    return new(*arena, alignof(Argument)) Argument(
            Argument::FilenameTag{}, create_chars_upper(name, arena), source);
}

inline auto ParserIR::create_string(std::string_view string, SourceRange source,
                                    ArenaMemoryResource* arena)
        -> arena_ptr<Argument>
{
    return new(*arena, alignof(Argument))
            Argument(Argument::StringTag{}, string, source);
}

inline auto ParserIR::create_chars(std::string_view from,
                                   ArenaMemoryResource* arena)
        -> std::string_view
{
    auto ptr = new(*arena, alignof(char)) char[from.size()];
    std::memcpy(ptr, from.data(), from.size());
    return {ptr, from.size()};
}

inline auto ParserIR::create_chars_upper(std::string_view from,
                                         ArenaMemoryResource* arena)
        -> std::string_view
{
    auto chars = create_chars(from, arena);
    for(auto& c : chars)
    {
        if(c >= 'a' && c <= 'z')
            const_cast<char&>(c) = c - 32;
    }
    return chars;
}

inline auto ParserIR::Command::create(std::string_view name, SourceRange source,
                                      ArenaMemoryResource* arena)
        -> arena_ptr<Command>
{
    return new(*arena, alignof(Command))
            Command{source, create_chars_upper(name, arena)};
}

inline auto ParserIR::LabelDef::create(std::string_view name,
                                       SourceRange source,
                                       ArenaMemoryResource* arena)
        -> arena_ptr<LabelDef>
{
    return new(*arena, alignof(LabelDef))
            LabelDef{source, create_chars_upper(name, arena)};
}

inline void ParserIR::Command::push_arg(arena_ptr<Argument> arg,
                                        ArenaMemoryResource* arena)
{
    assert(arg != nullptr);

    if(this->args.size() >= args_capacity)
    {
        auto new_caps = !args_capacity ? 6 : args_capacity * 2;
        auto new_args = new(*arena) arena_ptr<Argument>[new_caps];
        std::move(args.begin(), args.end(), new_args);

        this->args = adt::span(new_args, args.size());
        this->args_capacity = new_caps;
    }

    this->args = adt::span(args.data(), args.size() + 1);
    *(this->args.rbegin()) = arg;
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
}

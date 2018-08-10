#pragma once
#include <gta3sc/adt/span.hpp>
#include <gta3sc/arena-allocator.hpp>
#include <cstdint>
#include <cstring>
#include <variant>
#include <string_view>

#include <algorithm> // TODO take me off please!

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
/// VAR_INT) might not follow it (e.g. the first argument of GOSUB_FILE might
/// not be an identifier). The amount of arguments might be wrong as well.
///
/// This IR preserves source code information such as the location of
/// each of its identifiers.
struct ParserIR
{
    struct SourceInfo
    {
        const SourceFile& source;
        SourceRange range;
    };


    struct Identifier
    {
        std::string_view name;
        constexpr operator std::string_view() const { return name; }
    };

    struct Filename
    {
        std::string_view filename;
        constexpr operator std::string_view() const { return filename; }
    };

    struct String
    {
        std::string_view string;
        constexpr operator std::string_view() const { return string; }
    };

    struct Argument
    {
        arena_ptr<SourceInfo> source_info;
        std::variant<
            int32_t,
            float,
            Identifier,
            Filename,
            String> value;

        constexpr int32_t* as_integer() { return std::get_if<int32_t>(&value); }
        constexpr float* as_float() { return std::get_if<float>(&value); }
        constexpr Identifier* as_identifier() { return std::get_if<Identifier>(&value); }
        constexpr Filename* as_filename() { return std::get_if<Filename>(&value); }
        constexpr String* as_string() { return std::get_if<String>(&value); }
    };


    struct LabelDef
    {
        arena_ptr<SourceInfo> source_info;
        std::string_view name;

        static auto create(const SourceInfo& info, std::string_view name_a, ArenaMemoryResource& arena) -> arena_ptr<LabelDef>
        {
            auto name = create_upper_view(name_a, arena);
            return new (arena, alignof(LabelDef)) LabelDef { create_source_info(info, arena), name };
        }
    };

    struct Command
    {
        arena_ptr<SourceInfo> source_info;
        std::string_view name;
        adt::span<arena_ptr<Argument>> args;
        size_t acaps = 0;

        static auto create(const SourceInfo& info, std::string_view name_a, ArenaMemoryResource& arena) -> arena_ptr<Command>
        {
            auto name = create_upper_view(name_a, arena);
            return new (arena, alignof(Command)) Command { create_source_info(info, arena), name };
        }

        void push_arg(arena_ptr<Argument> arg, ArenaMemoryResource& arena)
        {
            assert(arg != nullptr);
            if(args.size() >= acaps)
            {
                auto new_caps = !acaps? 6 : acaps * 2;
                auto new_args = new (arena) arena_ptr<ParserIR::Argument>[new_caps];
                std::copy(args.begin(), args.end(), new_args);
                acaps = new_caps;
                args = adt::span(new_args, args.size());
            }
            args = adt::span(args.data(), args.data() + args.size() + 1);
            *args.rbegin() = arg;
        }
    };

    arena_ptr<LabelDef> label = nullptr;
    arena_ptr<Command> command = nullptr;
    arena_ptr<ParserIR> next = nullptr;
    arena_ptr<ParserIR> prev = nullptr;

    // use set_next for set_prev behaviour
    void set_next(arena_ptr<ParserIR> other)
    {
        assert(other != nullptr);

        if(this->next)
        {
            assert(this->next->prev == this);
            this->next->prev = nullptr;
        }

        this->next = other;

        if(other->prev)
        {
            assert(other->prev->next == other);
            other->prev->next = nullptr;
        }

        other->prev = this;
    }

    static auto create(ArenaMemoryResource& arena) -> arena_ptr<ParserIR>
    {
        return new (arena, alignof(ParserIR)) ParserIR;
    }

    static auto create_command(const SourceInfo& info, std::string_view name_a, ArenaMemoryResource& arena) -> arena_ptr<ParserIR>
    {
        auto ir = create(arena);
        ir->command = Command::create(info, name_a, arena);
        return ir;
    }

    static auto create_source_info(const SourceInfo& info, ArenaMemoryResource& arena) -> arena_ptr<SourceInfo>
    {
        return new (arena, alignof(SourceInfo)) SourceInfo(info);
    }

    static auto create_integer(const SourceInfo& info, int32_t value_,
                              ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        decltype(Argument::value) value = value_;
        arena_ptr<Argument> arg_ptr = new (arena, alignof(Argument)) Argument { create_source_info(info, arena), value };
        return arg_ptr;
    }

    static auto create_float(const SourceInfo& info, float value_,
                           ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = value_;
        return arg;
    }

    static auto create_identifier(const SourceInfo& info,
                                std::string_view name,
                                ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = Identifier { create_upper_view(name, arena) };
        return arg;
    }

    static auto create_filename(const SourceInfo& info,
                                std::string_view name,
                                ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = Filename { create_upper_view(name, arena) };
        return arg;
    }

    static auto create_string(const SourceInfo& info,
                            std::string_view string,
                            ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = String { create_string_view(string, arena) };
        return arg;
    }

    static auto create_upper_view(std::string_view from, ArenaMemoryResource& arena)
        -> std::string_view
    {
        auto ptr = (char*) arena.allocate(from.size(), alignof(char));
        std::transform(from.begin(), from.end(), ptr, [](unsigned char c) {
            return c >= 'a' && c <= 'z'? (c - 32) : c;
        });
        return std::string_view(ptr, from.size());
    }

    static auto create_string_view(std::string_view from, ArenaMemoryResource& arena)
        -> std::string_view
    {
        auto ptr = (char*) arena.allocate(from.size(), alignof(char));
        std::memcpy(ptr, from.data(), from.size());
        return std::string_view(ptr, from.size());
    }
};
// TODO static_assert trivial destructible everything
}

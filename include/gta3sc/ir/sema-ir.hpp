#pragma once
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/command-manager.hpp>

// TODO this is in progress

namespace gta3sc
{
// TODO review whether we should be using this
template<typename T> // TODO remove this
using observer_ptr = T*;

class SemaIR
{
public:
    struct Command;
    struct Argument;

    observer_ptr<SymLabel> label = nullptr;
    arena_ptr<Command> command = nullptr;

    struct Command
    {
    public:
        const SourceRange source;
        const CommandManager::CommandDef& def;
        adt::span<arena_ptr<Argument>> args;
        bool not_flag = false;

        static auto create(const CommandManager::CommandDef& def,
                           SourceRange source, ArenaMemoryResource* arena)
        {
            return new(*arena, alignof(Command)) Command(def, source);
        }

        void reserve_args(size_t count, ArenaMemoryResource* arena)
        {
            if(count < args_capacity)
                return;

            auto new_caps = count;
            auto new_args = new(*arena) arena_ptr<Argument>[new_caps];
            std::move(args.begin(), args.end(), new_args);

            this->args = adt::span(new_args, args.size());
            this->args_capacity = new_caps;
        }

        void push_arg(arena_ptr<Argument> arg, ArenaMemoryResource* arena)
        {
            assert(arg != nullptr);

            if(this->args.size() >= args_capacity)
            {
                const auto new_caps = !args_capacity ? 6 : args_capacity * 2;
                reserve_args(new_caps, arena);
            }

            this->args = adt::span(args.data(), args.size() + 1);
            *(this->args.rbegin()) = arg;
        }

        Command() = delete;

    private:
        size_t args_capacity = 0;

        Command(const CommandManager::CommandDef& def, SourceRange source) :
            source(source),
            def(def)
        {}
    };

    struct Argument
    {
    public:
        struct VariableRef
        {
            observer_ptr<SymVariable> def; // TODO maybe SymVariable&
            std::variant<std::monostate, int32_t, observer_ptr<SymVariable>>
                    index;

            bool has_index() const 
            { return !std::holds_alternative<std::monostate>(index); }

            auto index_as_integer() const -> const int32_t*
            {
                return std::get_if<int32_t>(&this->index);
            }

            auto index_as_variable() const -> const SymVariable*
            {
                if(auto var = std::get_if<observer_ptr<SymVariable>>(&this->index))
                    return *var;
                return nullptr;
            }
        };

        struct TextLabelTag
        {
        };
        struct StringTag
        {
        };

        struct StringConstant
        {
            CommandManager::EnumId enum_id;
            int32_t value;
        };

        using TextLabel = std::pair<TextLabelTag, std::string_view>;
        using String = std::pair<StringTag, std::string_view>;
        const SourceRange source;

        const std::variant<
                int32_t, float, TextLabel, String, VariableRef,
                // TODO observer_ptr<SymFilename>,
                observer_ptr<SymLabel>,
                // TODO observer_ptr<SymConstantInt>,
                // TODO observer_ptr<SymConstantFloat>,
                // TODO observer_ptr<__HeaderModel>,
                // TODO observer_ptr<__StreamedScriptConstant>,
                StringConstant
                >
                value;

        Argument() = delete;

        auto as_integer() const -> const int32_t*
        {
            return std::get_if<int32_t>(&this->value);
        }

        auto as_float() const -> const float*
        {
            return std::get_if<float>(&this->value);
        }

        auto as_text_label() const -> const std::string_view*
        {
            if(auto text_label = std::get_if<TextLabel>(&this->value))
                return &text_label->second;
            return nullptr;
        }

        auto as_string() const -> const std::string_view*
        {
            if(auto string = std::get_if<String>(&this->value))
                return &string->second;
            return nullptr;
        }

        auto as_var_ref() const -> const VariableRef*
        {
            return std::get_if<VariableRef>(&this->value);
        }

        auto as_label() const -> SymLabel*
        {
            if(auto label = std::get_if<observer_ptr<SymLabel>>(&this->value))
                return *label;
            return nullptr;
        }

        auto as_string_constant() const -> const StringConstant*
        {
            return std::get_if<StringConstant>(&this->value);
        }

    protected:
        friend class SemaIR;

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
    };

    static auto create(ArenaMemoryResource* arena) -> arena_ptr<SemaIR>
    {
        return new(*arena, alignof(SemaIR)) SemaIR();
    }

    static auto create_integer(int32_t value, SourceRange source,
                               ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        return new(*arena, alignof(Argument)) Argument(value, source);
    }

    static auto create_float(float value, SourceRange source,
                             ArenaMemoryResource* arena) -> arena_ptr<Argument>
    {
        return new(*arena, alignof(Argument)) Argument(value, source);
    }

    static auto create_text_label(std::string_view value, SourceRange source,
                                  ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        return new(*arena, alignof(Argument))
                Argument(Argument::TextLabelTag{}, value, source);
    }

    static auto create_label(SymLabel* label, SourceRange source,
                                ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        assert(label != nullptr);
        return new(*arena, alignof(Argument))
                Argument(label, source);
    }

    static auto create_string(std::string_view value, SourceRange source,
                              ArenaMemoryResource* arena) -> arena_ptr<Argument>
    {
        return new(*arena, alignof(Argument))
                Argument(Argument::StringTag{}, value, source);
    }

    static auto create_variable(SymVariable* var, SourceRange source,
                                ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        assert(var != nullptr);
        return new(*arena, alignof(Argument))
                Argument(Argument::VariableRef{var}, source);
    }

    static auto create_variable(SymVariable* var, int32_t index,
                                SourceRange source, ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        assert(var != nullptr);
        return new(*arena, alignof(Argument))
                Argument(Argument::VariableRef{var, index}, source);
    }

    static auto create_variable(SymVariable* var, SymVariable* index,
                                SourceRange source, ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        assert(var != nullptr && index != nullptr);
        return new(*arena, alignof(Argument))
                Argument(Argument::VariableRef{var, index}, source);
    }

    static auto create_string_constant(CommandManager::EnumId enum_id, 
                                       int32_t value, SourceRange source,
                                       ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        return new(*arena, alignof(Argument))
                Argument(Argument::StringConstant{enum_id, value}, source);
    }

    static auto create_string_constant(const CommandManager::ConstantDef& cdef, 
                                       SourceRange source,
                                       ArenaMemoryResource* arena)
            -> arena_ptr<Argument>
    {
        return create_string_constant(cdef.enum_id, cdef.value, source, arena);
    }
};
}

#include <gta3sc/ir/sema-ir.hpp>

namespace gta3sc
{
auto SemaIR::create(const SymLabel* label, arena_ptr<const Command> command,
                    ArenaMemoryResource* arena) -> arena_ptr<SemaIR>
{
    return new(*arena, alignof(SemaIR)) SemaIR(label, command);
}

auto SemaIR::create_integer(int32_t value, SourceRange source,
                            ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) Argument(value, source);
}

auto SemaIR::create_float(float value, SourceRange source,
                          ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) Argument(value, source);
}

auto SemaIR::create_text_label(std::string_view value, SourceRange source,
                               ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::TextLabelTag{}, value, source);
}

auto SemaIR::create_label(const SymLabel& label, SourceRange source,
                          ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(&label, source);
}

auto SemaIR::create_string(std::string_view value, SourceRange source,
                           ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::StringTag{}, value, source);
}

auto SemaIR::create_variable(const SymVariable& var, SourceRange source,
                             ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::VarRef{var}, source);
}

auto SemaIR::create_variable(const SymVariable& var, int32_t index,
                             SourceRange source, ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::VarRef{var, index}, source);
}

auto SemaIR::create_variable(const SymVariable& var, const SymVariable& index,
                             SourceRange source, ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::VarRef{var, &index}, source);
}

auto SemaIR::create_constant(const CommandManager::ConstantDef& cdef,
                             SourceRange source, ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(&cdef, source);
}

auto SemaIR::create_used_object(const SymUsedObject& used_object,
                                SourceRange source, ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(&used_object, source);
}

auto SemaIR::Argument::as_integer() const -> const int32_t*
{
    return std::get_if<int32_t>(&this->value);
}

auto SemaIR::Argument::as_float() const -> const float*
{
    return std::get_if<float>(&this->value);
}

auto SemaIR::Argument::as_text_label() const -> const std::string_view*
{
    if(const auto* text_label = std::get_if<TextLabel>(&this->value))
        return &text_label->second;
    return nullptr;
}

auto SemaIR::Argument::as_string() const -> const std::string_view*
{
    if(const auto* string = std::get_if<String>(&this->value))
        return &string->second;
    return nullptr;
}

auto SemaIR::Argument::as_var_ref() const -> const VarRef*
{
    return std::get_if<VarRef>(&this->value);
}

auto SemaIR::Argument::as_label() const -> const SymLabel*
{
    if(const auto* label = std::get_if<const SymLabel*>(&this->value))
        return *label;
    return nullptr;
}

auto SemaIR::Argument::as_constant() const -> const CommandManager::ConstantDef*
{
    if(const auto* sconst = std::get_if<const CommandManager::ConstantDef*>(
               &this->value))
        return *sconst;
    return nullptr;
}

auto SemaIR::Argument::as_used_object() const -> const SymUsedObject*
{
    if(const auto* uobj = std::get_if<const SymUsedObject*>(&this->value))
        return *uobj;
    return nullptr;
}

auto SemaIR::Argument::pun_as_integer() const -> std::optional<int32_t>
{
    if(const auto* value = as_integer())
        return *value;
    else if(const auto* sconst = as_constant())
        return sconst->value;
    else
        return std::nullopt;
}

auto SemaIR::Argument::pun_as_float() const -> std::optional<float>
{
    if(const auto* value = as_float())
        return *value;
    else
        return std::nullopt;
}

auto SemaIR::Argument::VarRef::has_index() const -> bool
{
    return !std::holds_alternative<std::monostate>(index);
}

auto SemaIR::Argument::VarRef::index_as_integer() const -> const int32_t*
{
    return std::get_if<int32_t>(&this->index);
}

auto SemaIR::Argument::VarRef::index_as_variable() const -> const SymVariable*
{
    if(const auto* var = std::get_if<const SymVariable*>(&this->index))
        return *var;
    return nullptr;
}

auto SemaIR::Builder::label(const SymLabel* label_ptr) -> Builder&&
{
    this->label_ptr = label_ptr;
    return std::move(*this);
}

auto SemaIR::Builder::command(arena_ptr<const Command> command_ptr) -> Builder&&
{
    assert(!this->has_not_flag && !this->has_command_def && this->args.empty());
    this->command_ptr = command_ptr;
    return std::move(*this);
}

auto SemaIR::Builder::command(const CommandManager::CommandDef& command_def,
                              SourceRange source) -> Builder&&
{
    assert(!this->command_ptr && !this->has_command_def);
    this->command_ptr = nullptr;
    this->has_command_def = true;
    this->command_def = &command_def;
    this->command_source = source;
    return std::move(*this);
}

auto SemaIR::Builder::not_flag(bool not_flag_value) -> Builder&&
{
    this->has_not_flag = true;
    this->not_flag_value = not_flag_value;
    return std::move(*this);
}

auto SemaIR::Builder::arg(arena_ptr<const Argument> value) -> Builder&&
{
    assert(value != nullptr);

    if(this->args.size() >= static_cast<std::ptrdiff_t>(args_capacity))
    {
        const auto new_caps = !args_capacity ? 6 : args_capacity * 2;
        auto* const new_args = new(*arena) arena_ptr<const Argument>[new_caps];
        std::move(this->args.begin(), this->args.end(), new_args);

        this->args = util::span(new_args, args.size());
        this->args_capacity = new_caps;
    }

    this->args = util::span(this->args.data(), this->args.size() + 1);
    *(this->args.rbegin()) = value;

    return std::move(*this);
}

auto SemaIR::Builder::arg_int(int32_t value, SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_integer(value, source, arena));
}

auto SemaIR::Builder::arg_float(float value, SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_float(value, source, arena));
}

auto SemaIR::Builder::arg_label(const SymLabel& label, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_label(label, source, arena));
}

auto SemaIR::Builder::arg_text_label(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_text_label(value, source, arena));
}

auto SemaIR::Builder::arg_string(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_string(value, source, arena));
}

auto SemaIR::Builder::arg_var(const SymVariable& var, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_variable(var, source, arena));
}

auto SemaIR::Builder::arg_var(const SymVariable& var, int32_t index,
                              SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_variable(var, index, source, arena));
}

auto SemaIR::Builder::arg_var(const SymVariable& var, const SymVariable& index,
                              SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_variable(var, index, source, arena));
}

/// Appends an argument referencing to the given string constant to the
/// command in construction.
auto SemaIR::Builder::arg_const(const CommandManager::ConstantDef& cdef,
                                SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_constant(cdef, source, arena));
}

auto SemaIR::Builder::arg_object(const SymUsedObject& used_object,
                                 SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_used_object(used_object, source, arena));
}

auto SemaIR::Builder::build() && -> arena_ptr<SemaIR>
{
    if(this->has_command_def)
    {
        this->create_command_from_attributes();
        assert(this->command_ptr != nullptr);
    }
    else
    {
        assert(!this->has_not_flag && this->args.empty());
    }
    return SemaIR::create(this->label_ptr, this->command_ptr, arena);
}

auto SemaIR::Builder::build_command() && -> arena_ptr<const SemaIR::Command>
{
    if(this->has_command_def)
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

void SemaIR::Builder::create_command_from_attributes()
{
    assert(this->has_command_def);

    this->command_ptr = new(*arena, alignof(Command))
            const Command{this->command_source, *this->command_def, this->args,
                          this->has_not_flag ? this->not_flag_value : false};

    this->has_command_def = false;
    this->has_not_flag = false;
}
} // namespace gta3sc

#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/memory.hpp>

namespace gta3sc
{
auto SemaIR::create(const SymbolTable::Label* label, const Command* command,
                    ArenaAllocator<> allocator) -> ArenaPtr<SemaIR>
{
    return allocator.new_object<SemaIR>(private_tag, label, command);
}

auto SemaIR::create_int(int32_t value, SourceRange source,
                        ArenaAllocator<> allocator) -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, value, source);
}

auto SemaIR::create_float(float value, SourceRange source,
                          ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, value, source);
}

auto SemaIR::create_text_label(std::string_view value, SourceRange source,
                               ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto text_label_obj = TextLabel(
            private_tag, util::new_string(value, allocator, util::toupper));

    return allocator.new_object<Argument>(private_tag, text_label_obj, source);
}

auto SemaIR::create_label(const SymbolTable::Label& label, SourceRange source,
                          ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, &label, source);
}

auto SemaIR::create_filename(const SymbolTable::File& filename,
                             SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, &filename, source);
}

auto SemaIR::create_string(std::string_view value, SourceRange source,
                           ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto string_obj = String(private_tag, util::new_string(value, allocator));

    return allocator.new_object<Argument>(private_tag, string_obj, source);
}

auto SemaIR::create_variable(const SymbolTable::Variable& var,
                             SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto var_obj = VarRef(private_tag, var);
    return allocator.new_object<Argument>(private_tag, var_obj, source);
}

auto SemaIR::create_variable(const SymbolTable::Variable& var, int32_t index,
                             SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto var_obj = VarRef(private_tag, var, index);
    return allocator.new_object<Argument>(private_tag, var_obj, source);
}

auto SemaIR::create_variable(const SymbolTable::Variable& var,
                             const SymbolTable::Variable& index,
                             SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto var_obj = VarRef(private_tag, var, index);
    return allocator.new_object<Argument>(private_tag, var_obj, source);
}

auto SemaIR::create_constant(const CommandTable::ConstantDef& cdef,
                             SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, &cdef, source);
}

auto SemaIR::create_used_object(const SymbolTable::UsedObject& used_object,
                                SourceRange source, ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, &used_object, source);
}

auto operator==(const SemaIR& lhs, const SemaIR& rhs) noexcept -> bool
{
    if(lhs.has_label() != rhs.has_label()
       || lhs.has_command() != rhs.has_command())
        return false;

    if(lhs.has_label() && lhs.m_label != rhs.m_label)
        return false;

    if(lhs.has_command() && *lhs.m_command != *rhs.m_command)
        return false;

    return true;
}

auto operator!=(const SemaIR& lhs, const SemaIR& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto operator==(const SemaIR::Command& lhs, const SemaIR::Command& rhs) noexcept
        -> bool
{
    return lhs.m_source == rhs.m_source && lhs.m_def == rhs.m_def
           && lhs.m_not_flag == rhs.m_not_flag
           && std::equal(lhs.m_args.begin(), lhs.m_args.end(),
                         rhs.m_args.begin(), rhs.m_args.end(),
                         [](const auto& a, const auto& b) { return *a == *b; });
}

auto operator!=(const SemaIR::Command& lhs, const SemaIR::Command& rhs) noexcept
        -> bool
{
    return !(lhs == rhs);
}

auto SemaIR::Argument::as_int() const noexcept -> std::optional<int32_t>
{
    if(const auto* value = std::get_if<int32_t>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto SemaIR::Argument::as_float() const noexcept -> std::optional<float>
{
    if(const auto* value = std::get_if<float>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto SemaIR::Argument::as_text_label() const noexcept
        -> std::optional<TextLabel>
{
    if(const auto* text_label = std::get_if<TextLabel>(&this->m_value))
        return *text_label;
    return std::nullopt;
}

auto SemaIR::Argument::as_string() const noexcept -> std::optional<String>
{
    if(const auto* string = std::get_if<String>(&this->m_value))
        return *string;
    return std::nullopt;
}

auto SemaIR::Argument::as_var_ref() const noexcept -> std::optional<VarRef>
{
    if(const auto* var_ref = std::get_if<VarRef>(&this->m_value))
        return *var_ref;
    return std::nullopt;
}

auto SemaIR::Argument::as_label() const noexcept -> const SymbolTable::Label*
{
    if(const auto* label = std::get_if<const SymbolTable::Label*>(
               &this->m_value))
        return *label;
    return nullptr;
}

auto SemaIR::Argument::as_filename() const noexcept -> const SymbolTable::File*
{
    if(auto file = std::get_if<const SymbolTable::File*>(&this->m_value))
        return *file;
    return nullptr;
}

auto SemaIR::Argument::as_constant() const noexcept
        -> const CommandTable::ConstantDef*
{
    if(const auto* sconst = std::get_if<const CommandTable::ConstantDef*>(
               &this->m_value))
        return *sconst;
    return nullptr;
}

auto SemaIR::Argument::as_used_object() const noexcept
        -> const SymbolTable::UsedObject*
{
    if(const auto* uobj = std::get_if<const SymbolTable::UsedObject*>(
               &this->m_value))
        return *uobj;
    return nullptr;
}

auto SemaIR::Argument::pun_as_int() const noexcept -> std::optional<int32_t>
{
    if(const auto value = as_int())
        return *value;
    else if(const auto* sconst = as_constant())
        return sconst->value();
    else
        return std::nullopt;
}

auto SemaIR::Argument::pun_as_float() const noexcept -> std::optional<float>
{
    if(const auto value = as_float())
        return *value;
    else
        return std::nullopt;
}

auto operator==(const SemaIR::Argument& lhs,
                const SemaIR::Argument& rhs) noexcept -> bool
{
    return lhs.m_source == rhs.m_source && lhs.m_value == rhs.m_value;
}

auto operator!=(const SemaIR::Argument& lhs,
                const SemaIR::Argument& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto SemaIR::VarRef::def() const noexcept -> const SymbolTable::Variable&
{
    return *m_def;
}

auto SemaIR::VarRef::has_index() const noexcept -> bool
{
    return !std::holds_alternative<std::monostate>(m_index);
}

auto SemaIR::VarRef::index_as_int() const noexcept -> std::optional<int32_t>
{
    if(const auto value = std::get_if<int32_t>(&this->m_index))
        return *value;
    return std::nullopt;
}

auto SemaIR::VarRef::index_as_variable() const noexcept
        -> const SymbolTable::Variable*
{
    if(const auto* var = std::get_if<const SymbolTable::Variable*>(
               &this->m_index))
        return *var;
    return nullptr;
}

auto operator==(const SemaIR::VarRef& lhs, const SemaIR::VarRef& rhs) noexcept
        -> bool
{
    return lhs.m_def == rhs.m_def && lhs.m_index == rhs.m_index;
}

auto operator!=(const SemaIR::VarRef& lhs, const SemaIR::VarRef& rhs) noexcept
        -> bool
{
    return !(lhs == rhs);
}

auto SemaIR::Builder::label(const SymbolTable::Label* label_ptr) -> Builder&&
{
    this->label_ptr = label_ptr;
    return std::move(*this);
}

auto SemaIR::Builder::command(const Command* command_ptr) -> Builder&&
{
    assert(!this->has_not_flag && !this->has_command_def && this->args.empty());
    this->command_ptr = command_ptr;
    return std::move(*this);
}

auto SemaIR::Builder::command(const CommandTable::CommandDef& command_def,
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

auto SemaIR::Builder::with_num_args(size_t num_args) -> Builder&&
{
    assert(args_hint == no_args_hint && args_capacity == 0);
    this->args_hint = num_args;
    return std::move(*this);
}

auto SemaIR::Builder::arg(const Argument* value) -> Builder&&
{
    const size_t default_args_capacity = args_hint != no_args_hint ? args_hint
                                                                   : 6;

    assert(value != nullptr);

    std::tie(args, args_capacity) = util::new_array_element(
            value, this->args, this->args_capacity, default_args_capacity,
            allocator);

    return std::move(*this);
}

auto SemaIR::Builder::arg_int(int32_t value, SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_int(value, source, allocator));
}

auto SemaIR::Builder::arg_float(float value, SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_float(value, source, allocator));
}

auto SemaIR::Builder::arg_label(const SymbolTable::Label& label,
                                SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_label(label, source, allocator));
}

auto SemaIR::Builder::arg_filename(const SymbolTable::File& filename,
                                   SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_filename(filename, source, allocator));
}

auto SemaIR::Builder::arg_text_label(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_text_label(value, source, allocator));
}

auto SemaIR::Builder::arg_string(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(SemaIR::create_string(value, source, allocator));
}

auto SemaIR::Builder::arg_var(const SymbolTable::Variable& var,
                              SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_variable(var, source, allocator));
}

auto SemaIR::Builder::arg_var(const SymbolTable::Variable& var, int32_t index,
                              SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_variable(var, index, source, allocator));
}

auto SemaIR::Builder::arg_var(const SymbolTable::Variable& var,
                              const SymbolTable::Variable& index,
                              SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_variable(var, index, source, allocator));
}

/// Appends an argument referencing to the given string constant to the
/// command in construction.
auto SemaIR::Builder::arg_const(const CommandTable::ConstantDef& cdef,
                                SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_constant(cdef, source, allocator));
}

auto SemaIR::Builder::arg_object(const SymbolTable::UsedObject& used_object,
                                 SourceRange source) -> Builder&&
{
    return arg(SemaIR::create_used_object(used_object, source, allocator));
}

auto SemaIR::Builder::build() && -> ArenaPtr<SemaIR>
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
    return SemaIR::create(this->label_ptr, this->command_ptr, allocator);
}

auto SemaIR::Builder::build_command() && -> ArenaPtr<const SemaIR::Command>
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

    if(this->args_hint != no_args_hint)
    {
        assert(args_capacity == 0 || args_capacity == args_hint);
        assert(args.size() <= args_hint);
    }

    this->command_ptr = allocator.new_object<Command>(
            private_tag, this->command_source, *this->command_def, this->args,
            this->has_not_flag ? this->not_flag_value : false);

    this->has_command_def = false;
    this->has_not_flag = false;
}
} // namespace gta3sc

// TODO efficient memory usage by not using a variant but internal inheritance

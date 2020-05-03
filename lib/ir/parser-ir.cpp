#include <algorithm>
#include <cstring>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/util/arena-utility.hpp>
#include <gta3sc/util/ctype.hpp>

namespace gta3sc
{
auto ParserIR::create(const LabelDef* label, const Command* command,
                      ArenaAllocator<> allocator) -> ArenaPtr<ParserIR>
{
    return allocator.new_object<ParserIR>(private_tag, label, command);
}

auto ParserIR::create_integer(int32_t value, SourceRange source,
                              ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, value, source);
}

auto ParserIR::create_float(float value, SourceRange source,
                            ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(private_tag, value, source);
}

auto ParserIR::create_identifier(std::string_view name, SourceRange source,
                                 ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(
            private_tag, IdentifierTag{},
            util::allocate_string(name, allocator, util::toupper), source);
}

auto ParserIR::create_filename(std::string_view name, SourceRange source,
                               ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(
            private_tag, FilenameTag{},
            util::allocate_string(name, allocator, util::toupper), source);
}

auto ParserIR::create_string(std::string_view string, SourceRange source,
                             ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    return allocator.new_object<Argument>(
            private_tag, StringTag{}, util::allocate_string(string, allocator),
            source);
}

auto operator==(const ParserIR& lhs, const ParserIR& rhs) noexcept -> bool
{
    if(!!lhs.m_label != !!rhs.m_label)
        return false;
    if(!!lhs.m_command != !!rhs.m_command)
        return false;

    if(lhs.m_label && *lhs.m_label != *rhs.m_label)
        return false;

    if(lhs.m_command && *lhs.m_command != *rhs.m_command)
        return false;

    return true;
}

auto operator!=(const ParserIR& lhs, const ParserIR& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto operator==(const ParserIR::Command& lhs,
                const ParserIR::Command& rhs) noexcept -> bool
{
    return lhs.m_source == rhs.m_source && lhs.m_name == rhs.m_name
           && lhs.m_not_flag == rhs.m_not_flag
           && std::equal(lhs.m_args.begin(), lhs.m_args.end(),
                         rhs.m_args.begin(), rhs.m_args.end(),
                         [](const auto& a, const auto& b) { return *a == *b; });
}

auto operator!=(const ParserIR::Command& lhs,
                const ParserIR::Command& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::LabelDef::create(std::string_view name, SourceRange source,
                                ArenaAllocator<> allocator)
        -> ArenaPtr<const LabelDef>
{
    return allocator.new_object<LabelDef>(
            private_tag, source,
            util::allocate_string(name, allocator, util::toupper));
}

auto operator==(const ParserIR::LabelDef& lhs,
                const ParserIR::LabelDef& rhs) noexcept -> bool
{
    return lhs.m_source == rhs.m_source && lhs.m_name == rhs.m_name;
}

auto operator!=(const ParserIR::LabelDef& lhs,
                const ParserIR::LabelDef& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::Argument::as_integer() const noexcept -> const int32_t*
{
    return std::get_if<int32_t>(&this->m_value);
}

auto ParserIR::Argument::as_float() const noexcept -> const float*
{
    return std::get_if<float>(&this->m_value);
}

auto ParserIR::Argument::as_identifier() const noexcept
        -> const std::string_view*
{
    if(const auto* ident = std::get_if<Identifier>(&this->m_value))
        return &ident->second;
    return nullptr;
}

auto ParserIR::Argument::as_filename() const noexcept -> const std::string_view*
{
    if(const auto* fi = std::get_if<Filename>(&this->m_value))
        return &fi->second;
    return nullptr;
}

auto ParserIR::Argument::as_string() const noexcept -> const std::string_view*
{
    if(const auto* s = std::get_if<String>(&this->m_value))
        return &s->second;
    return nullptr;
}

auto ParserIR::Argument::is_same_value(const Argument& other) const noexcept
        -> bool
{
    return this->m_value == other.m_value;
}

auto operator==(const ParserIR::Argument& lhs,
                const ParserIR::Argument& rhs) noexcept -> bool
{
    return lhs.m_source == rhs.m_source && lhs.is_same_value(rhs);
}

auto operator!=(const ParserIR::Argument& lhs,
                const ParserIR::Argument& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::Builder::label(const LabelDef* label_ptr) -> Builder&&
{
    this->label_ptr = label_ptr;
    return std::move(*this);
}

auto ParserIR::Builder::label(std::string_view name, SourceRange source)
        -> Builder&&
{
    return this->label(LabelDef::create(name, source, allocator));
}

auto ParserIR::Builder::command(const Command* command_ptr) -> Builder&&
{
    assert(!this->has_not_flag && !this->has_command_name
           && this->args.empty());
    this->command_ptr = command_ptr;
    return std::move(*this);
}

auto ParserIR::Builder::command(std::string_view name, SourceRange source)
        -> Builder&&
{
    assert(!this->command_ptr && !this->has_command_name);
    this->command_ptr = nullptr;
    this->has_command_name = true;
    this->command_name = util::allocate_string(name, allocator, util::toupper);
    this->command_source = source;
    return std::move(*this);
}

auto ParserIR::Builder::not_flag(bool not_flag_value) -> Builder&&
{
    this->has_not_flag = true;
    this->not_flag_value = not_flag_value;
    return std::move(*this);
}

auto ParserIR::Builder::arg(const Argument* value) -> Builder&&
{
    assert(value != nullptr);

    if(this->args.size() >= static_cast<std::ptrdiff_t>(args_capacity))
    {
        const auto new_caps = !args_capacity ? 6 : args_capacity * 2;

        auto* const new_args = allocator.allocate_object<const Argument*>(
                new_caps);
        std::uninitialized_move(this->args.begin(), this->args.end(), new_args);

        this->args = util::span(new_args, args.size());
        this->args_capacity = new_caps;
    }

    this->args = util::span(this->args.data(), this->args.size() + 1);
    *(this->args.rbegin()) = value;

    return std::move(*this);
}

auto ParserIR::Builder::arg_int(int32_t value, SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_integer(value, source, allocator));
}

auto ParserIR::Builder::arg_float(float value, SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_float(value, source, allocator));
}

auto ParserIR::Builder::arg_ident(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_identifier(value, source, allocator));
}

auto ParserIR::Builder::arg_filename(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_filename(value, source, allocator));
}

auto ParserIR::Builder::arg_string(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_string(value, source, allocator));
}

auto ParserIR::Builder::build() && -> ArenaPtr<ParserIR>
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
    return ParserIR::create(this->label_ptr, this->command_ptr, allocator);
}

auto ParserIR::Builder::build_command() && -> ArenaPtr<const ParserIR::Command>
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

void ParserIR::Builder::create_command_from_attributes()
{
    assert(this->has_command_name);

    this->command_ptr = allocator.new_object<Command>(
            private_tag, this->command_source, this->command_name, this->args,
            this->has_not_flag ? this->not_flag_value : false);

    this->has_command_name = false;
    this->has_not_flag = false;
}
} // namespace gta3sc

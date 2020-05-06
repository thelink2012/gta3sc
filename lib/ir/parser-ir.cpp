#include <algorithm>
#include <cstring>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/memory.hpp>

namespace gta3sc
{
auto ParserIR::create(const LabelDef* label, const Command* command,
                      ArenaAllocator<> allocator) -> ArenaPtr<ParserIR>
{
    return allocator.new_object<ParserIR>(private_tag, label, command);
}

auto ParserIR::create_int(int32_t value, SourceRange source,
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
    auto identifier_obj = Identifier(
            private_tag, util::new_string(name, allocator, util::toupper));

    return allocator.new_object<Argument>(private_tag, identifier_obj, source);
}

auto ParserIR::create_filename(std::string_view name, SourceRange source,
                               ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto filename_obj = Filename(
            private_tag, util::new_string(name, allocator, util::toupper));

    return allocator.new_object<Argument>(private_tag, filename_obj, source);
}

auto ParserIR::create_string(std::string_view string, SourceRange source,
                             ArenaAllocator<> allocator)
        -> ArenaPtr<const Argument>
{
    auto string_obj = String(private_tag, util::new_string(string, allocator));

    return allocator.new_object<Argument>(private_tag, string_obj, source);
}

auto operator==(const ParserIR& lhs, const ParserIR& rhs) noexcept -> bool
{
    if(lhs.has_label() != rhs.has_label()
       || lhs.has_command() != rhs.has_command())
        return false;

    if(lhs.has_label() && *lhs.m_label != *rhs.m_label)
        return false;

    if(lhs.has_command() && *lhs.m_command != *rhs.m_command)
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
            util::new_string(name, allocator, util::toupper));
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

auto ParserIR::Argument::as_int() const noexcept -> std::optional<int32_t>
{
    if(const auto* value = std::get_if<int32_t>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto ParserIR::Argument::as_float() const noexcept -> std::optional<float>
{
    if(const auto* value = std::get_if<float>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto ParserIR::Argument::as_identifier() const noexcept
        -> std::optional<Identifier>
{
    if(const auto* value = std::get_if<Identifier>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto ParserIR::Argument::as_filename() const noexcept -> std::optional<Filename>
{
    if(const auto* value = std::get_if<Filename>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto ParserIR::Argument::as_string() const noexcept -> std::optional<String>
{
    if(const auto* value = std::get_if<String>(&this->m_value))
        return *value;
    return std::nullopt;
}

auto ParserIR::Argument::is_same_value(const Argument& other) const noexcept
        -> bool
{
    if(this->type() != other.type())
        return false;

    constexpr auto visitor = [](const auto& this_val, const auto& other_val) {
        using T1 = std::decay_t<decltype(this_val)>;
        using T2 = std::decay_t<decltype(other_val)>;
        if constexpr(std::is_same_v<T1, T2>)
            return this_val == other_val;
        else
            return false;
    };

    return visit(visitor, *this, other);
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
    this->command_name = util::new_string(name, allocator, util::toupper);
    this->command_source = source;
    return std::move(*this);
}

auto ParserIR::Builder::not_flag(bool not_flag_value) -> Builder&&
{
    this->has_not_flag = true;
    this->not_flag_value = not_flag_value;
    return std::move(*this);
}

auto ParserIR::Builder::with_num_args(size_t num_args) -> Builder&&
{
    assert(args_hint == -1 && args_capacity == 0);
    this->args_hint = num_args;
    return std::move(*this);
}

auto ParserIR::Builder::arg(const Argument* value) -> Builder&&
{
    const size_t default_args_capacity = args_hint != -1 ? args_hint : 6;

    assert(value != nullptr);

    std::tie(args, args_capacity) = util::new_array_element(
            value, this->args, this->args_capacity, default_args_capacity,
            allocator);

    return std::move(*this);
}

auto ParserIR::Builder::arg_int(int32_t value, SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_int(value, source, allocator));
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

    if(this->args_hint != -1)
    {
        assert(args_capacity == 0 || args_capacity == args_hint);
        assert(args.size() <= args_hint);
    }

    this->command_ptr = allocator.new_object<Command>(
            private_tag, this->command_source, this->command_name, this->args,
            this->has_not_flag ? this->not_flag_value : false);

    this->has_command_name = false;
    this->has_not_flag = false;
}
} // namespace gta3sc

// TODO efficient memory usage by not using a variant but internal inheritance

#include <algorithm>
#include <cstring>
#include <gta3sc/ir/parser-ir.hpp>

namespace gta3sc
{
auto ParserIR::create(arena_ptr<const LabelDef> label,
                      arena_ptr<const Command> command,
                      ArenaMemoryResource* arena) -> arena_ptr<ParserIR>
{
    return new(*arena, alignof(ParserIR)) ParserIR(label, command);
}

auto ParserIR::create_integer(int32_t value, SourceRange source,
                              ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(value, source);
}

auto ParserIR::create_float(float value, SourceRange source,
                            ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument)) const Argument(value, source);
}

auto ParserIR::create_identifier(std::string_view name, SourceRange source,
                                 ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::IdentifierTag{},
                           util::allocate_string_upper(name, *arena), source);
}

auto ParserIR::create_filename(std::string_view name, SourceRange source,
                               ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::FilenameTag{},
                           util::allocate_string_upper(name, *arena), source);
}

auto ParserIR::create_string(std::string_view string, SourceRange source,
                             ArenaMemoryResource* arena)
        -> arena_ptr<const Argument>
{
    return new(*arena, alignof(Argument))
            const Argument(Argument::StringTag{}, string, source);
}

auto operator==(const ParserIR& lhs, const ParserIR& rhs) -> bool
{
    if(!!lhs.label != !!rhs.label)
        return false;
    if(!!lhs.command != !!rhs.command)
        return false;

    if(lhs.label && *lhs.label != *rhs.label)
        return false;

    if(lhs.command && *lhs.command != *rhs.command)
        return false;

    return true;
}

auto operator!=(const ParserIR& lhs, const ParserIR& rhs) -> bool
{
    return !(lhs == rhs);
}

auto operator==(const ParserIR::Command& lhs, const ParserIR::Command& rhs)
        -> bool
{
    return lhs.source == rhs.source && lhs.name == rhs.name
           && lhs.not_flag == rhs.not_flag
           && std::equal(lhs.args.begin(), lhs.args.end(), rhs.args.begin(),
                         rhs.args.end(),
                         [](const auto& a, const auto& b) { return *a == *b; });
}

auto operator!=(const ParserIR::Command& lhs, const ParserIR::Command& rhs)
        -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::LabelDef::create(std::string_view name, SourceRange source,
                                ArenaMemoryResource* arena)
        -> arena_ptr<const LabelDef>
{
    return new(*arena, alignof(LabelDef))
            const LabelDef{source, util::allocate_string_upper(name, *arena)};
}

auto operator==(const ParserIR::LabelDef& lhs, const ParserIR::LabelDef& rhs)
        -> bool
{
    return lhs.source == rhs.source && lhs.name == rhs.name;
}

auto operator!=(const ParserIR::LabelDef& lhs, const ParserIR::LabelDef& rhs)
        -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::Argument::as_integer() const -> const int32_t*
{
    return std::get_if<int32_t>(&this->value);
}

auto ParserIR::Argument::as_float() const -> const float*
{
    return std::get_if<float>(&this->value);
}

auto ParserIR::Argument::as_identifier() const -> const std::string_view*
{
    if(const auto* ident = std::get_if<Identifier>(&this->value))
        return &ident->second;
    return nullptr;
}

auto ParserIR::Argument::as_filename() const -> const std::string_view*
{
    if(const auto* fi = std::get_if<Filename>(&this->value))
        return &fi->second;
    return nullptr;
}

auto ParserIR::Argument::as_string() const -> const std::string_view*
{
    if(const auto* s = std::get_if<String>(&this->value))
        return &s->second;
    return nullptr;
}

auto ParserIR::Argument::is_same_value(const Argument& other) const -> bool
{
    return this->value == other.value;
}

auto operator==(const ParserIR::Argument& lhs, const ParserIR::Argument& rhs)
        -> bool
{
    return lhs.source == rhs.source && lhs.is_same_value(rhs);
}

auto operator!=(const ParserIR::Argument& lhs, const ParserIR::Argument& rhs)
        -> bool
{
    return !(lhs == rhs);
}

auto ParserIR::Builder::label(arena_ptr<const LabelDef> label_ptr) -> Builder&&
{
    this->label_ptr = label_ptr;
    return std::move(*this);
}

auto ParserIR::Builder::label(std::string_view name, SourceRange source)
        -> Builder&&
{
    return this->label(LabelDef::create(name, source, arena));
}

auto ParserIR::Builder::command(arena_ptr<const Command> command_ptr)
        -> Builder&&
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
    this->command_name = util::allocate_string_upper(name, *arena);
    this->command_source = source;
    return std::move(*this);
}

auto ParserIR::Builder::not_flag(bool not_flag_value) -> Builder&&
{
    this->has_not_flag = true;
    this->not_flag_value = not_flag_value;
    return std::move(*this);
}

auto ParserIR::Builder::arg(arena_ptr<const Argument> value) -> Builder&&
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

auto ParserIR::Builder::arg_int(int32_t value, SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_integer(value, source, arena));
}

auto ParserIR::Builder::arg_float(float value, SourceRange source) -> Builder&&
{
    return arg(ParserIR::create_float(value, source, arena));
}

auto ParserIR::Builder::arg_ident(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_identifier(value, source, arena));
}

auto ParserIR::Builder::arg_filename(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_filename(value, source, arena));
}

auto ParserIR::Builder::arg_string(std::string_view value, SourceRange source)
        -> Builder&&
{
    return arg(ParserIR::create_string(value, source, arena));
}

auto ParserIR::Builder::build() && -> arena_ptr<ParserIR>
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

auto ParserIR::Builder::build_command() && -> arena_ptr<const ParserIR::Command>
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

    this->command_ptr = new(*arena, alignof(Command))
            const Command{this->command_source, this->command_name, this->args,
                          this->has_not_flag ? this->not_flag_value : false};

    this->has_command_name = false;
    this->has_not_flag = false;
}
} // namespace gta3sc

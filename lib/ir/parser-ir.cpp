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

bool ParserIR::operator==(const ParserIR& other) const
{
    if(!!this->label != !!other.label)
        return false;
    if(!!this->command != !!other.command)
        return false;

    if(this->label && *this->label != *other.label)
        return false;

    if(this->command && *this->command != *other.command)
        return false;

    return true;
}

bool ParserIR::operator!=(const ParserIR& other) const
{
    return !(*this == other);
}

bool ParserIR::Command::operator==(const Command& other) const
{
    return this->source == other.source && this->name == other.name
           && this->not_flag == other.not_flag
           && std::equal(this->args.begin(), this->args.end(),
                         other.args.begin(), other.args.end(),
                         [](const auto& a, const auto& b) { return *a == *b; });
}

bool ParserIR::Command::operator!=(const Command& other) const
{
    return !(*this == other);
}

auto ParserIR::LabelDef::create(std::string_view name, SourceRange source,
                                ArenaMemoryResource* arena)
        -> arena_ptr<const LabelDef>
{
    return new(*arena, alignof(LabelDef))
            const LabelDef{source, util::allocate_string_upper(name, *arena)};
}

bool ParserIR::LabelDef::operator==(const LabelDef& other) const
{
    return this->source == other.source && this->name == other.name;
}

bool ParserIR::LabelDef::operator!=(const LabelDef& other) const
{
    return !(*this == other);
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
    if(auto ident = std::get_if<Identifier>(&this->value))
        return &ident->second;
    return nullptr;
}

auto ParserIR::Argument::as_filename() const -> const std::string_view*
{
    if(auto fi = std::get_if<Filename>(&this->value))
        return &fi->second;
    return nullptr;
}

auto ParserIR::Argument::as_string() const -> const std::string_view*
{
    if(auto s = std::get_if<String>(&this->value))
        return &s->second;
    return nullptr;
}

bool ParserIR::Argument::is_same_value(const Argument& other) const
{
    return this->value == other.value;
}

bool ParserIR::Argument::operator==(const Argument& other) const
{
    return this->source == other.source && this->is_same_value(other);
}

bool ParserIR::Argument::operator!=(const Argument& other) const
{
    return !(*this == other);
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
        const auto new_args = new(*arena) arena_ptr<const Argument>[new_caps];
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

    this->command_ptr = new(*arena, alignof(Command)) const Command{
            this->command_source, this->command_name, std::move(this->args),
            this->has_not_flag ? this->not_flag_value : false};

    this->has_command_name = false;
    this->has_not_flag = false;
}
}

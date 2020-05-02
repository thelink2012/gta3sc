#pragma once
#include <cstring>
#include <doctest/doctest.h>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/sourceman.hpp>
#include <memory>
#include <ostream>
#include <queue>

// FIXME I don't feel it's good to include this here but are doing so
// in order to pretty print
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>

namespace gta3sc::test::syntax
{
class SyntaxFixture
{
public:
    SyntaxFixture() : diagman([this](const auto& diag) { diags.push(diag); }) {}

    virtual ~SyntaxFixture() { CHECK(diags.empty()); }
    SyntaxFixture(const SyntaxFixture&) = delete;
    auto operator=(const SyntaxFixture&) -> SyntaxFixture& = delete;
    SyntaxFixture(SyntaxFixture&&) noexcept = default;
    auto operator=(SyntaxFixture&&) noexcept -> SyntaxFixture& = default;

protected:
    auto make_source(std::string_view src) -> gta3sc::SourceFile
    {
        const auto n = src.size();
        auto ptr = std::make_unique<char[]>(n + 1);
        std::memcpy(ptr.get(), src.data(), n);
        ptr[n] = '\0';
        return sourceman.load_file(std::move(ptr), n).value();
    }

    auto consume_diag() -> gta3sc::Diagnostic
    {
        REQUIRE(!this->diags.empty());
        auto front = std::move(this->diags.front());
        this->diags.pop();
        return front;
    }

    auto peek_diag() -> const gta3sc::Diagnostic&
    {
        REQUIRE(!this->diags.empty());
        return this->diags.front();
    }

protected:
    // Do not lint regarding protected member variables, this isn't quite in
    // the library code and we'll need protected access to the fixture.
    gta3sc::SourceManager sourceman;      // NOLINT
    gta3sc::DiagnosticHandler diagman;    // NOLINT
    std::queue<gta3sc::Diagnostic> diags; // NOLINT
};
} // namespace gta3sc::test::syntax

namespace gta3sc
{
// TODO can we improve this using internal methods?
inline auto operator<<(std::ostream& os, const Diag& message) -> std::ostream&
{
    os << "Diag(" << static_cast<uint32_t>(message) << ")";
    return os;
}

inline auto operator==(const Diagnostic::Arg& lhs, gta3sc::syntax::Category rhs)
        -> bool
{
    return lhs == gta3sc::Diagnostic::Arg(rhs);
}

inline auto operator==(const Diagnostic::Arg& lhs, std::string rhs) -> bool
{
    return lhs == gta3sc::Diagnostic::Arg(std::move(rhs));
}

inline auto operator==(const Diagnostic::Arg& lhs, std::vector<std::string> rhs)
        -> bool
{
    return lhs == gta3sc::Diagnostic::Arg(std::move(rhs));
}
} // namespace gta3sc

namespace gta3sc
{
// TODO improve these by having std::formatter specializations on lib

inline auto operator<<(std::ostream& os, const ParserIR::Argument& arg)
        -> std::ostream&
{
    if(const int* value = arg.as_integer())
        os << *value;
    else if(const float* value = arg.as_float())
        os << *value;
    else if(const auto* value = arg.as_identifier())
        os << *value;
    else if(const auto* value = arg.as_filename())
        os << *value;
    else if(const auto* value = arg.as_string())
        os << std::quoted(*value);
    else
        assert(false);
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR::LabelDef& label_def)
        -> std::ostream&
{
    os << label_def.name << ':';
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR::Command& command)
        -> std::ostream&
{
    if(command.not_flag)
        os << "NOT ";
    os << command.name;
    for(const auto& arg : command.args)
        os << ' ' << *arg;
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR& ir) -> std::ostream&
{
    if(ir.label)
    {
        os << *ir.label;
        if(ir.command)
            os << ' ';
    }

    if(ir.command)
        os << *ir.command;

    return os;
}

inline auto operator<<(std::ostream& os, const LinkedIR<ParserIR>& ir_list)
        -> std::ostream&
{
    for(const auto& ir : ir_list)
        os << ir << '\n';
    return os;
}
} // namespace gta3sc

namespace gta3sc::test::syntax
{
// FIXME this is a hack because the operator== operators above aren't found.
template<typename T>
auto d(T&& value)
{
    return Diagnostic::Arg(std::forward<T>(value));
}
} // namespace gta3sc::test::syntax

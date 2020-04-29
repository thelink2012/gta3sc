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
    gta3sc::SourceManager sourceman;
    gta3sc::DiagnosticHandler diagman;
    std::queue<gta3sc::Diagnostic> diags;
};
}

namespace gta3sc
{
// TODO can we improve this using internal methods?
inline std::ostream& operator<<(std::ostream& os, const Diag& message)
{
    os << "Diag(" << static_cast<uint32_t>(message) << ")";
    return os;
}

inline bool operator==(const Diagnostic::Arg& lhs, gta3sc::syntax::Category rhs)
{
    return lhs == gta3sc::Diagnostic::Arg(rhs);
}

inline bool operator==(const Diagnostic::Arg& lhs, std::string rhs)
{
    return lhs == gta3sc::Diagnostic::Arg(std::move(rhs));
}

inline bool operator==(const Diagnostic::Arg& lhs, std::vector<std::string> rhs)
{
    return lhs == gta3sc::Diagnostic::Arg(std::move(rhs));
}
}

namespace gta3sc
{
// TODO improve these by having std::formatter specializations on lib

inline std::ostream& operator<<(std::ostream& os, const ParserIR::Argument& arg)
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

inline std::ostream& operator<<(std::ostream& os,
                                const ParserIR::LabelDef& label_def)
{
    os << label_def.name << ':';
    return os;
}

inline std::ostream& operator<<(std::ostream& os,
                                const ParserIR::Command& command)
{
    if(command.not_flag)
        os << "NOT ";
    os << command.name;
    for(auto* it = command.args.begin(); it != command.args.end(); ++it)
        os << ' ' << **it;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const ParserIR& ir)
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

inline std::ostream& operator<<(std::ostream& os,
                                const LinkedIR<ParserIR>& ir_list)
{
    for(auto it = ir_list.begin(); it != ir_list.end(); ++it)
        os << *it << '\n';
    return os;
}
}

namespace gta3sc::test::syntax
{
// FIXME this is a hack because the operator== operators above aren't found.
template<typename T>
auto d(T&& value)
{
    return Diagnostic::Arg(std::move(value));
}
}

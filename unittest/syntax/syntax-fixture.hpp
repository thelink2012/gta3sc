#pragma once
#include "../with-diagnostic-fixture.hpp"
#include <ostream>

// FIXME I don't feel it's good to include this here but are doing so
// in order to pretty print
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>

namespace gta3sc::test::syntax
{
class SyntaxFixture : public WithDiagnosticFixture
{
public:
    SyntaxFixture() = default;
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
    auto visitor = [&os](const auto& value) {
        if constexpr(std::is_same_v<std::decay_t<decltype(value)>,
                                    ParserIR::String>)
            os << std::quoted(std::string_view(value));
        else
            os << value;
    };

    visit(visitor, arg);
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR::LabelDef& label_def)
        -> std::ostream&
{
    os << label_def.name() << ':';
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR::Command& command)
        -> std::ostream&
{
    if(command.not_flag())
        os << "NOT ";
    os << command.name();
    for(const auto& arg : command.args())
        os << ' ' << arg;
    return os;
}

inline auto operator<<(std::ostream& os, const ParserIR& ir) -> std::ostream&
{
    if(ir.has_label())
    {
        os << ir.label();
        if(ir.has_command())
            os << ' ';
    }

    if(ir.has_command())
        os << ir.command();

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

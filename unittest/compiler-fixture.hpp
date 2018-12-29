#pragma once
#include <doctest.h>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/diagnostics.hpp>
#include <cstring>
#include <memory>
#include <queue>
#include <ostream>

namespace gta3sc::test
{
class CompilerFixture
{
public:
    CompilerFixture() :
        source_file(make_source("")),
        diagman([this](const auto& diag) { diags.push(diag); })
    {}

    virtual ~CompilerFixture()
    {
        CHECK(diags.empty());
    }

protected:
    void build_source(std::string_view src)
    {
        this->source_file = make_source(src);
    }

    static auto make_source(std::string_view src) -> gta3sc::SourceFile
    {
        const auto n = src.size();
        auto ptr = std::make_unique<char[]>(n+1);
        std::memcpy(ptr.get(), src.data(), n);
        ptr[n] = '\0';
        return gta3sc::SourceFile(std::move(ptr), n);
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
    gta3sc::SourceFile source_file;
    gta3sc::DiagnosticHandler diagman;
    std::queue<gta3sc::Diagnostic> diags;
};
}

namespace gta3sc
{
inline std::ostream& operator<<(std::ostream& os, const Diag& message)
{
    os << "Diag(" << static_cast<uint32_t>(message) << ")";
    return os;
}

inline bool operator==(const Diagnostic::Arg& lhs, Category rhs)
{
    return lhs == Diagnostic::Arg(rhs);
}

inline bool operator==(const Diagnostic::Arg& lhs, std::string rhs)
{
    return lhs == Diagnostic::Arg(std::move(rhs));
}

inline bool operator==(const Diagnostic::Arg& lhs, std::vector<std::string> rhs)
{
    return lhs == Diagnostic::Arg(std::move(rhs));
}
}

#pragma once
#include <cstring>
#include <doctest/doctest.h>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/sourceman.hpp>
#include <queue>

namespace gta3sc::test
{
class WithDiagnosticFixture
{
public:
    WithDiagnosticFixture() :
        diagman([this](const auto& diag) { diags.push(diag); })
    {}

    virtual ~WithDiagnosticFixture() { CHECK(diags.empty()); }

    WithDiagnosticFixture(const WithDiagnosticFixture&) = delete;
    auto
    operator=(const WithDiagnosticFixture&) -> WithDiagnosticFixture& = delete;
    
    WithDiagnosticFixture(WithDiagnosticFixture&&) noexcept = default;
    auto operator=(WithDiagnosticFixture&&) noexcept
            -> WithDiagnosticFixture& = default;

protected:
    auto make_source(std::string_view content) -> SourceFile
    {
        const auto n = content.size();
        auto ptr = std::make_unique<char[]>(n + 1);
        std::memcpy(ptr.get(), content.data(), n);
        ptr[n] = '\0';
        return sourceman.load_file(std::move(ptr), n).value();
    }

    auto consume_diag() -> Diagnostic
    {
        REQUIRE(!diags.empty());
        auto front = std::move(diags.front());
        diags.pop();
        return front;
    }

    auto peek_diag() -> const Diagnostic&
    {
        REQUIRE(!diags.empty());
        return diags.front();
    }

    SourceManager sourceman;
    DiagnosticHandler diagman;
    std::queue<Diagnostic> diags;
};
} // namespace gta3sc::test
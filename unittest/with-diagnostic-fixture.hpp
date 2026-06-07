#pragma once
#include <cstring>
#include <doctest/doctest.h>
#include <gta3sc/diagnostics.hpp>
#include <queue>

namespace gta3sc::test
{
class WithDiagnosticFixture
{
public:
    WithDiagnosticFixture() :
        diagman([this](const auto& diag) { diags.push(diag); })
    {}

    ~WithDiagnosticFixture() { CHECK(diags.empty()); }

    WithDiagnosticFixture(const WithDiagnosticFixture&) = delete;
    auto
    operator=(const WithDiagnosticFixture&) -> WithDiagnosticFixture& = delete;

    WithDiagnosticFixture(WithDiagnosticFixture&&) noexcept = default;
    auto operator=(WithDiagnosticFixture&&) noexcept
            -> WithDiagnosticFixture& = default;

protected:
    auto consume_diag() -> Diagnostic
    {
        REQUIRE(!diags.empty());
        auto front = std::move(diags.front());
        diags.pop();
        return std::move(front);
    }

    auto peek_diag() -> const Diagnostic&
    {
        REQUIRE(!diags.empty());
        return diags.front();
    }

    CallbackDiagnosticHandler diagman;
    std::queue<Diagnostic> diags;
};
} // namespace gta3sc::test
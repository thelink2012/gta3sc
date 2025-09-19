#pragma once
#include "../with-diagnostic-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/config/config.hpp>

namespace gta3sc::test::config
{
class ConfigFixture : public WithDiagnosticFixture
{
public:
    ConfigFixture() = default;

protected:
    auto build_config(std::string_view src) -> CommandTable
    {
        auto source = make_source(src);
        return gta3sc::load_config(source, diagman,
                                   gta3sc::CommandTable::Builder(&arena))
                .build();
    }

    ArenaMemoryResource arena;
};
} // namespace gta3sc::test::config
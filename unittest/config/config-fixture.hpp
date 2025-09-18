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

class LoadConfigFixture : public ConfigFixture
{
public:
    LoadConfigFixture()
    {
        root_test_dir = std::filesystem::temp_directory_path()
                        / "gta3sc_load_config_test";
        std::filesystem::create_directories(root_test_dir);
    }

    ~LoadConfigFixture() { std::filesystem::remove_all(root_test_dir); }

    LoadConfigFixture(const LoadConfigFixture&) = delete;
    auto operator=(const LoadConfigFixture&) -> LoadConfigFixture& = delete;

    LoadConfigFixture(LoadConfigFixture&&) = delete;
    auto operator=(LoadConfigFixture&&) -> LoadConfigFixture& = delete;

protected:
    void create_test_file(const std::filesystem::path& path,
                          std::string_view content);

    std::filesystem::path root_test_dir;
};
} // namespace gta3sc::test::config
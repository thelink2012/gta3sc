#include "../with-diagnostic-fixture.hpp"
#include "../with-source-fixture.hpp"
#include "../with-temp-dir-fixture.hpp"
#include <gta3sc/filesystem/path-resolver.hpp>
#include <gta3sc/model-table.hpp>

namespace gta3sc::test::config
{
class ModelsTestFixture
    : public WithTempDirFixture
    , public WithDiagnosticFixture
    , public WithSourceFixture
{
public:
    auto expect_model(const ModelTable& table, std::string_view name,
                      uint32_t expected_id) -> const ModelTable::ModelDef*
    {
        auto* model = table.find_model(name);
        REQUIRE(model != nullptr);
        CHECK(model->model_id() == expected_id);
        return model;
    }

    auto
    expect_model(const ModelTable& table, std::string_view name,
                 uint32_t expected_id,
                 std::string_view expected_name) -> const ModelTable::ModelDef*
    {
        auto* model = expect_model(table, name, expected_id);
        CHECK(model->name() == expected_name);
        return model;
    }

    void expect_no_model(const ModelTable& table, std::string_view name)
    {
        auto* model = table.find_model(name);
        CHECK(model == nullptr);
    }

protected:
    [[nodiscard]] auto
    make_path_resolver() const -> gta3sc::filesystem::RelativePathResolver
    {
        return gta3sc::filesystem::RelativePathResolver(root_test_dir);
    }

    ArenaMemoryResource arena;
};
} // namespace gta3sc::test::config

#pragma once
#include <filesystem>

namespace gta3sc::test
{
class WithTempDirFixture
{
public:
    WithTempDirFixture()
    {
        root_test_dir = std::filesystem::temp_directory_path()
                        / "gta3sc_unit_test";
        std::filesystem::remove_all(
                root_test_dir); // ensure the directory is empty
        std::filesystem::create_directories(root_test_dir);
    }

    ~WithTempDirFixture() { std::filesystem::remove_all(root_test_dir); }

    WithTempDirFixture(const WithTempDirFixture&) = delete;
    auto operator=(const WithTempDirFixture&) -> WithTempDirFixture& = delete;

    WithTempDirFixture(WithTempDirFixture&&) = delete;
    auto operator=(WithTempDirFixture&&) -> WithTempDirFixture& = delete;

protected:
    void create_test_file(const std::filesystem::path& path,
                          std::string_view content);

    std::filesystem::path root_test_dir;
};
} // namespace gta3sc::test
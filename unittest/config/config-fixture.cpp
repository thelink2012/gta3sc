#include "config-fixture.hpp"
#include <filesystem>
#include <fstream>

namespace gta3sc::test::config
{
void LoadConfigFixture::create_test_file(const std::filesystem::path& path,
                                         std::string_view content)
{
    std::ofstream file(path);
    file << content;
}
} // namespace gta3sc::test::config
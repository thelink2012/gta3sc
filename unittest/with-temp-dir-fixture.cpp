#include "with-temp-dir-fixture.hpp"
#include <fstream>

namespace gta3sc::test
{
void WithTempDirFixture::create_test_file(const std::filesystem::path& path,
                                          std::string_view content)
{
    std::filesystem::create_directories(root_test_dir / path.parent_path());
    std::ofstream file(root_test_dir / path);
    file << content;
}
} // namespace gta3sc::test
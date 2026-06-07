#pragma once
#include <cstring>
#include <gta3sc/source-manager.hpp>

namespace gta3sc::test
{
class WithSourceFixture
{
public:
    WithSourceFixture() {}

    WithSourceFixture(const WithSourceFixture&) = delete;
    auto operator=(const WithSourceFixture&) -> WithSourceFixture& = delete;

    WithSourceFixture(WithSourceFixture&&) noexcept = default;
    auto
    operator=(WithSourceFixture&&) noexcept -> WithSourceFixture& = default;

protected:
    auto make_source(std::string_view content) -> FileEntryRef
    {
        const auto n = content.size();
        auto ptr = std::make_unique<char[]>(n + 1);
        std::memcpy(ptr.get(), content.data(), n);
        ptr[n] = '\0';
        return sourceman.load_buffer(std::move(ptr), n + 1).value();
    }

    SourceManager sourceman;
};
} // namespace gta3sc::test
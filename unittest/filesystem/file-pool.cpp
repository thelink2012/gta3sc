#include "../with-temp-dir-fixture.hpp"
#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <gta3sc/filesystem/file-pool.hpp>
#include <gta3sc/util/arena.hpp>
#include <memory>
#include <string_view>

using gta3sc::ArenaAllocator;
using gta3sc::ArenaMemoryResource;
using gta3sc::filesystem::FileEntryRef;
using gta3sc::filesystem::FileLoc;
using gta3sc::filesystem::FilePool;
using gta3sc::filesystem::FileRange;
using gta3sc::filesystem::no_file_loc;
using namespace gta3sc::test;

namespace
{
class FilePoolFixture : public WithTempDirFixture
{
public:
    static auto make_buffer(std::string_view content)
            -> std::pair<std::unique_ptr<char[]>, size_t>
    {
        const size_t total = content.size() + 1;
        auto buf = std::make_unique<char[]>(total);
        std::memcpy(buf.get(), content.data(), content.size());
        buf[content.size()] = '\0';
        return {std::move(buf), total};
    }

    auto load_buffer(std::string_view content) -> FileEntryRef
    {
        auto [buf, total] = make_buffer(content);
        auto ref = pool.load_buffer(std::move(buf), total);
        REQUIRE(ref.has_value());
        return std::move(*ref);
    }

protected:
    FilePool pool;
};
} // namespace

TEST_CASE_FIXTURE(FilePoolFixture, "load buffer")
{
    SUBCASE("content and metadata are correct")
    {
        auto [buf, total] = make_buffer("hello world");

        auto got = pool.load_buffer(std::move(buf), total);

        REQUIRE(got.has_value());
        CHECK(got->size() == 11);
        CHECK(got->view() == "hello world");
        CHECK(got->path().empty());
    }

    SUBCASE("empty content")
    {
        auto [buf, total] = make_buffer("");

        auto got = pool.load_buffer(std::move(buf), total);

        REQUIRE(got.has_value());
        CHECK(got->size() == 0);
        CHECK(got->view().empty());
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "load file")
{
    SUBCASE("content and metadata are correct")
    {
        create_test_file("hello.txt", "hello world");

        auto got = pool.load_file(root_test_dir / "hello.txt");

        REQUIRE(got.has_value());
        CHECK(got->size() == 11);
        CHECK(got->view() == "hello world");
        CHECK(got->path() == root_test_dir / "hello.txt");
    }

    SUBCASE("empty file")
    {
        create_test_file("empty.txt", "");

        auto got = pool.load_file(root_test_dir / "empty.txt");

        REQUIRE(got.has_value());
        CHECK(got->size() == 0);
        CHECK(got->view().empty());
    }

    SUBCASE("missing path returns nullopt")
    {
        CHECK(!pool.load_file(root_test_dir / "nope.txt").has_value());
    }

    SUBCASE("directory path returns nullopt")
    {
        std::filesystem::create_directory(root_test_dir / "subdir");

        CHECK(!pool.load_file(root_test_dir / "subdir").has_value());
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "location model")
{
    SUBCASE("location_of maps bytes to consecutive FileLocs")
    {
        auto ref = load_buffer("abcd");

        const auto start = ref.location_of(ref.data());
        const auto past = ref.location_of(ref.data() + ref.size());

        CHECK(start + static_cast<std::ptrdiff_t>(ref.size()) == past);
    }

    SUBCASE("location_of string_view overload matches char* overload")
    {
        auto ref = load_buffer("abcd");

        CHECK(ref.location_of(ref.view()) == ref.location_of(ref.data()));
    }

    SUBCASE("sequential buffers have a one-location gap between their ranges")
    {
        auto r1 = load_buffer("aa");
        auto r2 = load_buffer("bb");

        const auto end1 = r1.location_of(r1.data() + r1.size());
        const auto start2 = r2.location_of(r2.data());

        CHECK(end1 + 1 == start2);
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "view_of")
{
    SUBCASE("full file")
    {
        auto ref = load_buffer("abcd");
        const auto begin = ref.location_of(ref.data());

        CHECK(ref.view_of(FileRange{begin, begin + 4}) == "abcd");
    }

    SUBCASE("middle slice")
    {
        auto ref = load_buffer("abcdef");
        const auto loc_c = ref.location_of(ref.data() + 2);

        CHECK(ref.view_of(FileRange{loc_c, loc_c + 3}) == "cde");
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "string_copy_of")
{
    SUBCASE("full file")
    {
        auto ref = load_buffer("abcd");
        const auto range = FileRange{ref.location_of(ref.data()),
                                     ref.location_of(ref.data() + ref.size())};

        auto got = pool.string_copy_of(range);

        REQUIRE(got.has_value());
        CHECK(*got == "abcd");
    }

    SUBCASE("middle slice")
    {
        auto ref = load_buffer("abcdef");
        const auto loc_c = ref.location_of(ref.data() + 2);
        const auto range = FileRange{loc_c, loc_c + 3};

        auto got = pool.string_copy_of(range);

        REQUIRE(got.has_value());
        CHECK(*got == "cde");
    }

    SUBCASE("zero-length range")
    {
        auto ref = load_buffer("abcd");
        const auto mid = ref.location_of(ref.data() + 2);
        const auto range = FileRange{mid, mid};

        auto got = pool.string_copy_of(range);

        REQUIRE(got.has_value());
        CHECK(got->empty());
    }

    SUBCASE("range past end returns nullopt")
    {
        auto ref = load_buffer("abcd");
        const auto begin = ref.location_of(ref.data());
        const auto oob = FileRange{begin, begin + 5};

        CHECK(!pool.string_copy_of(oob).has_value());
    }

    SUBCASE("range spanning two files returns nullopt")
    {
        auto r1 = load_buffer("aa");
        auto r2 = load_buffer("bb");
        const auto begin = r1.location_of(r1.data());
        const auto end = r2.location_of(r2.data() + r2.size());
        const auto cross = FileRange{begin, end};

        CHECK(!pool.string_copy_of(cross).has_value());
    }

    SUBCASE("no_file_loc range returns nullopt")
    {
        const auto range = FileRange{no_file_loc, no_file_loc};

        CHECK(!pool.string_copy_of(range).has_value());
    }

    SUBCASE("empty pool returns nullopt")
    {
        const auto range = FileRange{FileLoc{1}, FileLoc{2}};

        CHECK(!pool.string_copy_of(range).has_value());
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "string_copy_of with arena")
{
    char arena_buf[256];
    ArenaMemoryResource arena_mem{arena_buf, sizeof(arena_buf)};
    ArenaAllocator<> alloc{&arena_mem};

    SUBCASE("result is allocated inside the arena")
    {
        auto ref = load_buffer("abcd");
        const auto begin = ref.location_of(ref.data());
        const auto range = FileRange{begin, begin + 2};

        auto got = pool.string_copy_of(range, alloc);

        REQUIRE(got.has_value());
        CHECK(*got == "ab");
        CHECK(got->data() >= arena_buf);
        CHECK(got->data() < arena_buf + sizeof(arena_buf));
    }

    SUBCASE("zero-length range returns empty view without allocating")
    {
        auto ref = load_buffer("abcd");
        const auto mid = ref.location_of(ref.data() + 2);
        const auto range = FileRange{mid, mid};

        auto got = pool.string_copy_of(range, alloc);

        REQUIRE(got.has_value());
        CHECK(got->empty());
        CHECK(got->data() == nullptr);
    }
}

TEST_CASE_FIXTURE(FilePoolFixture, "pool move")
{
    SUBCASE("existing refs remain valid after move")
    {
        auto ref = load_buffer("data");

        FilePool pool2 = std::move(pool);

        CHECK(ref.view() == "data");
    }

    SUBCASE("moved-from pool is usable")
    {
        FilePool pool2 = std::move(pool);
        auto [buf, total] = make_buffer("new");

        auto got = pool.load_buffer(std::move(buf), total);

        REQUIRE(got.has_value());
        CHECK(got->view() == "new");
    }

    SUBCASE("move assignment transfers ownership")
    {
        auto ref = load_buffer("data");
        FilePool pool2;
        pool2 = std::move(pool);

        CHECK(ref.view() == "data");
    }
}

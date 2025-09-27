#include "relocation-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/relocator.hpp>
#include <gta3sc/diagnostics.hpp>

using gta3sc::codegen::Relocator;
using gta3sc::test::codegen::RelocationFixture;
using FileType = gta3sc::SymbolTable::FileType;

namespace
{
class RelocatorFixture : public RelocationFixture
{
public:
    void setup_bytecode(uint32_t offset)
    {
        if(bytecode.size() < offset + 4)
            bytecode.resize(offset + 4, placeholder_byte);
    }

    void insert_fixup_entry(const SymbolTable::Label& label,
                            const SymbolTable::File& origin,
                            RelocationTable::AbsoluteOffset offset)
    {
        setup_bytecode(offset);
        RelocationFixture::insert_fixup_entry(label, origin, offset);
    }

    void insert_fixup_entry(const SymbolTable::File& file,
                            RelocationTable::AbsoluteOffset offset)
    {
        setup_bytecode(offset);
        RelocationFixture::insert_fixup_entry(file, offset);
    }

    void assert_bytecode(uint32_t offset, int32_t expected_value)
    {
        assert(bytecode.size() >= offset + 4);

        // Reconstruct the 32-bit value from little-endian bytes
        uint32_t actual_value = 0;
        actual_value |= static_cast<uint32_t>(bytecode[offset]);
        actual_value |= static_cast<uint32_t>(bytecode[offset + 1]) << 8;
        actual_value |= static_cast<uint32_t>(bytecode[offset + 2]) << 16;
        actual_value |= static_cast<uint32_t>(bytecode[offset + 3]) << 24;

        CHECK(static_cast<int32_t>(actual_value) == expected_value);
    }

    // assert that all bytes are placeholder_byte except for the bytes at the
    // given offsets
    void
    assert_placeholders(std::vector<RelocationTable::AbsoluteOffset> offsets)
    {
        std::sort(offsets.begin(), offsets.end());

        size_t current_offset = 0;
        size_t current_offset_to_check = 0;
        while(current_offset < bytecode.size())
        {
            if(current_offset_to_check < offsets.size()
               && current_offset == offsets[current_offset_to_check])
            {
                current_offset_to_check++;
                current_offset += 4;
                continue;
            }
            CHECK(bytecode[current_offset] == placeholder_byte);
            ++current_offset;
        }
    }

protected:
    static constexpr auto placeholder_byte = std::byte{0x99};
    std::vector<std::byte> bytecode;
};
} // namespace

TEST_CASE_FIXTURE(RelocatorFixture, "relocate static method")
{
    SUBCASE("writes little-endian 32-bit value correctly")
    {
        setup_bytecode(8);

        Relocator::relocate(bytecode, 0, 0x12345678);
        assert_bytecode(0, 0x12345678);

        assert_placeholders({0});
    }

    SUBCASE("handles zero value")
    {
        setup_bytecode(8);

        Relocator::relocate(bytecode, 0, 0);
        assert_bytecode(0, 0);

        assert_placeholders({0});
    }

    SUBCASE("handles negative values")
    {
        setup_bytecode(12);

        Relocator::relocate(bytecode, 0, -0x12345678);
        assert_bytecode(0, -0x12345678);

        Relocator::relocate(bytecode, 4, -2147483648); // INT32_MIN
        assert_bytecode(4, -2147483648);

        assert_placeholders({0, 4});
    }

    SUBCASE("handles maximum positive value")
    {
        setup_bytecode(8);

        Relocator::relocate(bytecode, 0, 2147483647); // INT32_MAX
        assert_bytecode(0, 2147483647);

        assert_placeholders({0});
    }
}

TEST_CASE_FIXTURE(RelocatorFixture, "relocator relocates label fixup entry")
{
    constexpr auto label_offset = 100;
    constexpr auto fixup_offset = 50;
    const auto mission_file_offset = 200;

    const auto& main_file = make_file(FileType::main);
    const auto& label = make_label();

    insert_file_loc(main_file, 0);
    insert_label_loc(label, main_file, label_offset);
    insert_fixup_entry(label, main_file, fixup_offset);

    auto relocator = Relocator(bytecode);

    SUBCASE("successful relocation")
    {
        const auto& fixup_entry = reloc_table.fixup_table().front();
        REQUIRE(relocator.relocate(reloc_table, fixup_entry, diagman));
        assert_bytecode(fixup_offset, label_offset);
    }

    SUBCASE("relocation failure propagates error")
    {
        const auto& mission_file = make_file(FileType::mission);
        const auto& mission_label = make_label();

        insert_file_loc(mission_file, mission_file_offset);
        insert_fixup_entry(mission_label, mission_file, mission_file_offset);
        insert_label_loc(mission_label, mission_file, mission_file_offset);

        const auto& fixup_entry = reloc_table.fixup_table().back();
        REQUIRE_FALSE(relocator.relocate(reloc_table, fixup_entry, diagman));
        REQUIRE(consume_diag().descriptor
                == &gta3sc::codegen::diag::label_at_local_zero_offset);
    }

    assert_placeholders({fixup_offset});
}

TEST_CASE_FIXTURE(RelocatorFixture, "relocator relocates file fixup entry")
{
    constexpr auto file_offset = 200;
    constexpr auto fixup_offset = 80;

    const auto& mission_file = make_file(FileType::mission);

    insert_file_loc(mission_file, file_offset);
    insert_fixup_entry(mission_file, fixup_offset);

    auto relocator = Relocator(bytecode);

    SUBCASE("successful file relocation")
    {
        const auto& file_fixup_entry = reloc_table.file_fixup_table().front();
        REQUIRE(relocator.relocate(reloc_table, file_fixup_entry, diagman));
        assert_bytecode(fixup_offset, file_offset);
    }

    // no error case for file fixup entry relocation

    assert_placeholders({fixup_offset});
}

TEST_CASE_FIXTURE(RelocatorFixture,
                  "relocator relocates entire relocation table")
{
    constexpr auto fixup1_offset = 10;
    constexpr auto file_fixup_offset = 30;
    constexpr auto label1_offset = 100;
    constexpr auto label2_offset = 200;
    constexpr auto fixup2_offset = 150;
    constexpr auto mission_file_offset = 300;
    constexpr auto label3_offset = 350;
    constexpr auto fixup3_offset = 380;
    constexpr auto local3_offset = -50; // label3_offset - mission_file_offset;

    const auto& main_file = make_file(FileType::main);
    const auto& mission_file = make_file(FileType::mission);
    const auto& label1 = make_label();
    const auto& label2 = make_label();
    const auto& label3 = make_label();

    // Setup all locations
    insert_label_loc(label1, main_file, label1_offset);
    insert_label_loc(label2, main_file, label2_offset);
    insert_label_loc(label3, mission_file, label3_offset);
    insert_file_loc(main_file, 0);
    insert_file_loc(mission_file, mission_file_offset);

    // Add fixup entries
    insert_fixup_entry(label1, main_file, fixup1_offset);
    insert_fixup_entry(label2, main_file, fixup2_offset);
    insert_fixup_entry(mission_file, file_fixup_offset);
    insert_fixup_entry(label3, mission_file, fixup3_offset);

    auto relocator = Relocator(bytecode);

    SUBCASE("successful full relocation")
    {
        REQUIRE(relocator.relocate(reloc_table, diagman));

        assert_bytecode(fixup1_offset, label1_offset);
        assert_bytecode(fixup2_offset, label2_offset);
        assert_bytecode(fixup3_offset, local3_offset);
        assert_bytecode(file_fixup_offset, mission_file_offset);
    }

    SUBCASE("partial failure returns false but continues processing")
    {
        // Add a failing fixup entry
        const auto& mission_zero_label = make_label();
        insert_label_loc(mission_zero_label, mission_file, mission_file_offset);
        insert_fixup_entry(mission_zero_label, mission_file,
                           mission_file_offset);

        REQUIRE_FALSE(relocator.relocate(reloc_table, diagman));

        // Successful relocations should still be processed
        assert_bytecode(fixup1_offset, label1_offset);
        assert_bytecode(fixup2_offset, label2_offset);
        assert_bytecode(fixup3_offset, local3_offset);
        assert_bytecode(file_fixup_offset, mission_file_offset);

        REQUIRE(consume_diag().descriptor
                == &gta3sc::codegen::diag::label_at_local_zero_offset);
    }

    assert_placeholders(
            {fixup1_offset, fixup2_offset, fixup3_offset, file_fixup_offset});
}

TEST_CASE_FIXTURE(RelocatorFixture, "relocator with empty relocation table")
{
    setup_bytecode(100);
    auto relocator = Relocator(bytecode);
    REQUIRE(relocator.relocate(reloc_table, diagman));
    assert_placeholders({});
}

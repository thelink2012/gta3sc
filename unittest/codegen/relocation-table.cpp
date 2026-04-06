#include "relocation-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/diagnostics.hpp>

using gta3sc::DiagnosticDescriptor;
using gta3sc::codegen::RelocationTable;
using gta3sc::test::codegen::RelocationFixture;
using FileType = gta3sc::SymbolTable::FileType;

namespace
{
class RelocationTableFixture : public RelocationFixture
{
public:
    auto relocate_one_from_fixup_table() -> RelocationTable::RelativeOffset
    {
        const auto view = reloc_table.fixup_table();
        REQUIRE(view.size() == 1);
        const auto reloc_offset = reloc_table.relocate(view.front(), diagman);
        REQUIRE(reloc_offset != std::nullopt);
        return reloc_offset.value();
    }

    auto relocate_one_from_file_fixup_table() -> RelocationTable::RelativeOffset
    {
        const auto view = reloc_table.file_fixup_table();
        REQUIRE(view.size() == 1);
        const auto reloc_offset = reloc_table.relocate(view.front(), diagman);
        REQUIRE(reloc_offset != std::nullopt);
        return reloc_offset.value();
    }

    void fail_to_relocate_one(const DiagnosticDescriptor& reason)
    {
        const auto view = reloc_table.fixup_table();
        REQUIRE(view.size() == 1);
        REQUIRE(reloc_table.relocate(view.front(), diagman) == std::nullopt);
        REQUIRE(consume_diag().descriptor == &reason);
    }
};
} // namespace

TEST_CASE_FIXTURE(RelocationTableFixture, "references to main label")
{
    constexpr auto main_label_offset = 100;
    constexpr auto label_ref_offset = 120;

    const auto& main_file = make_file(FileType::main);
    const auto& main_label = make_label();

    insert_label_loc(main_label, main_file, main_label_offset);
    insert_file_loc(main_file, 0);

    SUBCASE("reference from main itself produces absolute offset")
    {
        insert_fixup_entry(main_label, main_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_label_offset);
    }

    SUBCASE("reference from main extension file produces absolute offset")
    {
        const auto& main_extension_file = make_file(FileType::main_extension);
        insert_fixup_entry(main_label, main_extension_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_label_offset);
    }

    SUBCASE("reference from subscript file produces absolute offset")
    {
        const auto& subscript_file = make_file(FileType::subscript);
        insert_fixup_entry(main_label, subscript_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_label_offset);
    }

    SUBCASE("reference from mission script file produces absolute offset")
    {
        const auto& mission_file = make_file(FileType::mission);
        insert_fixup_entry(main_label, mission_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_label_offset);
    }
}

TEST_CASE_FIXTURE(RelocationTableFixture, "references to main extension label")
{
    constexpr auto main_extension_label_offset = 100;
    constexpr auto label_ref_offset = 120;

    const auto& main_extension_file = make_file(FileType::main_extension);
    const auto& main_extension_label = make_label();

    insert_label_loc(main_extension_label, main_extension_file,
                     main_extension_label_offset);
    insert_file_loc(main_extension_file, 0);

    SUBCASE("reference from main produces absolute offset")
    {
        const auto& main_file = make_file(FileType::main);
        insert_fixup_entry(main_extension_label, main_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_extension_label_offset);
    }

    SUBCASE("reference from the main extension file itself produces absolute "
            "offset")
    {
        insert_fixup_entry(main_extension_label, main_extension_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_extension_label_offset);
    }

    SUBCASE("reference from another main extension file produces absolute "
            "offset")
    {
        const auto& other_main_extension_file = make_file(
                FileType::main_extension);
        insert_fixup_entry(main_extension_label, other_main_extension_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_extension_label_offset);
    }

    SUBCASE("reference from subscript file produces absolute offset")
    {
        const auto& subscript_file = make_file(FileType::subscript);
        insert_fixup_entry(main_extension_label, subscript_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_extension_label_offset);
    }

    SUBCASE("reference from mission script file produces absolute offset")
    {
        const auto& mission_file = make_file(FileType::mission);
        insert_fixup_entry(main_extension_label, mission_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == main_extension_label_offset);
    }
}

TEST_CASE_FIXTURE(RelocationTableFixture, "references to subscript labels")
{
    constexpr auto subscript_label_offset = 100;
    constexpr auto label_ref_offset = 120;

    const auto& subscript_file = make_file(FileType::subscript);
    const auto& subscript_label = make_label();

    insert_label_loc(subscript_label, subscript_file, subscript_label_offset);
    insert_file_loc(subscript_file, 0);

    SUBCASE("reference from main produces absolute offset")
    {
        const auto& main_file = make_file(FileType::main);
        insert_fixup_entry(subscript_label, main_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == subscript_label_offset);
    }

    SUBCASE("reference from main extension file produces absolute offset")
    {
        const auto& main_extension_file = make_file(FileType::main_extension);
        insert_fixup_entry(subscript_label, main_extension_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == subscript_label_offset);
    }

    SUBCASE("reference from the subscript file itself produces absolute offset")
    {
        insert_fixup_entry(subscript_label, subscript_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == subscript_label_offset);
    }

    SUBCASE("reference from another subscript file produces absolute offset")
    {
        const auto& other_subscript_file = make_file(FileType::subscript);
        insert_fixup_entry(subscript_label, other_subscript_file,
                           label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == subscript_label_offset);
    }

    SUBCASE("reference from mission script file produces absolute offset")
    {
        const auto& mission_file = make_file(FileType::mission);
        insert_fixup_entry(subscript_label, mission_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table() == subscript_label_offset);
    }
}

TEST_CASE_FIXTURE(RelocationTableFixture, "references to mission labels")
{
    constexpr auto mission_file_offset = 70;
    constexpr auto mission_label_offset = 100;
    constexpr auto relative_mission_label_offset = -(mission_label_offset
                                                     - mission_file_offset);
    constexpr auto label_ref_offset = 120;

    const auto& mission_file = make_file(FileType::mission);
    const auto& mission_label = make_label();

    insert_label_loc(mission_label, mission_file, mission_label_offset);
    insert_file_loc(mission_file, mission_file_offset);

    SUBCASE("reference from main causes an error")
    {
        const auto& main_file = make_file(FileType::main);
        insert_fixup_entry(mission_label, main_file, label_ref_offset);
        fail_to_relocate_one(gta3sc::codegen::diag::label_ref_across_segments);
    }

    SUBCASE("reference from main extension file causes an error")
    {
        const auto& main_extension_file = make_file(FileType::main_extension);
        insert_fixup_entry(mission_label, main_extension_file,
                           label_ref_offset);
        fail_to_relocate_one(gta3sc::codegen::diag::label_ref_across_segments);
    }

    SUBCASE("reference from subscript file causes an error")
    {
        const auto& subscript_file = make_file(FileType::subscript);
        insert_fixup_entry(mission_label, subscript_file, label_ref_offset);
        fail_to_relocate_one(gta3sc::codegen::diag::label_ref_across_segments);
    }

    SUBCASE("reference from the mission script file itself is okay")
    {
        insert_fixup_entry(mission_label, mission_file, label_ref_offset);
        REQUIRE(relocate_one_from_fixup_table()
                == relative_mission_label_offset);
    }

    SUBCASE("reference from another mission script file causes an error")
    {
        const auto& another_mission_file = make_file(FileType::mission);
        insert_fixup_entry(mission_label, another_mission_file,
                           label_ref_offset);
        fail_to_relocate_one(gta3sc::codegen::diag::label_ref_across_segments);
    }
}

TEST_CASE_FIXTURE(RelocationTableFixture,
                  "cannot reference the local zero offset")
{
    constexpr auto mission_file_offset = 100;
    constexpr auto mission_label_offset = 100;
    constexpr auto label_ref_offset = 120;

    const auto& mission_file = make_file(FileType::mission);
    const auto& mission_label = make_label();

    insert_label_loc(mission_label, mission_file, mission_label_offset);
    insert_file_loc(mission_file, mission_file_offset);

    insert_fixup_entry(mission_label, mission_file, label_ref_offset);
    fail_to_relocate_one(gta3sc::codegen::diag::label_at_local_zero_offset);
}

TEST_CASE_FIXTURE(RelocationTableFixture,
                  "can reference the absolute zero offset")
{
    constexpr auto main_file_offset = 0;
    constexpr auto main_label_offset = 0;
    constexpr auto label_ref_offset = 120;

    const auto& main_file = make_file(FileType::main);
    const auto& main_label = make_label();

    insert_label_loc(main_label, main_file, main_label_offset);
    insert_file_loc(main_file, main_file_offset);

    insert_fixup_entry(main_label, main_file, label_ref_offset);
    REQUIRE(relocate_one_from_fixup_table() == main_label_offset);
}

TEST_CASE_FIXTURE(RelocationTableFixture,
                  "relocation of file offsets are absolute")
{
    constexpr auto mission_file_offset = 100;
    constexpr auto file_ref_offset = 120;

    const auto& mission_file = make_file(FileType::mission);

    insert_file_loc(mission_file, mission_file_offset);

    insert_fixup_entry(mission_file, file_ref_offset);
    REQUIRE(relocate_one_from_file_fixup_table() == mission_file_offset);
}

TEST_CASE_FIXTURE(RelocationTableFixture, "is_in_main_segment")
{
    SUBCASE("main file is in main segment")
    {
        const auto& main_file = make_file(FileType::main);
        REQUIRE(RelocationTable::is_in_main_segment(main_file) == true);
    }

    SUBCASE("main extension file is in main segment")
    {
        const auto& main_extension_file = make_file(FileType::main_extension);
        REQUIRE(RelocationTable::is_in_main_segment(main_extension_file)
                == true);
    }

    SUBCASE("subscript file is in main segment")
    {
        const auto& subscript_file = make_file(FileType::subscript);
        REQUIRE(RelocationTable::is_in_main_segment(subscript_file) == true);
    }

    SUBCASE("mission file is not in main segment")
    {
        const auto& mission_file = make_file(FileType::mission);
        REQUIRE(RelocationTable::is_in_main_segment(mission_file) == false);
    }
}
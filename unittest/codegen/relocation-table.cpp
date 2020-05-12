#include <doctest/doctest.h>
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <queue>

using gta3sc::ArenaMemoryResource;
using gta3sc::Diag;
using gta3sc::DiagnosticHandler;
using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using gta3sc::codegen::RelocationTable;
using FileType = gta3sc::SymbolTable::FileType;

namespace
{
class RelocationTableFixture
{
public:
    RelocationTableFixture() :
        diagman([this](const auto& diag) { diags.push(diag); }),
        symtable(&arena)
    {}

    RelocationTableFixture(const RelocationTableFixture&) = delete;
    auto operator=(const RelocationTableFixture&)
            -> RelocationTableFixture& = delete;

    RelocationTableFixture(RelocationTableFixture&&) = delete;
    auto operator=(RelocationTableFixture &&)
            -> RelocationTableFixture& = delete;

    ~RelocationTableFixture() { CHECK(diags.empty()); }

    auto make_label() -> const SymbolTable::Label&
    {
        const auto [label, inserted] = symtable.insert_label(
                std::to_string(next_label_id++), SymbolTable::global_scope,
                SourceManager::no_source_range);
        REQUIRE(inserted);
        return *label;
    }

    auto make_file(FileType type) -> const SymbolTable::File&
    {
        const auto [file, inserted] = symtable.insert_file(
                std::to_string(next_file_id++), type,
                SourceManager::no_source_range);
        REQUIRE(inserted);
        return *file;
    }

    void insert_label_loc(const SymbolTable::Label& label,
                          const SymbolTable::File& file,
                          RelocationTable::AbsoluteOffset offset)
    {
        REQUIRE(reloc_table.insert_label_loc(label, file, offset));
    }

    void insert_file_loc(const SymbolTable::File& file,
                         RelocationTable::AbsoluteOffset offset)
    {
        REQUIRE(reloc_table.insert_file_loc(file, offset));
    }

    void insert_fixup_entry(const SymbolTable::Label& label,
                            const SymbolTable::File& origin,
                            RelocationTable::AbsoluteOffset offset)
    {
        reloc_table.insert_fixup_entry(label, origin, offset);
    }

    void insert_fixup_entry(const SymbolTable::File& file,
                            RelocationTable::AbsoluteOffset offset)
    {
        reloc_table.insert_fixup_entry(file, offset);
    }

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

    void fail_to_relocate_one(Diag reason)
    {
        const auto view = reloc_table.fixup_table();
        REQUIRE(view.size() == 1);
        REQUIRE(reloc_table.relocate(view.front(), diagman) == std::nullopt);
        REQUIRE(!diags.empty());
        REQUIRE(diags.front().message == reason);
        diags.pop();
    }

private:
    ArenaMemoryResource arena;
    DiagnosticHandler diagman;
    SymbolTable symtable;
    RelocationTable reloc_table;
    uint32_t next_label_id{};
    uint32_t next_file_id{};
    std::queue<gta3sc::Diagnostic> diags;
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
        fail_to_relocate_one(Diag::codegen_label_ref_across_segments);
    }

    SUBCASE("reference from main extension file causes an error")
    {
        const auto& main_extension_file = make_file(FileType::main_extension);
        insert_fixup_entry(mission_label, main_extension_file,
                           label_ref_offset);
        fail_to_relocate_one(Diag::codegen_label_ref_across_segments);
    }

    SUBCASE("reference from subscript file causes an error")
    {
        const auto& subscript_file = make_file(FileType::subscript);
        insert_fixup_entry(mission_label, subscript_file, label_ref_offset);
        fail_to_relocate_one(Diag::codegen_label_ref_across_segments);
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
        fail_to_relocate_one(Diag::codegen_label_ref_across_segments);
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
    fail_to_relocate_one(Diag::codegen_label_at_local_zero_offset);
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

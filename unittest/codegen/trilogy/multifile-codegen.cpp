#include "common-codegen-fixture.hpp"
#include <concepts>
#include <doctest/doctest.h>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <gta3sc/codegen/trilogy/emitter.hpp>
#include <gta3sc/codegen/trilogy/multifile-codegen.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <utility>

using gta3sc::LinkedIR;
using gta3sc::no_file_range;
using gta3sc::SemaIR;
using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using VarType = SymbolTable::VarType;
using FileType = SymbolTable::FileType;
using gta3sc::codegen::RelocationTable;
using gta3sc::codegen::trilogy::CodeEmitter;
using gta3sc::codegen::trilogy::MultifileCodeGen;
using gta3sc::test::codegen::trilogy::CommonCodeGenFixture;

namespace
{
// Header layout constants (byte offsets into the generated binary output).
//
//   [0..7]   global-var chunk  (8 bytes; grows with global variables)
//   [8..43]  used-object chunk (36 bytes; grows with used objects)
//   [44..]   mission chunk     (20 bytes base; grows with missions)
//
constexpr RelocationTable::AbsoluteOffset base_header_size = 8 + 36 + 20; // 64

// Per-element growth amounts for each header chunk.
constexpr RelocationTable::AbsoluteOffset per_global_var_slot
        = 4; // INT/FLOAT = 1 slot; TEXT_LABEL = 2
constexpr RelocationTable::AbsoluteOffset per_used_object_slot
        = 24; // bytes per used-object entry
constexpr RelocationTable::AbsoluteOffset per_mission_slot
        = 4; // bytes per mission entry

// Byte offsets of fields inside the mission chunk.
// The chunk header is: opcode(2) + i32_arg(5) + align(1) = 8 bytes.
// clang-format off
constexpr size_t main_seg_size_offset = 44 + 2 + 5 + 1;                 // = 52
constexpr size_t largest_script_size_offset = main_seg_size_offset + 4; // = 56
constexpr size_t num_missions_offset = main_seg_size_offset + 8;        // = 60 (u16)
constexpr size_t mission_offsets_begin = main_seg_size_offset + 12;     // = 64
// clang-format on

class MultifileCodeGenFixture : public CommonCodeGenFixture
{
protected:
    auto make_file(FileType type, std::string_view basename)
            -> std::pair<const SymbolTable::File&, const SymbolTable::Label&>
    {
        const auto [file, file_inserted] = symtable.insert_file(basename, type,
                                                                no_file_range);
        REQUIRE(file_inserted);

        const auto label_name = std::string("@@").append(basename);
        const auto [label, label_inserted] = symtable.insert_label(
                label_name, SymbolTable::global_scope, no_file_range);
        REQUIRE(label_inserted);

        return {*file, *label};
    }

    auto make_file_label(FileType type,
                         std::string_view basename) -> const SymbolTable::Label&
    {
        return make_file(type, basename).second;
    }

    auto
    make_used_object(std::string_view name) -> const SymbolTable::UsedObject&
    {
        const auto [uobj, inserted] = symtable.insert_used_object(
                name, no_file_range);
        REQUIRE(inserted);
        return *uobj;
    }

    auto generate_code(const LinkedIR<SemaIR>& ir) -> std::vector<std::byte>
    {
        MultifileCodeGen gen(symtable, make_storage_table(), diagman);
        std::vector<std::byte> output;
        REQUIRE(gen.generate(ir, reloc_table, output));
        return output;
    }

    void fail_to_generate_code(const LinkedIR<SemaIR>& ir)
    {
        MultifileCodeGen gen(symtable, make_storage_table(), diagman);
        std::vector<std::byte> output;
        REQUIRE_FALSE(gen.generate(ir, reloc_table, output));
        CHECK(gen.has_error());
    }

    auto compute_header_size() -> RelocationTable::AbsoluteOffset
    {
        MultifileCodeGen gen(symtable, make_storage_table(), diagman);
        const auto size = gen.compute_header_stubs();
        REQUIRE(size != std::nullopt);
        return *size;
    }
};

template<std::unsigned_integral T>
auto read_le(const std::vector<std::byte>& data, size_t offset) -> T
{
    REQUIRE(data.size() >= offset + sizeof(T));
    T v{};
    for(size_t i = 0; i < sizeof(T); ++i)
        v = static_cast<T>(
                v
                | (static_cast<T>(std::to_integer<uint8_t>(data[offset + i]))
                   << (8 * i)));
    return v;
}
} // namespace

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "compute header stubs")
{
    SUBCASE("no extras")
    {
        REQUIRE(compute_header_size() == base_header_size);
    }

    SUBCASE("2 int vars")
    {
        make_var(VarType::INT);
        make_var(VarType::INT);

        REQUIRE(compute_header_size()
                == base_header_size + 2 * per_global_var_slot);
    }

    SUBCASE("1 float var")
    {
        make_var(VarType::FLOAT);

        REQUIRE(compute_header_size()
                == base_header_size + per_global_var_slot);
    }

    SUBCASE("1 text label var")
    {
        make_var(VarType::TEXT_LABEL);

        REQUIRE(compute_header_size()
                == base_header_size + 2 * per_global_var_slot);
    }

    SUBCASE("1 used object")
    {
        make_used_object("OBJ");

        REQUIRE(compute_header_size()
                == base_header_size + per_used_object_slot);
    }

    SUBCASE("1 mission")
    {
        make_file_label(FileType::mission, "MISS.SC");

        REQUIRE(compute_header_size() == base_header_size + per_mission_slot);
    }

    SUBCASE("4 vars, 3 used objects, 2 missions")
    {
        make_var(VarType::INT);        // 1 slot
        make_var(VarType::FLOAT);      // 1 slot
        make_var(VarType::TEXT_LABEL); // 2 slots
        make_var(VarType::INT);        // 1 slot
        make_used_object("CAR");
        make_used_object("CHOPPER");
        make_used_object("BIGMOOSE");
        make_file_label(FileType::mission, "MISS.SC");
        make_file_label(FileType::mission, "MISS2.SC");

        REQUIRE(compute_header_size()
                == base_header_size + 5 * per_global_var_slot
                           + 3 * per_used_object_slot + 2 * per_mission_slot);
    }
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "generate single main file")
{
    const auto& return_command = find_command("RETURN");
    const auto [main_file, main_label] = make_file(FileType::main, "MAIN.SC");

    const auto output = generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(return_command).build()});

    REQUIRE(output.size() == static_cast<size_t>(base_header_size) + 2);

    REQUIRE(reloc_table.files().size() == 1);
    REQUIRE(reloc_table.files().front().file == &main_file);
    REQUIRE(reloc_table.files().front().offset == base_header_size);
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "global var chunk header bytes")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");

    SUBCASE("no extra variables")
    {
        std::vector<std::byte> expected_prefix;
        CodeEmitter()
                .emit_opcode(0x0002)
                .emit_i32(8)
                .emit_raw_byte(std::byte{0})
                .emit_opcode(0x0002) // used-object chunk follows
                .drain(std::back_inserter(expected_prefix));

        const auto output = generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        REQUIRE(output.size() >= expected_prefix.size());
        REQUIRE(std::equal(expected_prefix.begin(), expected_prefix.end(),
                           output.begin()));
    }

    SUBCASE("two int variables expand the global chunk to 16 bytes")
    {
        make_var(VarType::INT);
        make_var(VarType::INT);

        std::vector<std::byte> expected_prefix;
        CodeEmitter()
                .emit_opcode(0x0002)
                .emit_i32(16)
                .emit_raw_byte(std::byte{0})
                .emit_fill(std::byte{0}, 8)
                .emit_opcode(0x0002) // used-object chunk follows
                .drain(std::back_inserter(expected_prefix));

        const auto output = generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        REQUIRE(output.size() >= expected_prefix.size());
        REQUIRE(std::equal(expected_prefix.begin(), expected_prefix.end(),
                           output.begin()));
    }
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "used object chunk header bytes")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");

    // Used-object chunk starts at byte 8.
    // Layout: opcode(2) + i32(5) + align(1) + count_u32(4) + name_slots(N*24).
    constexpr size_t used_chunk_start = 8;
    constexpr size_t count_field = used_chunk_start + 8; // u32 at +8 in chunk
    constexpr size_t first_slot = used_chunk_start + 12; // first name slot

    SUBCASE("no used objects: count is 1 (dummy sentinel only)")
    {
        const auto output = generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        REQUIRE(read_le<uint32_t>(output, count_field) == 1);
    }

    SUBCASE("2 used objects of different lengths")
    {
        make_used_object("CAR");
        make_used_object("CHOPPER");

        const auto output = generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        // Dummy sentinel + CAR + CHOPPER = 3.
        REQUIRE(read_le<uint32_t>(output, count_field) == 3);

        // CAR is in the second slot (first slot is the dummy).
        const size_t car_slot = first_slot + 24;
        REQUIRE(std::string_view(
                        reinterpret_cast<const char*>(output.data() + car_slot),
                        3)
                == "CAR");

        // CHOPPER is in the third slot.
        const size_t chopper_slot = first_slot + 48;
        REQUIRE(std::string_view(reinterpret_cast<const char*>(output.data()
                                                               + chopper_slot),
                                 7)
                == "CHOPPER");
    }
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture,
                  "mission chunk fields without missions")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");

    const auto output = generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(return_command).build()});

    REQUIRE(read_le<uint32_t>(output, main_seg_size_offset)
            == base_header_size + 2);
    REQUIRE(read_le<uint32_t>(output, largest_script_size_offset) == 0);
    REQUIRE(read_le<uint16_t>(output, num_missions_offset) == 0);
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "generate main file and one mission")
{
    const auto& return_command = find_command("RETURN");
    const auto [main_file, main_label] = make_file(FileType::main, "MAIN.SC");
    const auto [mission_file, mission_label] = make_file(FileType::mission,
                                                         "M00.SC");

    // header: global(8) + used(36) + mission(20 + 1*per_mission_slot)
    const RelocationTable::AbsoluteOffset header_size = base_header_size
                                                        + per_mission_slot;

    const auto output = generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&mission_label).build(),
            SemaIR::Builder(&arena).command(return_command).build()});

    // main body = 2 bytes (RETURN), mission body = 2 bytes (RETURN)
    REQUIRE(output.size() == static_cast<size_t>(header_size) + 2 + 2);

    REQUIRE(reloc_table.files().size() == 2);
    auto file_it = reloc_table.files().begin();
    REQUIRE(file_it->file == &main_file);
    REQUIRE(file_it->offset == header_size);
    ++file_it;
    REQUIRE(file_it->file == &mission_file);
    REQUIRE(file_it->offset == header_size + 2);

    REQUIRE(read_le<uint32_t>(output, main_seg_size_offset) == header_size + 2);
    REQUIRE(read_le<uint32_t>(output, largest_script_size_offset) == 2);
    REQUIRE(read_le<uint16_t>(output, num_missions_offset) == 1);

    REQUIRE(read_le<uint32_t>(output, mission_offsets_begin)
            == header_size + 2);
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture,
                  "main extension and subscript count toward main segment")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");
    const auto& ext_label = make_file_label(FileType::main_extension, "EXT.SC");
    const auto& sub_label = make_file_label(FileType::subscript, "SUB.SC");

    const auto output = generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&ext_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&sub_label).build(),
            SemaIR::Builder(&arena).command(return_command).build()});

    // 3 files * 2 bytes each = 6 bytes of main segment
    REQUIRE(read_le<uint32_t>(output, main_seg_size_offset)
            == base_header_size + 6);
    REQUIRE(read_le<uint32_t>(output, largest_script_size_offset) == 0);
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture,
                  "largest mission script size reflects the largest mission")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");
    const auto& m1_label = make_file_label(FileType::mission, "M01.SC");
    const auto& m2_label = make_file_label(FileType::mission, "M02.SC");
    const auto& m3_label = make_file_label(FileType::mission, "M03.SC");

    // header: global(8) + used(36) + mission(20 + 3*per_mission_slot)
    const RelocationTable::AbsoluteOffset header_size = base_header_size
                                                        + 3 * per_mission_slot;

    // M01: 2 bytes, M02: 6 bytes (largest, in the middle), M03: 4 bytes.
    const auto output = generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&m1_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&m2_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).label(&m3_label).build(),
            SemaIR::Builder(&arena).command(return_command).build(),
            SemaIR::Builder(&arena).command(return_command).build()});

    REQUIRE(output.size() == static_cast<size_t>(header_size) + 2 + 2 + 6 + 4);
    REQUIRE(read_le<uint32_t>(output, largest_script_size_offset) == 6);
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "used object name length boundary")
{
    const auto& return_command = find_command("RETURN");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");

    SUBCASE("23 characters is accepted")
    {
        // 23 chars — max allowed (slot is 24 bytes, last byte = '\0').
        make_used_object("ABCDEFGHIJKLMNOPQRSTUVW");

        const auto output = generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        REQUIRE(output.size()
                == static_cast<size_t>(base_header_size + per_used_object_slot)
                           + 2);
    }

    SUBCASE("24 characters emits diagnostic")
    {
        // 24 chars — one over the limit.
        make_used_object("ABCDEFGHIJKLMNOPQRSTUVWX");

        fail_to_generate_code(LinkedIR<SemaIR>{
                SemaIR::Builder(&arena).label(&main_label).build(),
                SemaIR::Builder(&arena).command(return_command).build()});

        REQUIRE(consume_diag().descriptor
                == &gta3sc::codegen::trilogy::diag::used_object_name_too_long);
    }
}

TEST_CASE_FIXTURE(MultifileCodeGenFixture, "generate tracks codegen failures")
{
    const auto& bad_command = find_command("COMMAND_WITHOUT_ID");
    const auto& main_label = make_file_label(FileType::main, "MAIN.SC");

    fail_to_generate_code(LinkedIR<SemaIR>{
            SemaIR::Builder(&arena).label(&main_label).build(),
            SemaIR::Builder(&arena).command(bad_command).build()});

    REQUIRE(consume_diag().descriptor
            == &gta3sc::codegen::diag::target_does_not_support_command);
}

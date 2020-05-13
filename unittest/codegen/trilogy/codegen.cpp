#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/codegen/storage-table.hpp>
#include <gta3sc/codegen/trilogy/codegen.hpp>
#include <queue>

using gta3sc::ArenaMemoryResource;
using gta3sc::CommandTable;
using gta3sc::Diag;
using gta3sc::DiagnosticHandler;
using gta3sc::LinkedIR;
using gta3sc::SemaIR;
using gta3sc::SourceManager;
using gta3sc::SymbolTable;
using VarType = SymbolTable::VarType;
using FileType = SymbolTable::FileType;
using gta3sc::codegen::RelocationTable;
using gta3sc::codegen::StorageTable;
using gta3sc::codegen::trilogy::CodeEmitter;
using gta3sc::codegen::trilogy::CodeGen;
using gta3sc::test::CommandTableFixture;

namespace
{
class CodeGenFixture : public CommandTableFixture
{
public:
    CodeGenFixture() : symtable(&arena) {}

    CodeGenFixture(const CodeGenFixture&) = delete;
    auto operator=(const CodeGenFixture&) -> CodeGenFixture& = delete;

    CodeGenFixture(CodeGenFixture&&) = delete;
    auto operator=(CodeGenFixture &&) -> CodeGenFixture& = delete;

    ~CodeGenFixture() { CHECK(diags.empty()); }

protected:
    auto make_scope() -> SymbolTable::ScopeId { return symtable.new_scope(); }

    auto make_lvar(VarType var_type, SymbolTable::ScopeId scope_id)
            -> const SymbolTable::Variable&
    {
        const auto [var, inserted] = symtable.insert_var(
                std::to_string(next_symbol_id++), scope_id, var_type,
                std::nullopt, SourceManager::no_source_range);
        REQUIRE(inserted);
        return *var;
    }

    auto make_var(VarType var_type) -> const SymbolTable::Variable&
    {
        return make_lvar(var_type, SymbolTable::global_scope);
    }

    auto make_label() -> const SymbolTable::Label&
    {
        const auto [label, inserted] = symtable.insert_label(
                std::to_string(next_symbol_id++), SymbolTable::global_scope,
                SourceManager::no_source_range);
        REQUIRE(inserted);
        return *label;
    }

    auto make_file(FileType file_type) -> const SymbolTable::File&
    {
        const auto [file, inserted] = symtable.insert_file(
                std::to_string(next_symbol_id++), file_type,
                SourceManager::no_source_range);
        REQUIRE(inserted);
        return *file;
    }

    auto make_used_object() -> const SymbolTable::UsedObject&
    {
        const auto [uobj, inserted] = symtable.insert_used_object(
                std::to_string(next_symbol_id++),
                SourceManager::no_source_range);
        REQUIRE(inserted);
        return *uobj;
    }

    auto make_storage_table() -> StorageTable
    {
        return StorageTable::from_symbols(symtable, StorageTable::Options())
                .value();
    }

    template<typename IR>
    auto generate_code(uint32_t multifile_offset, const IR& ir)
            -> std::vector<std::byte>
    {
        const auto& file = codegen_file();
        const auto storage_table = make_storage_table();

        DiagnosticHandler diagman(
                [this](const auto& diag) { diags.push(std::move(diag)); });

        auto codegen = CodeGen(file, multifile_offset, storage_table, diagman);

        std::vector<std::byte> output;
        const auto result = codegen.generate(ir, reloc_table,
                                             std::back_inserter(output));

        REQUIRE(result != std::nullopt);
        return output;
    }

    template<typename IR>
    auto generate_code(const IR& ir) -> std::vector<std::byte>
    {
        return generate_code(0, ir);
    }

    template<typename IR>
    void fail_to_generate_code(const IR& ir)
    {
        const auto& file = codegen_file();
        const auto storage_table = make_storage_table();

        DiagnosticHandler diagman(
                [this](const auto& diag) { diags.push(std::move(diag)); });

        auto codegen = CodeGen(file, 0, storage_table, diagman);

        std::vector<std::byte> output;
        const auto result = codegen.generate(ir, reloc_table,
                                             std::back_inserter(output));

        REQUIRE(result == std::nullopt);
    }

    auto find_command(std::string_view name) -> const CommandTable::CommandDef&
    {
        const auto command = cmdman.find_command(name);
        REQUIRE(command != nullptr);
        return *command;
    }

    auto find_constant(std::string_view enum_name,
                       std::string_view constant_name)
            -> const CommandTable::ConstantDef&
    {
        const auto enum_id = cmdman.find_enumeration(enum_name);
        REQUIRE(enum_id != std::nullopt);
        const auto constant = cmdman.find_constant(*enum_id, constant_name);
        REQUIRE(constant != nullptr);
        return *constant;
    }

    auto codegen_file() -> const SymbolTable::File&
    {
        constexpr std::string_view test_filename = "A.SC";
        const auto [file, _] = symtable.insert_file(
                test_filename, FileType::main, SourceManager::no_source_range);
        CHECK(file != nullptr);
        return *file;
    }

    auto consume_diag() -> gta3sc::Diagnostic
    {
        auto front = peek_diag();
        this->diags.pop();
        return front;
    }

    auto peek_diag() -> const gta3sc::Diagnostic&
    {
        REQUIRE(!this->diags.empty());
        return this->diags.front();
    }

protected:
    ArenaMemoryResource arena;   // NOLINT: Protected data is controlled
    RelocationTable reloc_table; // NOLINT: within this file.

private:
    uint32_t next_symbol_id{};
    SymbolTable symtable;
    std::queue<gta3sc::Diagnostic> diags;
};
} // namespace

TEST_CASE_FIXTURE(CodeGenFixture, "generating command")
{
    const auto& return_command = find_command("RETURN");
    const auto return_opcode = return_command.target_id().value();

    CHECK(return_opcode == 0x0051);

    SUBCASE("command emits opcode")
    {
        std::vector<std::byte> expected;
        CodeEmitter()
                .emit_opcode(return_opcode)
                .drain(std::back_insert_iterator(expected));

        const auto output = generate_code(
                *SemaIR::Builder(&arena).command(return_command).build());

        REQUIRE(output == expected);
    }

    SUBCASE("NOTed command emits opcode with hibit set")
    {
        std::vector<std::byte> expected;
        CodeEmitter()
                .emit_opcode(return_opcode, true)
                .drain(std::back_insert_iterator(expected));

        const auto output = generate_code(*SemaIR::Builder(&arena)
                                                   .not_flag()
                                                   .command(return_command)
                                                   .build());

        REQUIRE(output == expected);
    }

    SUBCASE("generating an opcode doesn't populate relocation table")
    {
        generate_code(*SemaIR::Builder(&arena).command(return_command).build());

        REQUIRE(reloc_table.labels().empty());
        REQUIRE(reloc_table.files().empty());
        REQUIRE(reloc_table.fixup_table().empty());
        REQUIRE(reloc_table.file_fixup_table().empty());
    }

    SUBCASE("generating an unhandled command produces an error")
    {
        const auto& flash_radar_blip_command = find_command("FLASH_RADAR_BLIP");
        CHECK(flash_radar_blip_command.target_id() != std::nullopt);
        CHECK(flash_radar_blip_command.target_handled() == false);

        fail_to_generate_code(*SemaIR::Builder(&arena)
                                       .command(flash_radar_blip_command)
                                       .build());
        REQUIRE(consume_diag().message
                == Diag::codegen_target_does_not_support_command);
    }

    SUBCASE("generating a command without id produces an error")
    {
        const auto& command_without_id = find_command("COMMAND_WITHOUT_ID");
        CHECK(command_without_id.target_id() == std::nullopt);
        CHECK(command_without_id.target_handled() == true);

        fail_to_generate_code(
                *SemaIR::Builder(&arena).command(command_without_id).build());
        REQUIRE(consume_diag().message
                == Diag::codegen_target_does_not_support_command);
    }
}

TEST_CASE_FIXTURE(CodeGenFixture, "generating label definition")
{
    constexpr auto codegen_offset = 100;
    constexpr auto opcode_size = sizeof(uint16_t);
    const auto& return_command = find_command("RETURN");
    const auto& label_before_command = make_label();
    const auto& label_in_command = make_label();
    const auto& label_after_command = make_label();

    const auto output = generate_code(
            codegen_offset,
            LinkedIR<SemaIR>{SemaIR::Builder(&arena)
                                     .label(&label_before_command)
                                     .build(),
                             SemaIR::Builder(&arena)
                                     .label(&label_in_command)
                                     .command(return_command)
                                     .build(),
                             SemaIR::Builder(&arena)
                                     .label(&label_after_command)
                                     .build()});

    REQUIRE(output.size() == opcode_size);

    REQUIRE(reloc_table.labels().size() == 3);
    REQUIRE(reloc_table.fixup_table().empty());
    REQUIRE(reloc_table.file_fixup_table().empty());

    const auto& label0_info = reloc_table.labels().front();
    REQUIRE(label0_info.label == &label_before_command);
    REQUIRE(label0_info.offset == codegen_offset);
    REQUIRE(label0_info.origin_file == &codegen_file());

    const auto& label2_info = reloc_table.labels().back();
    REQUIRE(label2_info.label == &label_after_command);
    REQUIRE(label2_info.offset == codegen_offset + opcode_size);
    REQUIRE(label2_info.origin_file == &codegen_file());

    SUBCASE("label in the same IR instruction as command has offset before "
            "command")
    {
        const auto& label1_info = *std::next(reloc_table.labels().begin());
        REQUIRE(label1_info.label == &label_in_command);
        REQUIRE(label1_info.offset == codegen_offset);
    }
}

TEST_CASE_FIXTURE(CodeGenFixture, "integer argument emits integer data")
{
    constexpr auto wait_time = 1000;
    const auto& wait_command = find_command("WAIT");
    const auto wait_opcode = wait_command.target_id().value();

    CHECK(wait_opcode == 0x0001);

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(wait_opcode)
            .emit_int(wait_time)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(wait_command)
                                               .arg_int(wait_time)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture,
                  "floating-point argument emits Q11.4 fixed-point data")
{
    constexpr auto time_scale = 0.3;
    const auto& set_time_scale_command = find_command("SET_TIME_SCALE");
    const auto set_time_scale_opcode
            = set_time_scale_command.target_id().value();

    CHECK(set_time_scale_opcode == 0x015D);

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(set_time_scale_opcode)
            .emit_q11_4(time_scale)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(set_time_scale_command)
                                               .arg_float(time_scale)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture, "text label argument emits 8 bytes of data")
{
    const auto text_label_output_size = 8;
    const std::string_view text_label = "HELLO";
    const auto& print_help_command = find_command("PRINT_HELP");
    const auto print_help_opcode = print_help_command.target_id().value();

    CHECK(print_help_opcode == 0x03E5);

    CHECK(text_label.size()
          < text_label_output_size); // wanna check if will emit null bytes

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(print_help_opcode)
            .emit_raw_bytes(text_label.begin(), text_label.end(),
                            text_label_output_size)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(print_help_command)
                                               .arg_text_label(text_label)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture, "string argument emits 128 bytes of data")
{
    const auto string_output_size = 128;
    const std::string_view string = "Hello World";
    const auto& save_string_to_debug_file_command = find_command(
            "SAVE_STRING_TO_DEBUG_FILE");
    const auto save_string_to_debug_file_opcode
            = save_string_to_debug_file_command.target_id().value();

    CHECK(save_string_to_debug_file_opcode == 0x05B6);

    CHECK(string.size()
          < string_output_size); // wanna check if will emit null bytes

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(save_string_to_debug_file_opcode)
            .emit_raw_bytes(string.begin(), string.end(), string_output_size)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(
            *SemaIR::Builder(&arena)
                     .command(save_string_to_debug_file_command)
                     .arg_string(string)
                     .build());

    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture,
                  "global variable argument emits global variable offset")
{
    const auto& do_fade_command = find_command("DO_FADE");
    const auto do_fade_opcode = do_fade_command.target_id().value();

    CHECK(do_fade_opcode == 0x016A);

    const auto& var_int = make_var(VarType::INT);
    const auto& var_float = make_var(VarType::FLOAT);
    const auto storage = make_storage_table();

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(do_fade_opcode)
            .emit_var(storage.var_index(var_int) * 4)
            .emit_var(storage.var_index(var_float) * 4)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(do_fade_command)
                                               .arg_var(var_int)
                                               .arg_var(var_float)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture,
                  "local variable argument emits local variable index")
{
    const auto& do_fade_command = find_command("DO_FADE");
    const auto do_fade_opcode = do_fade_command.target_id().value();

    CHECK(do_fade_opcode == 0x016A);

    const auto scope1_id = make_scope();
    const auto scope2_id = make_scope();
    const auto& lvar_int_scope1 = make_lvar(VarType::INT, scope1_id);
    const auto& lvar_float_scope1 = make_lvar(VarType::FLOAT, scope1_id);
    const auto& lvar_int_scope2 = make_lvar(VarType::INT, scope2_id);
    const auto& lvar_float_scope2 = make_lvar(VarType::FLOAT, scope2_id);
    const auto storage = make_storage_table();

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(do_fade_opcode)
            .emit_lvar(storage.var_index(lvar_int_scope1))
            .emit_lvar(storage.var_index(lvar_float_scope2))
            .emit_opcode(do_fade_opcode)
            .emit_lvar(storage.var_index(lvar_float_scope1))
            .emit_lvar(storage.var_index(lvar_int_scope2))
            .drain(std::back_inserter(expected));

    const auto output = generate_code(
            LinkedIR<SemaIR>{SemaIR::Builder(&arena)
                                     .command(do_fade_command)
                                     .arg_var(lvar_int_scope1)
                                     .arg_var(lvar_float_scope2)
                                     .build(),
                             SemaIR::Builder(&arena)
                                     .command(do_fade_command)
                                     .arg_var(lvar_float_scope1)
                                     .arg_var(lvar_int_scope1)
                                     .build()});
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture, "string constant argument emits integer data")
{
    constexpr auto fade_time = 1000;
    const auto& do_fade_command = find_command("DO_FADE");
    const auto do_fade_opcode = do_fade_command.target_id().value();
    const auto& fade_in_constant = find_constant("FADE", "FADE_IN");

    CHECK(do_fade_opcode == 0x016A);

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(do_fade_opcode)
            .emit_int(fade_time)
            .emit_int(fade_in_constant.value())
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(do_fade_command)
                                               .arg_int(fade_time)
                                               .arg_const(fade_in_constant)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture, "used object argument emits integer data")
{
    constexpr auto dummy_arg = 0;
    const auto& create_object_command = find_command("CREATE_OBJECT");
    const auto create_object_opcode = create_object_command.target_id().value();
    const auto& used_obj1 = make_used_object();
    const auto& used_obj2 = make_used_object();
    const auto& used_obj3 = make_used_object();
    const auto used_obj1_as_int = -(used_obj1.id() + 1);
    const auto used_obj2_as_int = -(used_obj2.id() + 1);
    const auto used_obj3_as_int = -(used_obj3.id() + 1);

    CHECK(create_object_opcode == 0x0107);

    CHECK(used_obj1_as_int == -1);
    CHECK(used_obj2_as_int == -2);
    CHECK(used_obj3_as_int == -3);

    // So, actually we are going to construct a semantically invalid IR
    // (because the parameter types of CREATE_OBJECT are disrespected)
    // just so that we can test the id of three used objects in a single go.
    //
    // Not a problem since `CodeGen` doesn't care about semantics.

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(create_object_opcode)
            .emit_int(used_obj3_as_int)
            .emit_int(used_obj1_as_int)
            .emit_int(used_obj2_as_int)
            .emit_int(dummy_arg)
            .emit_int(dummy_arg)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(*SemaIR::Builder(&arena)
                                               .command(create_object_command)
                                               .arg_object(used_obj3)
                                               .arg_object(used_obj1)
                                               .arg_object(used_obj2)
                                               .arg_int(dummy_arg)
                                               .arg_int(dummy_arg)
                                               .build());
    REQUIRE(output == expected);
}

TEST_CASE_FIXTURE(CodeGenFixture, "label argument emits 32-bit zero integer")
{
    constexpr auto codegen_offset = 100;
    constexpr auto opcode_size = sizeof(uint16_t);
    constexpr auto datatype_size = 1;
    const auto& goto_command = find_command("GOTO");
    const auto goto_opcode = goto_command.target_id().value();
    const auto& label = make_label();

    CHECK(goto_opcode == 0x0002);

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(goto_opcode)
            .emit_i32(0)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(codegen_offset,
                                      *SemaIR::Builder(&arena)
                                               .label(&label)
                                               .command(goto_command)
                                               .arg_label(label)
                                               .build());

    REQUIRE(output == expected);

    SUBCASE("inserts fixup entry in the relocation table")
    {
        REQUIRE(reloc_table.fixup_table().size() == 1);
        REQUIRE(reloc_table.file_fixup_table().empty());
        const auto& fixup_entry = reloc_table.fixup_table().front();
        REQUIRE(fixup_entry.label == &label);
        REQUIRE(fixup_entry.offset
                == codegen_offset + opcode_size + datatype_size);
        REQUIRE(fixup_entry.origin_file == &codegen_file());
    }
}

TEST_CASE_FIXTURE(CodeGenFixture, "filename argument emits 32-bit zero integer")
{
    constexpr auto codegen_offset = 100;
    constexpr auto opcode_size = sizeof(uint16_t);
    constexpr auto datatype_size = 1;
    const auto& launch_mission_command = find_command("LAUNCH_MISSION");
    const auto launch_mission_opcode
            = launch_mission_command.target_id().value();
    const auto& other_file = make_file(FileType::subscript);

    CHECK(launch_mission_opcode == 0x00D7);

    std::vector<std::byte> expected;
    CodeEmitter()
            .emit_opcode(launch_mission_opcode)
            .emit_i32(0)
            .drain(std::back_inserter(expected));

    const auto output = generate_code(codegen_offset,
                                      *SemaIR::Builder(&arena)
                                               .command(launch_mission_command)
                                               .arg_filename(other_file)
                                               .build());

    REQUIRE(output == expected);

    SUBCASE("inserts file fixup entry in the relocation table")
    {
        REQUIRE(reloc_table.file_fixup_table().size() == 1);
        REQUIRE(reloc_table.fixup_table().empty());
        const auto& fixup_entry = reloc_table.file_fixup_table().front();
        REQUIRE(fixup_entry.file == &other_file);
        REQUIRE(fixup_entry.offset
                == codegen_offset + opcode_size + datatype_size);
    }
}

TEST_CASE_FIXTURE(CodeGenFixture,
                  "variadic arguments emits null byte at end of it")
{
    const auto& start_new_script_command = find_command("START_NEW_SCRIPT");
    const auto start_new_script_opcode
            = start_new_script_command.target_id().value();
    const auto& label = make_label();

    SUBCASE("no optional arguments provided")
    {
        std::vector<std::byte> expected;
        CodeEmitter()
                .emit_opcode(start_new_script_opcode)
                .emit_i32(0)
                .emit_eoal()
                .drain(std::back_inserter(expected));

        const auto output = generate_code(
                *SemaIR::Builder(&arena)
                         .command(start_new_script_command)
                         .arg_label(label)
                         .build());

        REQUIRE(output == expected);
    }

    SUBCASE("optional arguments provided")
    {
        std::vector<std::byte> expected;
        CodeEmitter()
                .emit_opcode(start_new_script_opcode)
                .emit_i32(0)
                .emit_int(1)
                .emit_int(2)
                .emit_int(3)
                .emit_eoal()
                .drain(std::back_inserter(expected));

        const auto output = generate_code(
                *SemaIR::Builder(&arena)
                         .command(start_new_script_command)
                         .arg_label(label)
                         .arg_int(1)
                         .arg_int(2)
                         .arg_int(3)
                         .build());

        REQUIRE(output == expected);
    }
}

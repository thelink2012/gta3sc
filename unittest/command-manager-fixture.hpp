#pragma once
#include <doctest/doctest.h>
#include <gta3sc/command-manager.hpp>

namespace gta3sc::test
{
class CommandManagerFixture
{
public:
    CommandManagerFixture() : cmdman(build()) {}

private:
    auto build() -> CommandManager::Builder
    {
        using ParamDef = gta3sc::CommandManager::ParamDef;
        using ParamType = gta3sc::CommandManager::ParamType;
        using EntityId = gta3sc::CommandManager::EntityId;
        using EnumId = gta3sc::CommandManager::EnumId;

        CommandManager::Builder builder(arena);

        builder.insert_entity_type("CAR");
        builder.insert_entity_type("CHAR");
        builder.insert_entity_type("OBJECT");

        CHECK(builder.find_entity_type("CAR").value_or(EntityId{0})
              != EntityId{0});
        CHECK(builder.find_entity_type("CHAR").value_or(EntityId{0})
              != EntityId{0});
        CHECK(builder.find_entity_type("OBJECT").value_or(EntityId{0})
              != EntityId{0});

        CHECK(builder.find_enumeration("GLOBAL"));
        builder.insert_enumeration("PEDTYPE");
        builder.insert_enumeration("DEFAULTMODEL");
        builder.insert_enumeration("FADE");
        builder.insert_enumeration("MODEL"); // special

        builder.insert_or_assign_constant(*builder.find_enumeration("GLOBAL"),
                                          "FALSE", 0);
        builder.insert_or_assign_constant(*builder.find_enumeration("GLOBAL"),
                                          "TRUE", 1);
        builder.insert_or_assign_constant(*builder.find_enumeration("GLOBAL"),
                                          "OFF", 0);
        builder.insert_or_assign_constant(*builder.find_enumeration("GLOBAL"),
                                          "ON", 1);

        builder.insert_or_assign_constant(*builder.find_enumeration("PEDTYPE"),
                                          "PEDTYPE_CIVMALE", 4);
        builder.insert_or_assign_constant(*builder.find_enumeration("PEDTYPE"),
                                          "PEDTYPE_CIVFEMALE", 5);
        builder.insert_or_assign_constant(*builder.find_enumeration("PEDTYPE"),
                                          "PEDTYPE_MEDIC", 16);

        builder.insert_or_assign_constant(*builder.find_enumeration("FADE"),
                                          "FADE_OUT", 0);
        builder.insert_or_assign_constant(*builder.find_enumeration("FADE"),
                                          "FADE_IN", 1);

        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "MEDIC", 5);
        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "HFYST", 9);
        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "HMYST", 11);
        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "CHEETAH", 145);
        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "WASHING", 151);
        builder.insert_or_assign_constant(
                *builder.find_enumeration("DEFAULTMODEL"), "LOVEFIST", 201);

        add_command(builder, "{", {});
        add_command(builder, "}", {});
        add_command(builder, "IF", {ParamDef{ParamType::INT}});
        add_command(builder, "ENDIF", {});
        add_command(builder, "REPEAT",
                    {ParamDef{ParamType::INT}, ParamDef{ParamType::VAR_INT}});
        add_command(builder, "ENDREPEAT", {});
        add_command(builder, "WAIT", {ParamDef{ParamType::INPUT_INT}});
        add_command(builder, "GOTO", {ParamDef{ParamType::LABEL}});
        add_command(builder, "GOSUB", {ParamDef{ParamType::LABEL}});
        add_command(builder, "RETURN", {});
        add_command(builder, "SCRIPT_NAME", {ParamDef{ParamType::TEXT_LABEL}});
        add_command(builder, "PRINT_HELP", {ParamDef{ParamType::TEXT_LABEL}});
        add_command(
                builder, "START_NEW_SCRIPT",
                {ParamDef{ParamType::LABEL}, ParamDef{ParamType::INPUT_OPT}});
        add_command(builder, "VAR_INT",
                    {ParamDef{ParamType::VAR_INT},
                     ParamDef{ParamType::VAR_INT_OPT}});
        add_command(builder, "LVAR_INT",
                    {ParamDef{ParamType::LVAR_INT},
                     ParamDef{ParamType::LVAR_INT_OPT}});
        add_command(builder, "VAR_FLOAT",
                    {ParamDef{ParamType::VAR_FLOAT},
                     ParamDef{ParamType::VAR_FLOAT_OPT}});
        add_command(builder, "LVAR_FLOAT",
                    {ParamDef{ParamType::LVAR_FLOAT},
                     ParamDef{ParamType::LVAR_FLOAT_OPT}});
        add_command(builder, "VAR_TEXT_LABEL",
                    {ParamDef{ParamType::VAR_TEXT_LABEL},
                     ParamDef{ParamType::VAR_TEXT_LABEL_OPT}});
        add_command(builder, "LVAR_TEXT_LABEL",
                    {ParamDef{ParamType::LVAR_TEXT_LABEL},
                     ParamDef{ParamType::LVAR_TEXT_LABEL_OPT}});
        add_command(builder, "GENERATE_RANDOM_FLOAT_IN_RANGE",
                    {ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::OUTPUT_FLOAT}});
        add_command(builder, "GENERATE_RANDOM_INT_IN_RANGE",
                    {ParamDef{ParamType::INPUT_INT},
                     ParamDef{ParamType::INPUT_INT},
                     ParamDef{ParamType::OUTPUT_INT}});
        add_command(builder, "SAVE_STRING_TO_DEBUG_FILE",
                    {ParamDef{ParamType::STRING}});
        add_command(builder, "SAVE_STRING_TO_DEBUG_FILE",
                    {ParamDef{ParamType::STRING}});

        add_command(builder, "CREATE_CHAR",
                    {ParamDef{ParamType::INPUT_INT, EntityId{},
                              *builder.find_enumeration("PEDTYPE")},
                     ParamDef{ParamType::INPUT_INT, EntityId{},
                              *builder.find_enumeration("DEFAULTMODEL")},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::OUTPUT_INT,
                              *builder.find_entity_type("CHAR"), EnumId{}}});

        add_command(builder, "CREATE_CAR",
                    {ParamDef{ParamType::INPUT_INT, EntityId{},
                              *builder.find_enumeration("DEFAULTMODEL")},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::OUTPUT_INT,
                              *builder.find_entity_type("CAR"), EnumId{}}});

        add_command(builder, "CREATE_OBJECT",
                    {ParamDef{ParamType::INPUT_INT, EntityId{},
                              *builder.find_enumeration("MODEL")},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::INPUT_FLOAT},
                     ParamDef{ParamType::OUTPUT_INT,
                              *builder.find_entity_type("OBJECT"), EnumId{}}});

        add_command(builder, "DO_FADE",
                    {ParamDef{ParamType::INPUT_INT},
                     ParamDef{ParamType::INPUT_INT, EntityId{},
                              *builder.find_enumeration("FADE")}});

        add_command(
                builder, "SET_CAR_HEADING",
                {
                        ParamDef{ParamType::INPUT_INT,
                                 *builder.find_entity_type("CAR"), EnumId{}},
                        ParamDef{ParamType::INPUT_FLOAT},
                });

        add_command(
                builder, "SET_CHAR_HEADING",
                {
                        ParamDef{ParamType::INPUT_INT,
                                 *builder.find_entity_type("CHAR"), EnumId{}},
                        ParamDef{ParamType::INPUT_FLOAT},
                });

        add_command(builder, "SET_VAR_INT",
                    {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::INT}});
        add_command(
                builder, "SET_VAR_FLOAT",
                {ParamDef{ParamType::VAR_FLOAT}, ParamDef{ParamType::FLOAT}});
        add_command(builder, "SET_LVAR_INT",
                    {ParamDef{ParamType::LVAR_INT}, ParamDef{ParamType::INT}});
        add_command(
                builder, "SET_LVAR_FLOAT",
                {ParamDef{ParamType::LVAR_FLOAT}, ParamDef{ParamType::FLOAT}});
        add_command(
                builder, "SET_VAR_INT_TO_VAR_INT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::VAR_INT}});
        add_command(
                builder, "SET_LVAR_INT_TO_LVAR_INT",
                {ParamDef{ParamType::LVAR_INT}, ParamDef{ParamType::LVAR_INT}});
        add_command(builder, "SET_VAR_FLOAT_TO_VAR_FLOAT",
                    {ParamDef{ParamType::VAR_FLOAT},
                     ParamDef{ParamType::VAR_FLOAT}});
        add_command(builder, "SET_LVAR_FLOAT_TO_LVAR_FLOAT",
                    {ParamDef{ParamType::LVAR_FLOAT},
                     ParamDef{ParamType::LVAR_FLOAT}});
        add_command(builder, "SET_VAR_FLOAT_TO_LVAR_FLOAT",
                    {ParamDef{ParamType::VAR_FLOAT},
                     ParamDef{ParamType::LVAR_FLOAT}});
        add_command(builder, "SET_LVAR_FLOAT_TO_VAR_FLOAT",
                    {ParamDef{ParamType::LVAR_FLOAT},
                     ParamDef{ParamType::VAR_FLOAT}});
        add_command(
                builder, "SET_VAR_INT_TO_LVAR_INT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::LVAR_INT}});
        add_command(
                builder, "SET_LVAR_INT_TO_VAR_INT",
                {ParamDef{ParamType::LVAR_INT}, ParamDef{ParamType::VAR_INT}});
        add_command(
                builder, "SET_VAR_INT_TO_CONSTANT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::INPUT_INT}});
        add_command(
                builder, "SET_LVAR_INT_TO_CONSTANT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::INPUT_INT}});
        add_command(builder, "SET_VAR_TEXT_LABEL",
                    {ParamDef{ParamType::VAR_TEXT_LABEL},
                     ParamDef{ParamType::TEXT_LABEL}});
        add_command(builder, "SET_LVAR_TEXT_LABEL",
                    {ParamDef{ParamType::LVAR_TEXT_LABEL},
                     ParamDef{ParamType::TEXT_LABEL}});

        add_command(builder, "ABS_VAR_INT", {ParamDef{ParamType::VAR_INT}});
        add_command(builder, "ABS_LVAR_INT", {ParamDef{ParamType::LVAR_INT}});
        add_command(builder, "ABS_VAR_FLOAT", {ParamDef{ParamType::VAR_FLOAT}});
        add_command(builder, "ABS_LVAR_FLOAT",
                    {ParamDef{ParamType::LVAR_FLOAT}});

        add_command(builder, "ACCEPTS_ONLY_VAR_INT",
                    {ParamDef{ParamType::VAR_INT}});
        add_command(builder, "ACCEPTS_ONLY_LVAR_INT",
                    {ParamDef{ParamType::LVAR_INT}});
        add_command(builder, "ACCEPTS_ONLY_VAR_FLOAT",
                    {ParamDef{ParamType::VAR_FLOAT}});
        add_command(builder, "ACCEPTS_ONLY_LVAR_FLOAT",
                    {ParamDef{ParamType::LVAR_FLOAT}});

        add_alternator(builder, "SET",
                       {builder.find_command("SET_VAR_INT"),
                        builder.find_command("SET_VAR_FLOAT"),
                        builder.find_command("SET_LVAR_INT"),
                        builder.find_command("SET_LVAR_FLOAT"),
                        builder.find_command("SET_VAR_INT_TO_VAR_INT"),
                        builder.find_command("SET_LVAR_INT_TO_LVAR_INT"),
                        builder.find_command("SET_VAR_FLOAT_TO_VAR_FLOAT"),
                        builder.find_command("SET_LVAR_FLOAT_TO_LVAR_FLOAT"),
                        builder.find_command("SET_VAR_FLOAT_TO_LVAR_FLOAT"),
                        builder.find_command("SET_LVAR_FLOAT_TO_VAR_FLOAT"),
                        builder.find_command("SET_VAR_INT_TO_LVAR_INT"),
                        builder.find_command("SET_LVAR_INT_TO_VAR_INT"),
                        builder.find_command("SET_VAR_INT_TO_CONSTANT"),
                        builder.find_command("SET_LVAR_INT_TO_CONSTANT"),
                        builder.find_command("SET_VAR_TEXT_LABEL"),
                        builder.find_command("SET_LVAR_TEXT_LABEL")});

        add_alternator(builder, "ABS",
                       {builder.find_command("ABS_VAR_INT"),
                        builder.find_command("ABS_LVAR_INT"),
                        builder.find_command("ABS_VAR_FLOAT"),
                        builder.find_command("ABS_LVAR_FLOAT")});

        add_alternator(builder, "ACCEPTS_ONLY_GLOBAL_VAR",
                       {builder.find_command("ACCEPTS_ONLY_VAR_INT"),
                        builder.find_command("ACCEPTS_ONLY_VAR_FLOAT")});

        add_alternator(builder, "ACCEPTS_ONLY_LOCAL_VAR",
                       {builder.find_command("ACCEPTS_ONLY_LVAR_INT"),
                        builder.find_command("ACCEPTS_ONLY_LVAR_FLOAT")});

        return builder;
    }

    static void
    add_command(CommandManager::Builder& builder, std::string_view name,

                std::initializer_list<CommandManager::ParamDef> params)
    {
        auto [command, _] = builder.insert_command(name);
        builder.set_command_params(*command, params.begin(), params.end(),
                                   params.size());
    }

    static void
    add_alternator(CommandManager::Builder& builder, std::string_view name,
                   std::initializer_list<const CommandManager::CommandDef*>
                           alternatives)
    {
        auto [alternator, _] = builder.insert_alternator(name);
        for(const auto& alternative : alternatives)
            builder.insert_alternative(*alternator, *alternative);
    }

private:
    ArenaMemoryResource arena;

protected:
    CommandManager cmdman;
};
} // namespace gta3sc::test

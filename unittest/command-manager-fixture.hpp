#pragma once
#include <doctest.h>
#include <gta3sc/command-manager.hpp>

namespace gta3sc::test
{
class CommandManagerFixture
{
public:
    CommandManagerFixture()
    {
        using ParamDef = gta3sc::CommandManager::ParamDef;
        using ParamType = gta3sc::CommandManager::ParamType;
        using EntityId = gta3sc::CommandManager::EntityId;
        using EnumId = gta3sc::CommandManager::EnumId;

        CHECK(cmdman.find_enumeration("GLOBAL"));
        cmdman.add_enumeration("PEDTYPE");
        cmdman.add_enumeration("DEFAULTMODEL");
        cmdman.add_enumeration("FADE");

        cmdman.add_constant(*cmdman.find_enumeration("GLOBAL"), "FALSE", 0);
        cmdman.add_constant(*cmdman.find_enumeration("GLOBAL"), "TRUE", 1);
        cmdman.add_constant(*cmdman.find_enumeration("GLOBAL"), "OFF", 0);
        cmdman.add_constant(*cmdman.find_enumeration("GLOBAL"), "ON", 1);

        cmdman.add_constant(*cmdman.find_enumeration("PEDTYPE"), "PEDTYPE_CIVMALE", 4);
        cmdman.add_constant(*cmdman.find_enumeration("PEDTYPE"), "PEDTYPE_CIVFEMALE", 5);
        cmdman.add_constant(*cmdman.find_enumeration("PEDTYPE"), "PEDTYPE_MEDIC", 16);

        cmdman.add_constant(*cmdman.find_enumeration("FADE"), "FADE_OUT", 0);
        cmdman.add_constant(*cmdman.find_enumeration("FADE"), "FADE_IN", 1);

        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "MEDIC", 5);
        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "HFYST", 9);
        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "HMYST", 11);
        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "CHEETAH", 145);
        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "WASHING", 151);
        cmdman.add_constant(*cmdman.find_enumeration("DEFAULTMODEL"), "LOVEFIST", 201);

        cmdman.add_command("{", {});
        cmdman.add_command("}", {});
        cmdman.add_command("IF", {ParamDef{ParamType::INT}});
        cmdman.add_command("ENDIF", {});
        cmdman.add_command("WAIT", {ParamDef{ParamType::INPUT_INT}});
        cmdman.add_command("GOTO", {ParamDef{ParamType::LABEL}});
        cmdman.add_command("GOSUB", {ParamDef{ParamType::LABEL}});
        cmdman.add_command("RETURN", {});
        cmdman.add_command("SCRIPT_NAME", {ParamDef{ParamType::TEXT_LABEL}});
        cmdman.add_command("PRINT_HELP", {ParamDef{ParamType::TEXT_LABEL}});
        cmdman.add_command("START_NEW_SCRIPT", {ParamDef{ParamType::LABEL},
                                                ParamDef{ParamType::INPUT_OPT}});
        cmdman.add_command("VAR_INT", {ParamDef{ParamType::VAR_INT},
                                       ParamDef{ParamType::VAR_INT_OPT}});
        cmdman.add_command("LVAR_INT", {ParamDef{ParamType::LVAR_INT},
                                        ParamDef{ParamType::LVAR_INT_OPT}});
        cmdman.add_command("VAR_FLOAT", {ParamDef{ParamType::VAR_FLOAT},
                                         ParamDef{ParamType::VAR_FLOAT_OPT}});
        cmdman.add_command("LVAR_FLOAT", {ParamDef{ParamType::LVAR_FLOAT},
                                          ParamDef{ParamType::LVAR_FLOAT_OPT}});
        cmdman.add_command("VAR_TEXT_LABEL", {ParamDef{ParamType::VAR_TEXT_LABEL},
                                              ParamDef{ParamType::VAR_TEXT_LABEL_OPT}});
        cmdman.add_command("LVAR_TEXT_LABEL", {ParamDef{ParamType::LVAR_TEXT_LABEL},
                                               ParamDef{ParamType::LVAR_TEXT_LABEL_OPT}});
        cmdman.add_command("GENERATE_RANDOM_FLOAT_IN_RANGE",
            {ParamDef{ParamType::INPUT_FLOAT},
             ParamDef{ParamType::INPUT_FLOAT},
             ParamDef{ParamType::OUTPUT_FLOAT}});
        cmdman.add_command("GENERATE_RANDOM_INT_IN_RANGE",
            {ParamDef{ParamType::INPUT_INT},
             ParamDef{ParamType::INPUT_INT},
             ParamDef{ParamType::OUTPUT_INT}});
        cmdman.add_command("SAVE_STRING_TO_DEBUG_FILE", {ParamDef{ParamType::STRING}});
        cmdman.add_command("SAVE_STRING_TO_DEBUG_FILE", {ParamDef{ParamType::STRING}});

        cmdman.add_command("CREATE_CHAR", {
                ParamDef{ParamType::INPUT_INT, EntityId{}, *cmdman.find_enumeration("PEDTYPE")}, 
                ParamDef{ParamType::INPUT_INT, EntityId{}, *cmdman.find_enumeration("DEFAULTMODEL")},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::OUTPUT_INT}});

        cmdman.add_command("CREATE_CAR", {
                ParamDef{ParamType::INPUT_INT, EntityId{}, *cmdman.find_enumeration("DEFAULTMODEL")},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::INPUT_FLOAT},
                ParamDef{ParamType::OUTPUT_INT}});

        cmdman.add_command("DO_FADE", {
                ParamDef{ParamType::INPUT_INT},
                ParamDef{ParamType::INPUT_INT, EntityId{}, *cmdman.find_enumeration("FADE")}});

        cmdman.add_command("SET_VAR_INT", {ParamDef{ParamType::VAR_INT},
                                           ParamDef{ParamType::INT}});
        cmdman.add_command("SET_VAR_FLOAT", {ParamDef{ParamType::VAR_FLOAT},
                                             ParamDef{ParamType::FLOAT}});
        cmdman.add_command("SET_LVAR_INT", {ParamDef{ParamType::LVAR_INT},
                                            ParamDef{ParamType::INT}});
        cmdman.add_command("SET_LVAR_FLOAT", {ParamDef{ParamType::LVAR_FLOAT},
                                              ParamDef{ParamType::FLOAT}});
        cmdman.add_command(
                "SET_VAR_INT_TO_VAR_INT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::VAR_INT}});
        cmdman.add_command(
                "SET_LVAR_INT_TO_LVAR_INT",
                {ParamDef{ParamType::LVAR_INT}, ParamDef{ParamType::LVAR_INT}});
        cmdman.add_command("SET_VAR_FLOAT_TO_VAR_FLOAT",
                           {ParamDef{ParamType::VAR_FLOAT},
                            ParamDef{ParamType::VAR_FLOAT}});
        cmdman.add_command("SET_LVAR_FLOAT_TO_LVAR_FLOAT",
                           {ParamDef{ParamType::LVAR_FLOAT},
                            ParamDef{ParamType::LVAR_FLOAT}});
        cmdman.add_command("SET_VAR_FLOAT_TO_LVAR_FLOAT",
                           {ParamDef{ParamType::VAR_FLOAT},
                            ParamDef{ParamType::LVAR_FLOAT}});
        cmdman.add_command("SET_LVAR_FLOAT_TO_VAR_FLOAT",
                           {ParamDef{ParamType::LVAR_FLOAT},
                            ParamDef{ParamType::VAR_FLOAT}});
        cmdman.add_command(
                "SET_VAR_INT_TO_LVAR_INT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::LVAR_INT}});
        cmdman.add_command(
                "SET_LVAR_INT_TO_VAR_INT",
                {ParamDef{ParamType::LVAR_INT}, ParamDef{ParamType::VAR_INT}});
        cmdman.add_command(
                "SET_VAR_INT_TO_CONSTANT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::INPUT_INT}});
        cmdman.add_command(
                "SET_LVAR_INT_TO_CONSTANT",
                {ParamDef{ParamType::VAR_INT}, ParamDef{ParamType::INPUT_INT}});
        cmdman.add_command("SET_VAR_TEXT_LABEL",
                           {ParamDef{ParamType::VAR_TEXT_LABEL},
                            ParamDef{ParamType::TEXT_LABEL}});
        cmdman.add_command("SET_LVAR_TEXT_LABEL",
                           {ParamDef{ParamType::LVAR_TEXT_LABEL},
                            ParamDef{ParamType::TEXT_LABEL}});

        cmdman.add_command("ABS_VAR_INT", {ParamDef{ParamType::VAR_INT}});
        cmdman.add_command("ABS_LVAR_INT", {ParamDef{ParamType::LVAR_INT}});
        cmdman.add_command("ABS_VAR_FLOAT", {ParamDef{ParamType::VAR_FLOAT}});
        cmdman.add_command("ABS_LVAR_FLOAT", {ParamDef{ParamType::LVAR_FLOAT}});

        cmdman.add_command("ACCEPTS_ONLY_VAR_INT", {ParamDef{ParamType::VAR_INT}});
        cmdman.add_command("ACCEPTS_ONLY_LVAR_INT", {ParamDef{ParamType::LVAR_INT}});
        cmdman.add_command("ACCEPTS_ONLY_VAR_FLOAT", {ParamDef{ParamType::VAR_FLOAT}});
        cmdman.add_command("ACCEPTS_ONLY_LVAR_FLOAT", {ParamDef{ParamType::LVAR_FLOAT}});

        cmdman.add_alternator("SET", 
                              {cmdman.find_command("SET_VAR_INT"),
                               cmdman.find_command("SET_VAR_FLOAT"),
                               cmdman.find_command("SET_LVAR_INT"),
                               cmdman.find_command("SET_LVAR_FLOAT"),
                               cmdman.find_command("SET_VAR_INT_TO_VAR_INT"),
                               cmdman.find_command("SET_LVAR_INT_TO_LVAR_INT"),
                               cmdman.find_command("SET_VAR_FLOAT_TO_VAR_FLOAT"),
                               cmdman.find_command("SET_LVAR_FLOAT_TO_LVAR_FLOAT"),
                               cmdman.find_command("SET_VAR_FLOAT_TO_LVAR_FLOAT"),
                               cmdman.find_command("SET_LVAR_FLOAT_TO_VAR_FLOAT"),
                               cmdman.find_command("SET_VAR_INT_TO_LVAR_INT"),
                               cmdman.find_command("SET_LVAR_INT_TO_VAR_INT"),
                               cmdman.find_command("SET_VAR_INT_TO_CONSTANT"),
                               cmdman.find_command("SET_LVAR_INT_TO_CONSTANT"),
                               cmdman.find_command("SET_VAR_TEXT_LABEL"),
                               cmdman.find_command("SET_LVAR_TEXT_LABEL")});

        cmdman.add_alternator("ABS",
                              {cmdman.find_command("ABS_VAR_INT"),
                               cmdman.find_command("ABS_LVAR_INT"),
                               cmdman.find_command("ABS_VAR_FLOAT"),
                               cmdman.find_command("ABS_LVAR_FLOAT")});

        cmdman.add_alternator("ACCEPTS_ONLY_GLOBAL_VAR",
                              {cmdman.find_command("ACCEPTS_ONLY_VAR_INT"),
                               cmdman.find_command("ACCEPTS_ONLY_VAR_FLOAT")});

        cmdman.add_alternator("ACCEPTS_ONLY_LOCAL_VAR",
                              {cmdman.find_command("ACCEPTS_ONLY_LVAR_INT"),
                               cmdman.find_command("ACCEPTS_ONLY_LVAR_FLOAT")});
    }

protected:
    CommandManager cmdman;
};
}

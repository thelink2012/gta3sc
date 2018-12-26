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

    }

protected:
    CommandManager cmdman;
};
}

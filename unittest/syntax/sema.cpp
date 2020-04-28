#include "../command-manager-fixture.hpp"
#include "syntax-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/sema.hpp>
using namespace gta3sc::test;
using namespace gta3sc::test::syntax;
using namespace std::literals::string_view_literals;
using gta3sc::test::syntax::d;

namespace gta3sc::test::syntax
{
class SemaFixture
    : public SyntaxFixture
    , public CommandManagerFixture
{
public:
    SemaFixture() : sema(default_sema()), symrepo(arena)
    {
        modelman = ModelManager::Builder(arena)
                           .insert_model("CHEETAH")
                           .insert_model("LEVEL_MODEL")
                           .insert_model("OTHER_LEVEL_MODEL")
                           .build();
    }

    void build_sema(std::string_view src, bool parser_error = false)
    {
        auto ir = make_parser(make_source(src), arena).parse_main_script_file();
        if(!parser_error)
        {
            REQUIRE(ir != std::nullopt);
            this->sema = gta3sc::syntax::Sema(std::move(*ir), symrepo, cmdman,
                                              modelman, diagman, arena);
        }
        else
        {
            REQUIRE(ir == std::nullopt);
            this->sema = default_sema();
        }
    }

private:
    auto make_parser(gta3sc::SourceFile source,
                     gta3sc::ArenaMemoryResource& arena)
            -> gta3sc::syntax::Parser
    {
        auto pp = gta3sc::syntax::Preprocessor(std::move(source), diagman);
        auto scanner = gta3sc::syntax::Scanner(std::move(pp));
        return gta3sc::syntax::Parser(std::move(scanner), arena);
    }

    auto default_sema() -> gta3sc::syntax::Sema
    {
        return gta3sc::syntax::Sema(gta3sc::LinkedIR<gta3sc::ParserIR>(),
                                    symrepo, cmdman, modelman, diagman, arena);
    }

    gta3sc::ArenaMemoryResource arena;

protected:
    gta3sc::SymbolRepository symrepo;
    gta3sc::ModelManager modelman;
    gta3sc::syntax::Sema sema;
};
}

namespace
{
auto size(const gta3sc::LinkedIR<gta3sc::SemaIR>& ir)
{
    return std::distance(ir.begin(), ir.end());
}
}

TEST_CASE_FIXTURE(SemaFixture, "sema valid variables declarations")
{
    build_sema("VAR_INT gvar1 gvar2 gvar3\n"
               "VAR_FLOAT gvar4\n"
               "VAR_TEXT_LABEL gvar5\n"
               "{\n"
               "LVAR_INT lvar11 lvar12 lvar13\n"
               "LVAR_FLOAT lvar14 lvar15 lvar16 lvar17\n"
               "LVAR_TEXT_LABEL lvar18\n"
               "}\n"
               "VAR_INT gvar6\n"
               "{\n"
               "VAR_FLOAT gvar7 gvar8[25]\n"
               "LVAR_FLOAT lvar21[30] lvar22 lvar23 lvar24 lvar25\n"
               "}\n");

    REQUIRE(sema.validate() != std::nullopt);

    const gta3sc::SymVariable* gvar[10] = {};
    const gta3sc::SymVariable* lvar1[10] = {};
    const gta3sc::SymVariable* lvar2[10] = {};

    gvar[1] = symrepo.lookup_var("GVAR1");
    gvar[2] = symrepo.lookup_var("GVAR2");
    gvar[3] = symrepo.lookup_var("GVAR3");
    gvar[4] = symrepo.lookup_var("GVAR4");
    gvar[5] = symrepo.lookup_var("GVAR5");
    gvar[6] = symrepo.lookup_var("GVAR6");
    gvar[7] = symrepo.lookup_var("GVAR7");
    gvar[8] = symrepo.lookup_var("GVAR8");
    gvar[9] = symrepo.lookup_var("GVAR9");

    lvar1[1] = symrepo.lookup_var("LVAR11", 1);
    lvar1[2] = symrepo.lookup_var("LVAR12", 1);
    lvar1[3] = symrepo.lookup_var("LVAR13", 1);
    lvar1[4] = symrepo.lookup_var("LVAR14", 1);
    lvar1[5] = symrepo.lookup_var("LVAR15", 1);
    lvar1[6] = symrepo.lookup_var("LVAR16", 1);
    lvar1[7] = symrepo.lookup_var("LVAR17", 1);
    lvar1[8] = symrepo.lookup_var("LVAR18", 1);
    lvar1[9] = symrepo.lookup_var("LVAR19", 1);

    lvar2[1] = symrepo.lookup_var("LVAR21", 2);
    lvar2[2] = symrepo.lookup_var("LVAR22", 2);
    lvar2[3] = symrepo.lookup_var("LVAR23", 2);
    lvar2[4] = symrepo.lookup_var("LVAR24", 2);
    lvar2[5] = symrepo.lookup_var("LVAR25", 2);
    lvar2[6] = symrepo.lookup_var("LVAR26", 2);

    REQUIRE(symrepo.lookup_var("gvar1") == nullptr);
    REQUIRE(symrepo.lookup_var("GVAR1", 1) == nullptr);
    REQUIRE(symrepo.lookup_var("LVAR11") == nullptr);
    REQUIRE(symrepo.lookup_var("LVAR11", 2) == nullptr);

    REQUIRE(gvar[1] != nullptr);
    REQUIRE(gvar[9] == nullptr);
    for(int i = 1; i <= 8; ++i)
        REQUIRE(gvar[i] != nullptr);
    for(int i = 1; i <= 8; ++i)
        REQUIRE(gvar[i]->id == i - 1);
    for(int i = 1; i <= 7; ++i)
        REQUIRE(gvar[i]->dim == std::nullopt);
    REQUIRE(gvar[8]->dim == 25);
    REQUIRE(gvar[1]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(gvar[2]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(gvar[3]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(gvar[4]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(gvar[5]->type == gta3sc::SymVariable::Type::TEXT_LABEL);
    REQUIRE(gvar[6]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(gvar[7]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(gvar[8]->type == gta3sc::SymVariable::Type::FLOAT);

    REQUIRE(lvar1[9] == nullptr);
    for(int i = 1; i <= 8; ++i)
        REQUIRE(lvar1[i] != nullptr);
    for(int i = 1; i <= 8; ++i)
        REQUIRE(lvar1[i]->id == i - 1);
    for(int i = 1; i <= 8; ++i)
        REQUIRE(lvar1[i]->dim == std::nullopt);
    REQUIRE(lvar1[1]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(lvar1[2]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(lvar1[3]->type == gta3sc::SymVariable::Type::INT);
    REQUIRE(lvar1[4]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(lvar1[5]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(lvar1[6]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(lvar1[7]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(lvar1[8]->type == gta3sc::SymVariable::Type::TEXT_LABEL);

    REQUIRE(lvar2[6] == nullptr);
    for(int i = 1; i <= 5; ++i)
        REQUIRE(lvar2[i] != nullptr);
    for(int i = 1; i <= 5; ++i)
        REQUIRE(lvar2[i]->id == i - 1);
    for(int i = 1; i <= 5; ++i)
        REQUIRE(lvar2[i]->type == gta3sc::SymVariable::Type::FLOAT);
    REQUIRE(lvar2[1]->dim == 30);
    for(int i = 2; i <= 5; ++i)
        REQUIRE(lvar2[i]->dim == std::nullopt);
}

TEST_CASE_FIXTURE(SemaFixture, "sema invalid variables declarations")
{
    SUBCASE("not integer literal subscript")
    {
        build_sema("VAR_INT x[y] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::var_decl_subscript_must_be_literal);
    }

    SUBCASE("zero integer literal subscript")
    {
        build_sema("VAR_INT x[0] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::var_decl_subscript_must_be_nonzero);
    }

    SUBCASE("local variable declaration outside of scope")
    {
        build_sema("LVAR_INT x y z");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::var_decl_outside_of_scope);
        CHECK(consume_diag().message
              == gta3sc::Diag::var_decl_outside_of_scope);
        CHECK(consume_diag().message
              == gta3sc::Diag::var_decl_outside_of_scope);
    }

    SUBCASE("variable decl name is not identifier")
    {
        build_sema("VAR_INT x 9 z");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_identifier);
    }

    SUBCASE("global variable duplicate")
    {
        build_sema("VAR_INT x x y\nVAR_INT y\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_global);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_global);
    }

    SUBCASE("local variable duplicate")
    {
        build_sema("{\nLVAR_INT x x y\nLVAR_INT y\n}\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_in_scope);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_in_scope);
    }

    SUBCASE("local variable in different scopes is not a duplicate")
    {
        build_sema("{\nLVAR_INT x y\n}\n{\nLVAR_INT y\n}\n");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("local variable with same name as global variable")
    {
        build_sema("VAR_INT x\n{\nLVAR_INT x\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_lvar);
    }

    SUBCASE("global variable with same name as local variable")
    {
        build_sema("{\nLVAR_INT x\n}\nVAR_INT x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_lvar);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema label declaration")
{
    SUBCASE("no label duplicate")
    {
        build_sema("label1:\nlabel2:\n");
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(symrepo.lookup_label("label1") == nullptr);
        REQUIRE(symrepo.lookup_label("LABEL1") != nullptr);
        REQUIRE(symrepo.lookup_label("LABEL2") != nullptr);
    }

    SUBCASE("label duplicate")
    {
        build_sema("label1:\nlabel1:\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_label);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema different symbol tables do not collide")
{
    build_sema("x:\n"
               "y:\n"
               "VAR_INT x\n"
               "{\nLVAR_INT y\n}\n");
    REQUIRE(sema.validate() != std::nullopt);
}

TEST_CASE_FIXTURE(SemaFixture,
                  "sema variable names collides with string constants")
{
    build_sema("VAR_INT ON PEDTYPE_CIVMALE\n"
               "{\nLVAR_INT FALSE PEDTYPE_CIVFEMALE\n}");
    REQUIRE(sema.validate() == std::nullopt);
    CHECK(consume_diag().message
          == gta3sc::Diag::duplicate_var_string_constant);
    CHECK(consume_diag().message
          == gta3sc::Diag::duplicate_var_string_constant);
    CHECK(diags.empty()); // does not collide with ON/FALSE
}

TEST_CASE_FIXTURE(SemaFixture, "sema using local variable from another scope")
{
    build_sema("{\nLVAR_INT x\n}\n{\nSET_VAR_INT x 1\n}\nSET_VAR_INT x 1");
    REQUIRE(sema.validate() == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
}

TEST_CASE_FIXTURE(SemaFixture, "sema parsing variable reference")
{
    SUBCASE("empty subscript")
    {
        build_sema("VAR_INT x[] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_subscript);
    }

    SUBCASE("negative subscript")
    {
        build_sema("VAR_INT x[-5] z[10]", true); // parser error
        CHECK(consume_diag().message == gta3sc::Diag::expected_token);
    }

    SUBCASE("too big literal subscript")
    {
        build_sema("VAR_INT x[2147483647] y[2147483648] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::integer_literal_too_big);
    }

    SUBCASE("floating point subscript")
    {
        build_sema("VAR_INT x[2.0] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
    }

    SUBCASE("swap brackets")
    {
        build_sema("VAR_INT x]10[ z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == d("["));
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == d("]"));
    }

    SUBCASE("missing opening bracket")
    {
        build_sema("VAR_INT x10] z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == d("["));
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == d("]"));
        CHECK(consume_diag().message == gta3sc::Diag::expected_subscript);
    }

    SUBCASE("missing closing bracket")
    {
        build_sema("VAR_INT x[10 z[10]");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == d("]"));
    }

    SUBCASE("empty var name")
    {
        build_sema("VAR_INT [10] z[10]", true); // parser error
        CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema undefined command")
{
    build_sema("UNDEFINED_COMMAND 1 x 3.0 4");
    REQUIRE(sema.validate() == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::undefined_command);
}

TEST_CASE_FIXTURE(SemaFixture, "sema valid command")
{
    build_sema("WAIT 98\nsome_label: print_HELP hello\n");
    auto ir = sema.validate();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 2);

    REQUIRE(ir->front().label == nullptr);
    REQUIRE(ir->front().command != nullptr);
    REQUIRE(ir->front().command->not_flag == false);
    REQUIRE(&ir->front().command->def == cmdman.find_command("WAIT"));
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE(*ir->front().command->args[0]->as_integer() == 98);

    REQUIRE(ir->back().label == symrepo.lookup_label("SOME_LABEL"));
    REQUIRE(ir->back().command != nullptr);
    REQUIRE(ir->front().command->not_flag == false);
    REQUIRE(&ir->back().command->def == cmdman.find_command("PRINT_HELP"));
    REQUIRE(ir->back().command->args.size() == 1);
    REQUIRE(*ir->back().command->args[0]->as_text_label() == "HELLO"sv);
}

TEST_CASE_FIXTURE(SemaFixture, "sema valid NOTed command")
{
    build_sema("IF NOT WAIT 0\nENDIF");
    auto ir = sema.validate();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 3);

    const auto& wait = *std::next(ir->begin());
    REQUIRE(wait.label == nullptr);
    REQUIRE(wait.command != nullptr);
    REQUIRE(wait.command->not_flag == true);
    REQUIRE(&wait.command->def == cmdman.find_command("WAIT"));
}

TEST_CASE_FIXTURE(SemaFixture, "sema INT parameter")
{
    SUBCASE("valid INT param")
    {
        build_sema("VAR_INT x\nSET_VAR_INT x 100\n");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);

        const auto& command_1 = *std::next(ir->begin(), 1)->command;
        REQUIRE(command_1.args.size() == 2);
        REQUIRE(*command_1.args[1]->as_integer() == 100);
    }

    SUBCASE("invalid INT param - not integer")
    {
        build_sema("VAR_INT x\nSET_VAR_INT x x\nSET_VAR_INT x y");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
    }

    SUBCASE("invalid INT param - string constant")
    {
        build_sema("VAR_INT x\nSET_VAR_INT x ON\nSET_VAR_INT x CHEETAH");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema FLOAT parameter")
{
    SUBCASE("valid FLOAT param")
    {
        build_sema("VAR_FLOAT x\nSET_VAR_FLOAT x 1.0");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);
        REQUIRE(ir->back().command->args.size() == 2);
        REQUIRE(*ir->back().command->args[1]->as_float() == 1.0);
    }

    SUBCASE("invalid FLOAT param - not float")
    {
        build_sema("VAR_FLOAT x\nSET_VAR_FLOAT x x\nSET_VAR_FLOAT x y");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_float);
        CHECK(consume_diag().message == gta3sc::Diag::expected_float);
    }

    SUBCASE("invalid FLOAT param - string constant")
    {
        build_sema("VAR_FLOAT x\nSET_VAR_FLOAT x ON\nSET_VAR_FLOAT x CHEETAH");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_float);
        CHECK(consume_diag().message == gta3sc::Diag::expected_float);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema TEXT_LABEL parameter")
{
    SUBCASE("valid TEXT_LABEL param")
    {
        build_sema("VAR_TEXT_LABEL soMehelP $x\n"
                   "PRINT_HELP somehelp\n"
                   "PRINT_HELP $someHElp\n"
                   "PRINT_HELP $$x\n");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 4);

        const auto& second = *std::prev(ir->end(), 3);
        const auto& third = *std::prev(ir->end(), 2);
        const auto& fourth = *std::prev(ir->end(), 1);

        REQUIRE(*second.command->args[0]->as_text_label() == "SOMEHELP"sv);
        REQUIRE(second.command->args[0]->as_var_ref() == nullptr);

        REQUIRE(std::addressof(third.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("SOMEHELP"));
        REQUIRE(third.command->args[0]->as_text_label() == nullptr);

        REQUIRE(std::addressof(fourth.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$X"));
        REQUIRE(fourth.command->args[0]->as_text_label() == nullptr);
    }

    SUBCASE("invalid TEXT_LABEL param - not identifier")
    {
        build_sema("PRINT_HELP 1234");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_text_label);
    }

    SUBCASE("invalid TEXT_LABEL param - not variable")
    {
        build_sema("PRINT_HELP $\n"
                   "PRINT_HELP $[\n"
                   "PRINT_HELP $]\n"
                   "PRINT_HELP $a\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::expected_varname_after_dollar);
        CHECK(consume_diag().message
              == gta3sc::Diag::expected_varname_after_dollar);
        CHECK(consume_diag().message
              == gta3sc::Diag::expected_varname_after_dollar);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid TEXT_LABEL param - global string constant")
    {
        build_sema("PRINT_HELP ON");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::cannot_use_string_constant_here);
    }

    SUBCASE("valid TEXT_LABEL param - non-global string constant")
    {
        build_sema("PRINT_HELP MEDIC");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(*ir->front().command->args[0]->as_text_label() == "MEDIC"sv);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema LABEL parameter")
{
    SUBCASE("valid LABEL param")
    {
        build_sema("laBel1: GOTO lAbel1");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().label == symrepo.lookup_label("LABEL1"));
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args[0]->as_label() == ir->front().label);
    }

    SUBCASE("valid LABEL param - does not collide with string constants")
    {
        build_sema("ON: GOTO ON");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().label == symrepo.lookup_label("ON"));
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args[0]->as_label() == ir->front().label);
    }

    SUBCASE("invalid LABEL param - does not exist")
    {
        build_sema("laBel1: GOTO lAbel2");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_label);
    }

    SUBCASE("invalid LABEL param - not identifier")
    {
        build_sema("laBel1: GOTO 1234");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_label);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema STRING parameter")
{
    SUBCASE("valid STRING param")
    {
        build_sema("SAVE_STRING_TO_DEBUG_FILE \"Hello World\"");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 1);
        REQUIRE(*ir->front().command->args[0]->as_string() == "Hello World"sv);
    }

    SUBCASE("invalid STRING param - not string")
    {
        build_sema("SAVE_STRING_TO_DEBUG_FILE hello\n"
                   "SAVE_STRING_TO_DEBUG_FILE 1234\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_string);
        CHECK(consume_diag().message == gta3sc::Diag::expected_string);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema VAR_INT parameter")
{
    SUBCASE("valid VAR_INT param")
    {
        build_sema("{\nVAR_INT g1 $g2\nSET_VAR_INT $g2 1\nSET_VAR_INT g1 1\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_var_int_1 = *std::prev(ir->end(), 3);
        const auto& set_var_int_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_var_int_1.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_var_int_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$G2"));
        REQUIRE(set_var_int_2.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_var_int_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("G1"));
    }

    SUBCASE("invalid VAR_INT param - not identifier")
    {
        build_sema("SET_VAR_INT 1 1");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid VAR_INT param - undeclared variable")
    {
        build_sema("SET_VAR_INT x 1");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid VAR_INT param - lvar instead of gvar")
    {
        build_sema("{\nLVAR_INT lvar\nSET_VAR_INT lvar 1\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_gvar_got_lvar);
    }

    SUBCASE("invalid VAR_INT param - type mismatch")
    {
        build_sema("{\nVAR_FLOAT g1\nSET_VAR_INT g1 1\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema LVAR_INT parameter")
{
    SUBCASE("valid LVAR_INT param")
    {
        build_sema(
                "{\nLVAR_INT l1 $l2\nSET_LVAR_INT $l2 1\nSET_LVAR_INT l1 1\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_lvar_int_1 = *std::prev(ir->end(), 3);
        const auto& set_lvar_int_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_lvar_int_1.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_lvar_int_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$L2", 1));
        REQUIRE(set_lvar_int_2.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_lvar_int_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("L1", 1));
    }

    SUBCASE("invalid LVAR_INT param - not identifier")
    {
        build_sema("SET_LVAR_INT 1 1");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid LVAR_INT param - undeclared variable")
    {
        build_sema("SET_LVAR_INT x 1");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid LVAR_INT param - gvar instead of lvar")
    {
        build_sema("{\nVAR_INT gvar\nSET_LVAR_INT gvar 1\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_lvar_got_gvar);
    }

    SUBCASE("invalid LVAR_INT param - type mismatch")
    {
        build_sema("{\nLVAR_FLOAT l1\nSET_LVAR_INT l1 1\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema VAR_FLOAT parameter")
{
    SUBCASE("valid VAR_FLOAT param")
    {
        build_sema("{\nVAR_FLOAT g1 $g2\nSET_VAR_FLOAT $g2 1.0\nSET_VAR_FLOAT "
                   "g1 1.0\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_var_flt_1 = *std::prev(ir->end(), 3);
        const auto& set_var_flt_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_var_flt_1.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_var_flt_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$G2"));
        REQUIRE(set_var_flt_2.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_var_flt_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("G1"));
    }

    SUBCASE("invalid VAR_FLOAT param - not identifier")
    {
        build_sema("SET_VAR_FLOAT 1.0 1.0");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid VAR_FLOAT param - undeclared variable")
    {
        build_sema("SET_VAR_FLOAT x 1.0");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid VAR_FLOAT param - lvar instead of gvar")
    {
        build_sema("{\nLVAR_FLOAT lvar\nSET_VAR_FLOAT lvar 1.0\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_gvar_got_lvar);
    }

    SUBCASE("invalid VAR_FLOAT param - type mismatch")
    {
        build_sema("{\nVAR_INT g1\nSET_VAR_FLOAT g1 1.0\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema LVAR_FLOAT parameter")
{
    SUBCASE("valid LVAR_FLOAT param")
    {
        build_sema("{\nLVAR_FLOAT l1 $l2\nSET_LVAR_FLOAT $l2 "
                   "1.0\nSET_LVAR_FLOAT l1 1.0\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_lvar_flt_1 = *std::prev(ir->end(), 3);
        const auto& set_lvar_flt_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_lvar_flt_1.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_lvar_flt_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$L2", 1));
        REQUIRE(set_lvar_flt_2.command->args.size() == 2);
        REQUIRE(std::addressof(
                        set_lvar_flt_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("L1", 1));
    }

    SUBCASE("invalid LVAR_FLOAT param - not identifier")
    {
        build_sema("SET_LVAR_FLOAT 1.0 1.0");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid LVAR_FLOAT param - undeclared variable")
    {
        build_sema("SET_LVAR_FLOAT x 1.0");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid LVAR_FLOAT param - gvar instead of lvar")
    {
        build_sema("{\nVAR_FLOAT gvar\nSET_LVAR_FLOAT gvar 1.0\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_lvar_got_gvar);
    }

    SUBCASE("invalid LVAR_FLOAT param - type mismatch")
    {
        build_sema("{\nLVAR_INT l1\nSET_LVAR_FLOAT l1 1.0\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema VAR_TEXT_LABEL parameter")
{
    SUBCASE("valid VAR_TEXT_LABEL param")
    {
        build_sema("{\nVAR_TEXT_LABEL g1 $g2\nSET_VAR_TEXT_LABEL $g2 TEXT"
                   "\nSET_VAR_TEXT_LABEL g1 TEXT\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_var_1 = *std::prev(ir->end(), 3);
        const auto& set_var_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_var_1.command->args.size() == 2);
        REQUIRE(std::addressof(set_var_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$G2"));
        REQUIRE(set_var_2.command->args.size() == 2);
        REQUIRE(std::addressof(set_var_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("G1"));
    }

    SUBCASE("invalid VAR_TEXT_LABEL param - not identifier")
    {
        build_sema("SET_VAR_TEXT_LABEL 1234 TEXT");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid VAR_TEXT_LABEL param - undeclared variable")
    {
        build_sema("SET_VAR_TEXT_LABEL x TEXT");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid VAR_TEXT_LABEL param - lvar instead of gvar")
    {
        build_sema("{\nLVAR_TEXT_LABEL lvar\nSET_VAR_TEXT_LABEL lvar TEXT\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_gvar_got_lvar);
    }

    SUBCASE("invalid VAR_TEXT_LABEL param - type mismatch")
    {
        build_sema("{\nVAR_INT g1\nSET_VAR_TEXT_LABEL g1 TEXT\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema LVAR_TEXT_LABEL parameter")
{
    SUBCASE("valid LVAR_TEXT_LABEL param")
    {
        build_sema("{\nLVAR_TEXT_LABEL l1 $l2\nSET_LVAR_TEXT_LABEL $l2 TEXT"
                   "\nSET_LVAR_TEXT_LABEL l1 TEXT\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);
        const auto& set_lvar_1 = *std::prev(ir->end(), 3);
        const auto& set_lvar_2 = *std::prev(ir->end(), 2);
        REQUIRE(set_lvar_1.command->args.size() == 2);
        REQUIRE(std::addressof(set_lvar_1.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("$L2", 1));
        REQUIRE(set_lvar_2.command->args.size() == 2);
        REQUIRE(std::addressof(set_lvar_2.command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("L1", 1));
    }

    SUBCASE("invalid LVAR_TEXT_LABEL param - not identifier")
    {
        build_sema("SET_LVAR_TEXT_LABEL 1234 TEXT");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid LVAR_TEXT_LABEL param - undeclared variable")
    {
        build_sema("SET_LVAR_TEXT_LABEL x TEXT");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid LVAR_TEXT_LABEL param - gvar instead of lvar")
    {
        build_sema("{\nVAR_TEXT_LABEL gvar\nSET_LVAR_TEXT_LABEL gvar TEXT\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_lvar_got_gvar);
    }

    SUBCASE("invalid LVAR_TEXT_LABEL param - type mismatch")
    {
        build_sema("{\nLVAR_FLOAT l1\nSET_LVAR_TEXT_LABEL l1 TEXT\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema OUTPUT_INT parameter")
{
    SUBCASE("valid OUTPUT_INT parameter")
    {
        build_sema("{\nVAR_INT x\nLVAR_INT y\n"
                   "GENERATE_RANDOM_INT_IN_RANGE 0 10 x\n"
                   "GENERATE_RANDOM_INT_IN_RANGE 0 10 y\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);

        REQUIRE(size(*ir) == 6);

        auto arg1 = std::prev(ir->end(), 3)->command->args[2];
        auto arg2 = std::prev(ir->end(), 2)->command->args[2];

        REQUIRE(std::addressof(arg1->as_var_ref()->def)
                == symrepo.lookup_var("X", 0));
        REQUIRE(std::addressof(arg2->as_var_ref()->def)
                == symrepo.lookup_var("Y", 1));
    }

    SUBCASE("invalid OUTPUT_INT param - not identifier")
    {
        build_sema("GENERATE_RANDOM_INT_IN_RANGE 0 10 1234");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid OUTPUT_INT param - undeclared variable")
    {
        build_sema("GENERATE_RANDOM_INT_IN_RANGE 0 10 x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid OUTPUT_INT param - type mismatch")
    {
        build_sema("VAR_FLOAT x\nGENERATE_RANDOM_INT_IN_RANGE 0 10 x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("invalid OUTPUT_INT param - global string constant")
    {
        build_sema("VAR_INT ON\nGENERATE_RANDOM_INT_IN_RANGE 0 10 ON");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::cannot_use_string_constant_here);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema OUTPUT_FLOAT parameter")
{
    SUBCASE("valid OUTPUT_FLOAT parameter")
    {
        build_sema("{\nVAR_FLOAT x\nLVAR_FLOAT y\n"
                   "GENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 x\n"
                   "GENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 y\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);

        REQUIRE(size(*ir) == 6);

        auto arg1 = std::prev(ir->end(), 3)->command->args[2];
        auto arg2 = std::prev(ir->end(), 2)->command->args[2];

        REQUIRE(std::addressof(arg1->as_var_ref()->def)
                == symrepo.lookup_var("X", 0));
        REQUIRE(std::addressof(arg2->as_var_ref()->def)
                == symrepo.lookup_var("Y", 1));
    }

    SUBCASE("invalid OUTPUT_FLOAT param - not identifier")
    {
        build_sema("GENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 1234");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::expected_variable);
    }

    SUBCASE("invalid OUTPUT_FLOAT param - undeclared variable")
    {
        build_sema("GENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid OUTPUT_FLOAT param - type mismatch")
    {
        build_sema("VAR_INT x\nGENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("invalid OUTPUT_FLOAT param - global string constant")
    {
        build_sema("VAR_FLOAT ON\nGENERATE_RANDOM_FLOAT_IN_RANGE 0.0 1.0 ON");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::cannot_use_string_constant_here);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema INPUT_INT parameter")
{
    SUBCASE("valid INPUT_INT param")
    {
        build_sema("{\nVAR_INT x\nLVAR_INT y\nWAIT x\nWAIT y\nWAIT 8\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 7);

        auto arg_1 = std::next(ir->begin(), 3)->command->args[0];
        auto arg_2 = std::next(ir->begin(), 4)->command->args[0];
        auto arg_3 = std::next(ir->begin(), 5)->command->args[0];

        REQUIRE(arg_1->as_integer() == nullptr);
        REQUIRE(arg_1->as_float() == nullptr);
        REQUIRE(std::addressof(arg_1->as_var_ref()->def)
                == symrepo.lookup_var("X", 0));

        REQUIRE(arg_2->as_integer() == nullptr);
        REQUIRE(arg_2->as_float() == nullptr);
        REQUIRE(std::addressof(arg_2->as_var_ref()->def)
                == symrepo.lookup_var("Y", 1));

        REQUIRE(arg_3->as_integer() != nullptr);
        REQUIRE(*arg_3->as_integer() == 8);
        REQUIRE(arg_3->as_float() == nullptr);
        REQUIRE(arg_3->as_var_ref() == nullptr);
    }

    SUBCASE("valid INPUT_INT param with string constants")
    {
        build_sema("{\nVAR_INT ON x\n"
                   "WAIT off\n"
                   "WAIT ON\n"
                   "CREATE_CHAR PEDTYPE_CIVMALE MEDIC 0.0 0.0 0.0 x\n"
                   "CREATE_CHAR PEDTYPE_MEDIC medic 0.0 0.0 0.0 x\n"
                   "CREATE_CHAR ON mEDic 0.0 0.0 0.0 x\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 8);

        const auto& wait_off = *std::next(ir->begin(), 2)->command;
        const auto& wait_on = *std::next(ir->begin(), 3)->command;
        const auto& create_char_male = *std::next(ir->begin(), 4)->command;
        const auto& create_char_medic = *std::next(ir->begin(), 5)->command;
        const auto& create_char_on = *std::next(ir->begin(), 6)->command;

        REQUIRE(wait_off.args[0]->as_constant()->value == 0);
        REQUIRE(wait_on.args[0]->as_constant()->value == 1);

        REQUIRE(create_char_male.args[0]->as_constant()->enum_id
                == *cmdman.find_enumeration("PEDTYPE"));
        REQUIRE(create_char_male.args[0]->as_constant()->value == 4);
        REQUIRE(create_char_male.args[1]->as_constant()->enum_id
                == *cmdman.find_enumeration("DEFAULTMODEL"));
        REQUIRE(create_char_male.args[1]->as_constant()->value == 5);

        REQUIRE(create_char_medic.args[0]->as_constant()->enum_id
                == *cmdman.find_enumeration("PEDTYPE"));
        REQUIRE(create_char_medic.args[0]->as_constant()->value == 16);
        REQUIRE(create_char_medic.args[1]->as_constant()->enum_id
                == *cmdman.find_enumeration("DEFAULTMODEL"));
        REQUIRE(create_char_medic.args[1]->as_constant()->value == 5);

        REQUIRE(std::addressof(create_char_on.args[0]->as_var_ref()->def)
                == symrepo.lookup_var("ON"));
        REQUIRE(create_char_on.args[1]->as_constant()->enum_id
                == *cmdman.find_enumeration("DEFAULTMODEL"));
        REQUIRE(create_char_on.args[1]->as_constant()->value == 5);
    }

    SUBCASE("invalid INPUT_INT param - float literal")
    {
        build_sema("WAIT 1.0");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_input_int);
    }

    SUBCASE("invalid INPUT_INT param - string literal")
    {
        build_sema("WAIT \"Hello\"");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_input_int);
    }

    SUBCASE("invalid INPUT_INT param - undeclared variable")
    {
        build_sema("WAIT x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid INPUT_INT param - mistyped variable")
    {
        build_sema("VAR_FLOAT x\nWAIT x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("invalid INPUT_INT param - non-global string constant")
    {
        build_sema("WAIT PEDTYPE_MEDIC");
        auto ir = sema.validate();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid INPUT_INT param - non-enum string constant")
    {
        build_sema("VAR_INT x\nCREATE_CHAR MEDIC ON 0.0 0.0 0.0 x");
        auto ir = sema.validate();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema INPUT_FLOAT parameter")
{
    SUBCASE("valid INPUT_FLOAT param")
    {
        build_sema("{\nVAR_FLOAT x\nLVAR_FLOAT y\n"
                   "GENERATE_RANDOM_FLOAT_IN_RANGE x y x\n"
                   "GENERATE_RANDOM_FLOAT_IN_RANGE 0.0 2.0 y\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 6);

        auto arg_1 = std::next(ir->begin(), 3)->command->args[0];
        auto arg_2 = std::next(ir->begin(), 3)->command->args[1];
        auto arg_3 = std::next(ir->begin(), 4)->command->args[1];

        REQUIRE(arg_1->as_integer() == nullptr);
        REQUIRE(arg_1->as_float() == nullptr);
        REQUIRE(std::addressof(arg_1->as_var_ref()->def)
                == symrepo.lookup_var("X", 0));

        REQUIRE(arg_2->as_integer() == nullptr);
        REQUIRE(arg_2->as_float() == nullptr);
        REQUIRE(std::addressof(arg_2->as_var_ref()->def)
                == symrepo.lookup_var("Y", 1));

        REQUIRE(arg_3->as_integer() == nullptr);
        REQUIRE(arg_3->as_float() != nullptr);
        REQUIRE(*arg_3->as_float() == 2.0f);
        REQUIRE(arg_3->as_var_ref() == nullptr);
    }

    SUBCASE("invalid INPUT_FLOAT param - integer literal")
    {
        build_sema("VAR_FLOAT x\nGENERATE_RANDOM_FLOAT_IN_RANGE 0 2.0 x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_input_float);
    }

    SUBCASE("invalid INPUT_FLOAT param - string literal")
    {
        build_sema(
                "VAR_FLOAT x\nGENERATE_RANDOM_FLOAT_IN_RANGE 0.0 \"Hello\" x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_input_float);
    }

    SUBCASE("invalid INPUT_FLOAT param - undeclared variable")
    {
        build_sema("VAR_FLOAT x\nGENERATE_RANDOM_FLOAT_IN_RANGE x y x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid INPUT_FLOAT param - mistyped variable")
    {
        build_sema(
                "VAR_FLOAT x\nVAR_INT y\nGENERATE_RANDOM_FLOAT_IN_RANGE x y x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("invalid INPUT_FLOAT param - global string constant")
    {
        build_sema("VAR_FLOAT ON x\nGENERATE_RANDOM_FLOAT_IN_RANGE 0.0 ON x");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::cannot_use_string_constant_here);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema INPUT_OPT parameter")
{
    SUBCASE("valid INPUT_OPT param")
    {
        build_sema("{\nVAR_INT gi\nLVAR_INT li\n"
                   "VAR_FLOAT gf\nLVAR_FLOAT lf\n"
                   "LVAR_INT in3\nLVAR_FLOAT in4\nLVAR_INT in5\n"
                   "LVAR_FLOAT in6\nLVAR_INT in7\n"
                   "VAR_INT ON\n"
                   "label1: START_NEW_SCRIPT label1 gi gf li lf 123 1.0 ON\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 13);

        auto cmd = std::prev(ir->end(), 2)->command;
        REQUIRE(cmd->args.size() == 8);

        auto arg_1 = cmd->args[1];
        auto arg_2 = cmd->args[2];
        auto arg_3 = cmd->args[3];
        auto arg_4 = cmd->args[4];
        auto arg_5 = cmd->args[5];
        auto arg_6 = cmd->args[6];
        auto arg_7 = cmd->args[7];

        REQUIRE(arg_1->as_integer() == nullptr);
        REQUIRE(arg_1->as_float() == nullptr);
        REQUIRE(std::addressof(arg_1->as_var_ref()->def)
                == symrepo.lookup_var("GI", 0));
        REQUIRE(arg_1->as_constant() == nullptr);

        REQUIRE(arg_2->as_integer() == nullptr);
        REQUIRE(arg_2->as_float() == nullptr);
        REQUIRE(std::addressof(arg_2->as_var_ref()->def)
                == symrepo.lookup_var("GF", 0));
        REQUIRE(arg_2->as_constant() == nullptr);

        REQUIRE(arg_3->as_integer() == nullptr);
        REQUIRE(arg_3->as_float() == nullptr);
        REQUIRE(std::addressof(arg_3->as_var_ref()->def)
                == symrepo.lookup_var("LI", 1));
        REQUIRE(arg_3->as_constant() == nullptr);

        REQUIRE(arg_4->as_integer() == nullptr);
        REQUIRE(arg_4->as_float() == nullptr);
        REQUIRE(std::addressof(arg_4->as_var_ref()->def)
                == symrepo.lookup_var("LF", 1));
        REQUIRE(arg_4->as_constant() == nullptr);

        REQUIRE(arg_5->as_integer() != nullptr);
        REQUIRE(*arg_5->as_integer() == 123);
        REQUIRE(arg_5->as_float() == nullptr);
        REQUIRE(arg_5->as_var_ref() == nullptr);
        REQUIRE(arg_5->as_constant() == nullptr);

        REQUIRE(arg_6->as_integer() == nullptr);
        REQUIRE(arg_6->as_float() != nullptr);
        REQUIRE(*arg_6->as_float() == 1.0f);
        REQUIRE(arg_6->as_var_ref() == nullptr);
        REQUIRE(arg_6->as_constant() == nullptr);

        REQUIRE(arg_7->as_integer() == nullptr);
        REQUIRE(arg_7->as_float() == nullptr);
        REQUIRE(arg_7->as_var_ref() == nullptr);
        REQUIRE(arg_7->as_constant() != nullptr);
        REQUIRE(arg_7->as_constant()->enum_id == cmdman.global_enum);
        REQUIRE(arg_7->as_constant()->value == 1);
    }

    SUBCASE("invalid INPUT_OPT param - string literal")
    {
        build_sema("label1: START_NEW_SCRIPT label1 \"Hello\"");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_input_opt);
    }

    SUBCASE("invalid INPUT_OPT param - undeclared variable")
    {
        build_sema("label1: START_NEW_SCRIPT label1 x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid INPUT_OPT param - mistyped variable")
    {
        build_sema("VAR_TEXT_LABEL var\nlabel1: START_NEW_SCRIPT label1 var");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("invalid INPUT_OPT param - non-global string constant")
    {
        build_sema("label1: START_NEW_SCRIPT label1 MEDIC");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema validate subscript")
{
    SUBCASE("valid variable subscript")
    {
        build_sema("VAR_INT x[10] y z[10]\n"
                   "SET_VAR_INT x[9] 3\n"
                   "SET_VAR_INT x[y] 3\n"
                   "SET_VAR_INT x 3\n"
                   "SET_VAR_INT y 3\n");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 5);

        auto var_ref_1
                = std::prev(ir->end(), 4)->command->args[0]->as_var_ref();
        auto var_ref_2
                = std::prev(ir->end(), 3)->command->args[0]->as_var_ref();
        auto var_ref_3
                = std::prev(ir->end(), 2)->command->args[0]->as_var_ref();
        auto var_ref_4
                = std::prev(ir->end(), 1)->command->args[0]->as_var_ref();

        REQUIRE(var_ref_1 != nullptr);
        REQUIRE(std::addressof(var_ref_1->def) == symrepo.lookup_var("X"));
        REQUIRE(var_ref_1->has_index());
        REQUIRE(*var_ref_1->index_as_integer() == 9);
        REQUIRE(var_ref_1->index_as_variable() == nullptr);

        REQUIRE(var_ref_2 != nullptr);
        REQUIRE(std::addressof(var_ref_2->def) == symrepo.lookup_var("X"));
        REQUIRE(var_ref_2->has_index());
        REQUIRE(var_ref_2->index_as_integer() == nullptr);
        REQUIRE(var_ref_2->index_as_variable() == symrepo.lookup_var("Y"));

        REQUIRE(var_ref_3 != nullptr);
        REQUIRE(std::addressof(var_ref_3->def) == symrepo.lookup_var("X"));
        REQUIRE(var_ref_3->has_index());
        REQUIRE(*var_ref_3->index_as_integer() == 0);
        REQUIRE(var_ref_3->index_as_variable() == nullptr);

        REQUIRE(var_ref_4 != nullptr);
        REQUIRE(std::addressof(var_ref_4->def) == symrepo.lookup_var("Y"));
        REQUIRE(!var_ref_4->has_index());
        REQUIRE(var_ref_4->index_as_integer() == nullptr);
        REQUIRE(var_ref_4->index_as_variable() == nullptr);
    }

    SUBCASE("invalid variable subscript - not array")
    {
        build_sema("VAR_INT x\nSET_VAR_INT x[0] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::subscript_but_var_is_not_array);
    }

    SUBCASE("invalid variable subscript - not array")
    {
        build_sema("VAR_INT x\nSET_VAR_INT x[0] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::subscript_but_var_is_not_array);
    }

    SUBCASE("invalid variable subscript - out of bounds")
    {
        build_sema("VAR_INT x[10]\nSET_VAR_INT x[10] 0\nSET_VAR_INT x[99] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::subscript_out_of_range);
        CHECK(consume_diag().message == gta3sc::Diag::subscript_out_of_range);
    }

    SUBCASE("invalid variable subscript - subscript var undeclared")
    {
        build_sema("VAR_INT x[10]\nSET_VAR_INT x[y] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("invalid variable subscript - subscript var is not int")
    {
        build_sema("VAR_INT x[10]\nVAR_FLOAT y\nSET_VAR_INT x[y] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::subscript_var_must_be_int);
    }

    SUBCASE("invalid variable subscript - subscript var is array")
    {
        build_sema("VAR_INT x[10]\nVAR_INT y[10]\nSET_VAR_INT x[y] 0\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::subscript_var_must_not_be_array);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema too few arguments")
{
    /* Responsability of the parser now :)
    SUBCASE("VAR_INT with no args")
    {
        build_sema("VAR_INT ");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
    */

    SUBCASE("START_NEW_SCRIPT with no args")
    {
        build_sema("START_NEW_SCRIPT ");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }

    SUBCASE("START_NEW_SCRIPT with single arg")
    {
        build_sema("{\nlabel1: START_NEW_SCRIPT label1\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("GENERATE_RANDOM_INT_IN_RANGE with single arg")
    {
        build_sema("GENERATE_RANDOM_INT_IN_RANGE 10");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema too many arguments")
{
    SUBCASE("99 optional args")
    {
        build_sema(
                "{\n"
                "LVAR_INT a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 a10 a11 a12 a13 a14"
                " a15 a16 a17 a18 a19 a20 a21 a22 a23 a24 a25 a26 a27 a28"
                " a29 a30 a31 a32 a33 a34 a35 a36 a37 a38 a39 a40 a41 a42"
                " a43 a44 a45 a46 a47 a48 a49 a50 a51 a52 a53 a54 a55 a56"
                " a57 a58 a59 a60 a61 a62 a63 a64 a65 a66 a67 a68 a69 a70"
                " a71 a72 a73 a74 a75 a76 a77 a78 a79 a80 a81 a82 a83 a84"
                " a85 a86 a87 a88 a89 a90 a91 a92 a93 a94 a95 a96 a97 a98\n"
                "label1: START_NEW_SCRIPT label1 a0 a1 a2 a3 a4 a5 a6 a7 a8"
                " a9 a10 a11 a12 a13 a14 a15 a16 a17 a18 a19 a20 a21 a22 a23"
                " a24 a25 a26 a27 a28 a29 a30 a31 a32 a33 a34 a35 a36 a37 a38"
                " a39 a40 a41 a42 a43 a44 a45 a46 a47 a48 a49 a50 a51 a52 a53"
                " a54 a55 a56 a57 a58 a59 a60 a61 a62 a63 a64 a65 a66 a67 a68"
                " a69 a70 a71 a72 a73 a74 a75 a76 a77 a78 a79 a80 a81 a82 a83"
                " a84 a85 a86 a87 a88 a89 a90 a91 a92 a93 a94 a95 a96 a97 a98\n"
                "}\n");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("too many arguments in WAIT")
    {
        build_sema("WAIT 10 11\nWAIT 10 11 12 13 14 15 16 17\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::too_many_arguments);
        CHECK(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema alternators")
{
    SUBCASE("alternative match - valid matches")
    {
        build_sema("{\nVAR_INT gi\nVAR_FLOAT gf\nLVAR_INT li\nLVAR_FLOAT lf\n"
                   "ABS gi\n"
                   "ABS li\n"
                   "ABS gf\n"
                   "ABS lf\n"
                   "SET gi 10\n"
                   "SET gf 1.0\n"
                   "SET gi li\n"
                   "SET lf gf\n}");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(&std::next(ir->begin(), 5)->command->def
                == cmdman.find_command("ABS_VAR_INT"));
        REQUIRE(&std::next(ir->begin(), 6)->command->def
                == cmdman.find_command("ABS_LVAR_INT"));
        REQUIRE(&std::next(ir->begin(), 7)->command->def
                == cmdman.find_command("ABS_VAR_FLOAT"));
        REQUIRE(&std::next(ir->begin(), 8)->command->def
                == cmdman.find_command("ABS_LVAR_FLOAT"));
        REQUIRE(&std::next(ir->begin(), 9)->command->def
                == cmdman.find_command("SET_VAR_INT"));
        REQUIRE(&std::next(ir->begin(), 10)->command->def
                == cmdman.find_command("SET_VAR_FLOAT"));
        REQUIRE(&std::next(ir->begin(), 11)->command->def
                == cmdman.find_command("SET_VAR_INT_TO_LVAR_INT"));
        REQUIRE(&std::next(ir->begin(), 12)->command->def
                == cmdman.find_command("SET_LVAR_FLOAT_TO_VAR_FLOAT"));
    }

    SUBCASE("alternative match - otherwise case, text label")
    {
        build_sema("VAR_TEXT_LABEL x\nSET x y\nSET x $x\n");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);

        const auto& command_1 = *std::next(ir->begin(), 1)->command;
        const auto& command_2 = *std::next(ir->begin(), 2)->command;

        REQUIRE(&command_1.def == cmdman.find_command("SET_VAR_TEXT_LABEL"));
        REQUIRE(std::addressof(command_1.args[0]->as_var_ref()->def)
                == symrepo.lookup_var("X"));
        REQUIRE(*command_1.args[1]->as_text_label() == "Y"sv);

        REQUIRE(&command_2.def == cmdman.find_command("SET_VAR_TEXT_LABEL"));
        REQUIRE(std::addressof(command_2.args[0]->as_var_ref()->def)
                == symrepo.lookup_var("X"));
        REQUIRE(std::addressof(command_2.args[1]->as_var_ref()->def)
                == symrepo.lookup_var("X"));
    }

    SUBCASE("alternative match - string constants")
    {
        build_sema("VAR_INT x\nSET x ON\nSET x CHEETAH");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);

        const auto& command_1 = *std::next(ir->begin(), 1)->command;
        const auto& command_2 = *std::next(ir->begin(), 2)->command;

        REQUIRE(&command_1.def == cmdman.find_command("SET_VAR_INT"));
        REQUIRE(std::addressof(command_1.args[0]->as_var_ref()->def)
                == symrepo.lookup_var("X"));
        REQUIRE(command_1.args[1]->as_constant()->value == 1);

        REQUIRE(&command_2.def
                == cmdman.find_command("SET_VAR_INT_TO_CONSTANT"));
        REQUIRE(std::addressof(command_2.args[0]->as_var_ref()->def)
                == symrepo.lookup_var("X"));
        REQUIRE(command_2.args[1]->as_constant()->value == 145);
    }

    SUBCASE("no alternative match - string constants")
    {
        build_sema("VAR_INT x\nABS ON\nABS CHEETAH\nSET x INVAL_CONST");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - wrong number of arguments")
    {
        build_sema("VAR_INT x\nABS x x\nABS");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - literal in variable only")
    {
        build_sema("ABS 10\nABS 1.0\nABS x\nABS \"Hello\"");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - local in global only")
    {
        build_sema("{\nVAR_INT x\nLVAR_INT y\nLVAR_FLOAT z\n"
                   "ACCEPTS_ONLY_GLOBAL_VAR x\n"
                   "ACCEPTS_ONLY_GLOBAL_VAR x\n"
                   "ACCEPTS_ONLY_GLOBAL_VAR y\n"
                   "ACCEPTS_ONLY_GLOBAL_VAR z\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - global in local only")
    {
        build_sema("{\nLVAR_INT x\nVAR_INT y\nVAR_FLOAT z\n"
                   "ACCEPTS_ONLY_LOCAL_VAR x\n"
                   "ACCEPTS_ONLY_LOCAL_VAR x\n"
                   "ACCEPTS_ONLY_LOCAL_VAR y\n"
                   "ACCEPTS_ONLY_LOCAL_VAR z\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - text label variable in int/float only")
    {
        build_sema("{\nVAR_TEXT_LABEL x\nLVAR_TEXT_LABEL y\n"
                   "ABS x\nABS y\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }

    SUBCASE("no alternative match - types mismatch")
    {
        build_sema("{\nVAR_INT i\nLVAR_FLOAT f\nVAR_INT ON\n"
                   "SET i 2.0\n"
                   "SET f 2\n"
                   "SET 2 i\n"
                   "SET 2.0 f\n"
                   "SET ON 1\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::alternator_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema entities")
{
    const auto no_entity = cmdman.no_entity_type;
    const auto entity_car = cmdman.find_entity_type("CAR").value();
    const auto entity_char = cmdman.find_entity_type("CHAR").value();

    SUBCASE("valid entity usage")
    {
        build_sema("VAR_INT car ped x\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped\n"
                   "SET_CAR_HEADING car 0.0\n"
                   "SET_CHAR_HEADING ped 0.0\n");
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR")) == entity_car);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("PED"))
                == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("X")) == no_entity);
    }

    SUBCASE("valid entity usage - passing entity to/from integer")
    {
        build_sema("VAR_INT car\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "SET_CAR_HEADING 1234 0.0\n" // INT instead of CAR
                   "WAIT car\n");               // CAR instead of INT
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR")) == entity_car);
    }

    SUBCASE("invalid entity usage")
    {
        build_sema("VAR_INT car ped x y\n"
                   "SET x 10\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped\n"
                   "SET_CAR_HEADING ped 0.0\n"  // PED instead of CAR
                   "SET_CHAR_HEADING car 0.0\n" // CAR instead of PED
                   "SET_CAR_HEADING x 0.0\n"    // NONE instead of CAR
                   "SET_CHAR_HEADING y 0.0\n"); // NONE instead of CHAR
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
    }

    SUBCASE("invalid entity usage - reinitialization")
    {
        build_sema("VAR_INT car ped\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 car\n"); // BAD
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
    }

    SUBCASE("valid entity assignment")
    {
        build_sema("VAR_INT car1 car2 ped1 ped2 x y z w\n"
                   "SET x 10\n"
                   "SET z 11\n"
                   "SET ped2 12\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car1\n"
                   "CREATE_CAR WASHING 0.0 0.0 0.0 car2\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped1\n"
                   "CREATE_CHAR PEDTYPE_CIVFEMALE HFYST 0.0 0.0 0.0 ped2\n"
                   "SET car1 car2\n" // CAR = CAR
                   "SET car2 car1\n" // CAR = CAR
                   "SET car2 car2\n" // CAR = CAR
                   "SET ped1 ped2\n" // PED = PED
                   "SET y ped2\n"    // NONE = PED
                   "SET x y\n"       // NONE = PED
                   "SET x -1\n"      // PED = INT (unchecked)
                   "SET ped2 x\n");  // PED = PED (even after INT assignment)
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR1"))
                == entity_car);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR2"))
                == entity_car);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("PED1"))
                == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("PED2"))
                == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("X")) == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("Y")) == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("Z")) == no_entity);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("W")) == no_entity);
    }

    SUBCASE("invalid entity assignment - from one type to another")
    {
        build_sema("VAR_INT car1 car2 ped1 ped2 x\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car1\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped1\n"
                   "SET car2 car1\n" // NONE = CAR, OK
                   "SET ped2 ped1\n" // NONE = PED, OK
                   "SET ped1 car1\n" // PED = CAR, BAD
                   "SET car1 ped2\n" // CAR = PED, BAD
                   "SET ped2 x\n"    // PED = NONE, BAD
                   "SET ped2 x\n");  // CAR = NONE, BAD
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
    }

    SUBCASE("entities propagates through arrays")
    {
        build_sema("VAR_INT car[10] ped[10] x y z w\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car[3]\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped[0]\n"
                   "SET x ped[9]\n"
                   "SET_CAR_HEADING car[5] 0.0\n");
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR")) == entity_car);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("PED"))
                == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("X")) == entity_char);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("Y")) == no_entity);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("Z")) == no_entity);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("W")) == no_entity);
    }

    SUBCASE("entity initialization does not affect previous lines")
    {
        build_sema("VAR_INT car x y\n"
                   "SET car x\n" // NONE = NONE (not CAR = NONE -- invalid)
                   "SET x car\n" // NONE = NONE (not NONE = CAR)
                   "SET y car\n" // NONE = NONE (not NONE = CAR)
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "SET y car\n"); // NONE = CAR
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("CAR")) == entity_car);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("X")) == no_entity);
        REQUIRE(sema.var_entity_type(*symrepo.lookup_var("Y")) == entity_car);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema hardcoded REPEAT with local variable")
{
    build_sema("{\nVAR_INT x\nLVAR_INT y\n"
               "REPEAT 10 x\n"
               "    REPEAT 20 y\n"
               "    ENDREPEAT\n"
               "ENDREPEAT\n}");
    REQUIRE(sema.validate() != std::nullopt);
}

TEST_CASE_FIXTURE(SemaFixture, "sema hardcoded SCRIPT_NAME")
{
    SUBCASE("many SCRIPT_NAMEs with no duplicate")
    {
        build_sema("SCRIPT_NAME HELLO\nSCRIPT_NAME world\n"
                   "SCRIPT_NAME three\nSCRIPT_NAME FOURTH\n");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("duplicate SCRIPT_NAME is invalid")
    {
        build_sema("SCRIPT_NAME HELLO\nSCRIPT_NAME wOrLD\n"
                   "SCRIPT_NAME hello\nSCRIPT_NAME FOURTH\n"
                   "SCRIPT_NAME FIFTH\nSCRIPT_NAME SIXTH\n"
                   "SCRIPT_NAME SEVENTH\nSCRIPT_NAME WORLD\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_script_name);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_script_name);
    }

    SUBCASE("SCRIPT_NAME with text label variable") // unspecified in the spec
    {
        build_sema("VAR_TEXT_LABEL var\n"
                   "SCRIPT_NAME $var\nSCRIPT_NAME $var\nSCRIPT_NAME var");
        REQUIRE(sema.validate() != std::nullopt);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema hardcoded START_NEW_SCRIPT")
{
    SUBCASE("valid - no argument passing")
    {
        build_sema("\n{\nlabel1: START_NEW_SCRIPT label1\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("invalid - label outside of scope")
    {
        build_sema("label1: START_NEW_SCRIPT label1 10 20 30");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message
                == gta3sc::Diag::target_label_not_within_scope);
    }

    SUBCASE("valid - more target vars than arguments")
    {
        build_sema("{\nLVAR_INT a b c d e f g h\nlabel1: START_NEW_SCRIPT "
                   "label1 10 20\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("invalid - more arguments than target vars")
    {
        build_sema("{\nLVAR_INT x y\nlabel1: START_NEW_SCRIPT label1 10 20 30 "
                   "40 50\n}");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message
                == gta3sc::Diag::target_scope_not_enough_vars);
        REQUIRE(consume_diag().message
                == gta3sc::Diag::target_scope_not_enough_vars);
        REQUIRE(consume_diag().message
                == gta3sc::Diag::target_scope_not_enough_vars);
    }

    SUBCASE("valid - passing integers")
    {
        build_sema("{\nLVAR_INT a1 a2 a3\nlabel1: START_NEW_SCRIPT label1 10 "
                   "ON a3\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("valid - passing floats")
    {
        build_sema("{\nLVAR_FLOAT a1 a2 a3\nlabel1: START_NEW_SCRIPT label1 "
                   "1.0 a2 a3\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("invalid - passing text labels")
    {
        build_sema("{\nLVAR_TEXT_LABEL a1 a2\nlabel1: START_NEW_SCRIPT label1 "
                   "TEXT a2\n}");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::undefined_variable);
        CHECK(consume_diag().message == gta3sc::Diag::var_type_mismatch);
    }

    SUBCASE("valid - passing variables")
    {
        build_sema("{\nLVAR_INT x\nLVAR_FLOAT y\nlabel1: START_NEW_SCRIPT "
                   "label1 x y\n}");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("invalid - target variable type mismatch")
    {
        build_sema("{\n"
                   "LVAR_INT i1 i2 i3\n"
                   "LVAR_FLOAT f1 f2 f3 f4\n"
                   "label1: START_NEW_SCRIPT label1 i1 f1 1.0 f1 i1 ON 10\n"
                   "}\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_type_mismatch); // f1
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_type_mismatch); // 1.0
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_type_mismatch); // i1
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_type_mismatch); // ON
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_type_mismatch); // 10
    }

    SUBCASE("valid - passing entity type forward")
    {
        build_sema("VAR_INT car\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "START_NEW_SCRIPT label1 car\n"
                   "{\n"
                   "label1: LVAR_INT local_car\n"
                   "SET_CAR_HEADING local_car 0.0\n"
                   "}\n");
        REQUIRE(sema.validate() != std::nullopt);
    }

    SUBCASE("invalid - passing entity type backwards")
    {
        build_sema("{\n"
                   "label1: LVAR_INT local_car\n"
                   "SET_CAR_HEADING local_car 0.0\n"
                   "}\n"
                   "VAR_INT car\n"
                   "CREATE_CAR CHEETAH 0.0 0.0 0.0 car\n"
                   "START_NEW_SCRIPT label1 car\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
    }

    SUBCASE("invalid - passing wrong entity type backwards")
    {
        build_sema("{\n"
                   "label1: LVAR_INT local_car\n"
                   "CREATE_CAR WASHING 0.0 0.0 0.0 local_car\n"
                   "SET_CAR_HEADING local_car 0.0\n"
                   "}\n"
                   "VAR_INT ped\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped\n"
                   "START_NEW_SCRIPT label1 ped\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message
              == gta3sc::Diag::target_var_entity_type_mismatch);
    }

    SUBCASE("invalid - passing wrong entity type forward")
    {
        build_sema("VAR_INT ped\n"
                   "CREATE_CHAR PEDTYPE_MEDIC MEDIC 0.0 0.0 0.0 ped\n"
                   "START_NEW_SCRIPT label1 ped\n"
                   "{\n"
                   "label1: LVAR_INT local_car\n"
                   "SET_CAR_HEADING local_car 0.0\n"
                   "}\n");
        REQUIRE(sema.validate() == std::nullopt);
        CHECK(consume_diag().message == gta3sc::Diag::var_entity_type_mismatch);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema used objects")
{
    const auto defaultmodel_enum
            = cmdman.find_enumeration("DEFAULTMODEL").value();

    SUBCASE("DEFAULTMODEL has precedence over level model")
    {
        build_sema("VAR_INT x\nCREATE_OBJECT CHEETAH 0.0 0.0 0.0 x");

        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);

        CHECK(cmdman.find_constant(defaultmodel_enum, "CHEETAH"));
        CHECK(modelman.find_model("CHEETAH") != nullptr);

        REQUIRE(ir->back().command);
        REQUIRE(ir->back().command->args.size() == 5);
        REQUIRE(!ir->back().command->args[0]->as_used_object());
        REQUIRE(ir->back().command->args[0]->as_constant());
        REQUIRE(ir->back().command->args[0]->as_constant()->enum_id
                == defaultmodel_enum);
        REQUIRE(ir->back().command->args[0]->as_constant()->value
                == cmdman.find_constant(defaultmodel_enum, "CHEETAH")->value);

        REQUIRE(!symrepo.lookup_used_object("CHEETAH"));
    }

    SUBCASE("level model has precedence over variable reference")
    {
        build_sema("VAR_INT LEVEL_MODEL\n"
                   "CREATE_OBJECT LEVEL_MODEL 0.0 0.0 0.0 LEVEL_MODEL");

        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);

        CHECK(modelman.find_model("LEVEL_MODEL") != nullptr);

        REQUIRE(ir->back().command);
        REQUIRE(ir->back().command->args.size() == 5);
        REQUIRE(!ir->back().command->args[0]->as_constant());
        REQUIRE(ir->back().command->args[0]->as_used_object());
        REQUIRE(ir->back().command->args[0]->as_used_object()
                == symrepo.lookup_used_object("LEVEL_MODEL"));

        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL")->id == 0);
    }

    SUBCASE("variable may be used as argument to object model param")
    {
        build_sema("VAR_INT x\n"
                   "CREATE_OBJECT x 0.0 0.0 0.0 x");

        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);

        CHECK(modelman.find_model("X") == nullptr);

        REQUIRE(ir->back().command);
        REQUIRE(ir->back().command->args.size() == 5);
        REQUIRE(ir->back().command->args[0]->as_var_ref());
        REQUIRE(std::addressof(ir->back().command->args[0]->as_var_ref()->def)
                == symrepo.lookup_var("X"));

        REQUIRE(symrepo.lookup_used_object("X") == nullptr);
    }

    SUBCASE("used object model must be known to exist")
    {
        build_sema("VAR_INT x\nCREATE_OBJECT NOT_LEVEL_MODEL 0.0 0.0 0.0 x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("level model does not affect DEFAULTMODEL params")
    {
        build_sema("VAR_INT x\nCREATE_CAR LEVEL_MODEL 0.0 0.0 0.0 x");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::undefined_variable);
    }

    SUBCASE("level models do not affect the namespace of symbols seen by var "
            "declaration")
    {
        build_sema("VAR_INT LEVEL_MODEL");
        REQUIRE(sema.validate() != std::nullopt);
        CHECK(modelman.find_model("LEVEL_MODEL") != nullptr);
    }

    SUBCASE("multiple used objects have different sequential ids (normal)")
    {
        build_sema("VAR_INT x\n"
                   "CREATE_OBJECT LEVEL_MODEL 0.0 0.0 0.0 x\n"
                   "CREATE_OBJECT OTHER_LEVEL_MODEL 0.0 0.0 0.0 x");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);

        REQUIRE(std::next(ir->begin(), 1)->command);
        REQUIRE(std::next(ir->begin(), 1)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 1)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("LEVEL_MODEL"));

        REQUIRE(std::next(ir->begin(), 2)->command);
        REQUIRE(std::next(ir->begin(), 2)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 2)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("OTHER_LEVEL_MODEL"));

        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL"));
        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL")->id == 0);
        REQUIRE(symrepo.lookup_used_object("OTHER_LEVEL_MODEL"));
        REQUIRE(symrepo.lookup_used_object("OTHER_LEVEL_MODEL")->id == 1);
    }

    SUBCASE("multiple used objects have different sequential ids (reversed)")
    {
        build_sema("VAR_INT x\n"
                   "CREATE_OBJECT OTHER_LEVEL_MODEL 0.0 0.0 0.0 x\n"
                   "CREATE_OBJECT LEVEL_MODEL 0.0 0.0 0.0 x");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);

        REQUIRE(std::next(ir->begin(), 1)->command);
        REQUIRE(std::next(ir->begin(), 1)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 1)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("OTHER_LEVEL_MODEL"));

        REQUIRE(std::next(ir->begin(), 2)->command);
        REQUIRE(std::next(ir->begin(), 2)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 2)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("LEVEL_MODEL"));

        REQUIRE(symrepo.lookup_used_object("OTHER_LEVEL_MODEL"));
        REQUIRE(symrepo.lookup_used_object("OTHER_LEVEL_MODEL")->id == 0);
        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL"));
        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL")->id == 1);
    }

    SUBCASE("multiple usages of used object do not define multiple entries in "
            "symbol table")
    {
        build_sema("VAR_INT x\n"
                   "CREATE_OBJECT LEVEL_MODEL 0.0 0.0 0.0 x\n"
                   "CREATE_OBJECT LEVEL_MODEL 0.0 0.0 0.0 x\n");
        auto ir = sema.validate();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);

        REQUIRE(std::next(ir->begin(), 1)->command);
        REQUIRE(std::next(ir->begin(), 1)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 1)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("LEVEL_MODEL"));

        REQUIRE(std::next(ir->begin(), 2)->command);
        REQUIRE(std::next(ir->begin(), 2)->command->args.size() == 5);
        REQUIRE(std::next(ir->begin(), 2)->command->args[0]->as_used_object()
                == symrepo.lookup_used_object("LEVEL_MODEL"));

        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL"));
        REQUIRE(symrepo.lookup_used_object("LEVEL_MODEL")->id == 0);
        REQUIRE(symrepo.used_objects.size() == 1);
    }

    SUBCASE("unused level models is not an used object")
    {
        build_sema("VAR_INT x\nCREATE_OBJECT CHEETAH 0.0 0.0 0.0 x");
        REQUIRE(sema.validate() != std::nullopt);

        CHECK(modelman.find_model("LEVEL_MODEL") != nullptr);
        CHECK(modelman.find_model("OTHER_LEVEL_MODEL") != nullptr);

        REQUIRE(!symrepo.lookup_used_object("LEVEL_MODEL"));
        REQUIRE(!symrepo.lookup_used_object("OTHER_LEVEL_MODEL"));
    }
}

TEST_CASE_FIXTURE(SemaFixture, "local timers")
{
    SUBCASE("successful timer creation")
    {
        build_sema("{\n}");
        REQUIRE(sema.validate() != std::nullopt);

        const auto timera = symrepo.lookup_var("TIMERA", 1);
        const auto timerb = symrepo.lookup_var("TIMERB", 1);

        REQUIRE(timera != nullptr);
        REQUIRE(timerb != nullptr);

        SUBCASE("timers are not arrays")
        {
            REQUIRE(!timera->is_array());
            REQUIRE(!timerb->is_array());
        }

        SUBCASE("timers are integers")
        {
            REQUIRE(timera->type == gta3sc::SymVariable::Type::INT);
            REQUIRE(timera->type == gta3sc::SymVariable::Type::INT);
        }

        SUBCASE("timers use the first variable indices")
        {
            REQUIRE(timera->id == 0);
            REQUIRE(timerb->id == 1);
        }
    }

    SUBCASE("timers do not exist in global scope")
    {
        build_sema("{\n}");
        REQUIRE(sema.validate() != std::nullopt);
        REQUIRE(symrepo.lookup_var("TIMERA") == nullptr);
        REQUIRE(symrepo.lookup_var("TIMERB") == nullptr);
    }

    SUBCASE("cannot declare global variable with name of timers")
    {
        build_sema("VAR_INT timera timerb");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::duplicate_var_timer);
        REQUIRE(consume_diag().message == gta3sc::Diag::duplicate_var_timer);
    }

    SUBCASE("cannot declare local variable with name of timers")
    {
        build_sema("{\nLVAR_INT timera timerb\n}");
        REQUIRE(sema.validate() == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::duplicate_var_timer);
        REQUIRE(consume_diag().message == gta3sc::Diag::duplicate_var_timer);
    }

    SUBCASE("timer ids are at the end of the scope")
    {
        build_sema("{\nLVAR_INT x\n}");
        REQUIRE(sema.validate() != std::nullopt);

        const auto timera = symrepo.lookup_var("TIMERA", 1);
        const auto timerb = symrepo.lookup_var("TIMERB", 1);
        const auto x = symrepo.lookup_var("X", 1);

        REQUIRE(x != nullptr);
        REQUIRE(x->id == 0);

        REQUIRE(timera != nullptr);
        REQUIRE(timera->id == 1);

        REQUIRE(timerb != nullptr);
        REQUIRE(timerb->id == 2);
    }
}

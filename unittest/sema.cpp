#include <doctest.h>
#include <gta3sc/sema.hpp>
#include <gta3sc/parser.hpp>
#include <cstring>
#include "compiler-fixture.hpp"
using namespace std::literals::string_view_literals;

namespace
{
class SemaFixture : public CompilerFixture
{
public:
    SemaFixture() :
        sema(default_sema())
    {}

    void build_sema(std::string_view src, bool parser_error = false)
    {
        this->build_source(src);
        auto ir = make_parser(source_file, arena).parse_main_script_file();
        if(!parser_error)
        {
            REQUIRE(ir != std::nullopt);
            this->sema = gta3sc::Sema(std::move(*ir), symrepo, diagman, arena);
        }
        else
        {
            REQUIRE(ir == std::nullopt);
            this->sema = default_sema();
        }
    }

private:
    auto make_parser(const gta3sc::SourceFile& source,
                     gta3sc::ArenaMemoryResource& arena)
            -> gta3sc::Parser
    {
        auto pp = gta3sc::Preprocessor(source, diagman);
        auto scanner = gta3sc::Scanner(std::move(pp));
        return gta3sc::Parser(std::move(scanner), arena);
    }

    auto default_sema() -> gta3sc::Sema
    {
        return gta3sc::Sema(gta3sc::LinkedIR<gta3sc::ParserIR>(arena),
                            symrepo, diagman, arena);
    }

    gta3sc::ArenaMemoryResource arena;
protected:
    gta3sc::SymbolRepository symrepo;
    gta3sc::Sema sema;
};
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

    sema.pass_declarations();
    
    gta3sc::SymVariable* gvar[10] = {};
    gta3sc::SymVariable* lvar1[10] = {};
    gta3sc::SymVariable* lvar2[10] = {};

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
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::var_decl_subscript_must_be_literal);
    }

    SUBCASE("zero integer literal subscript")
    {
        build_sema("VAR_INT x[0] z[10]");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::var_decl_subscript_must_be_nonzero);
    }

    SUBCASE("local variable declaration outside of scope")
    {
        build_sema("LVAR_INT x y z");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::var_decl_outside_of_scope);
        CHECK(consume_diag().message == gta3sc::Diag::var_decl_outside_of_scope);
        CHECK(consume_diag().message == gta3sc::Diag::var_decl_outside_of_scope);
    }

    SUBCASE("variable decl name is not identifier")
    {
        build_sema("VAR_INT x 9 z");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::expected_identifier);
    }

    SUBCASE("global variable duplicate")
    {
        build_sema("VAR_INT x x y\nVAR_INT y\n");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_global);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_global);
    }

    SUBCASE("local variable duplicate")
    {
        build_sema("{\nLVAR_INT x x y\nLVAR_INT y\n}\n");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_in_scope);
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_in_scope);
    }

    SUBCASE("local variable in different scopes is not a duplicate")
    {
        build_sema("{\nLVAR_INT x y\n}\n{\nLVAR_INT y\n}\n");
        sema.pass_declarations();
        CHECK(diags.empty());
    }

    SUBCASE("local variable with same name as global variable")
    {
        build_sema("VAR_INT x\n{\nLVAR_INT x\n}");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_lvar);
    }

    SUBCASE("global variable with same name as local variable")
    {
        build_sema("{\nLVAR_INT x\n}\nVAR_INT x");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_var_lvar);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema label declaration")
{
    SUBCASE("no label duplicate")
    {
        build_sema("label1:\nlabel2\n");
        sema.pass_declarations();
        CHECK(diags.empty());
    }

    SUBCASE("label duplicate")
    {
        build_sema("label1:\nlabel1:\n");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::duplicate_label);
    }
}

TEST_CASE_FIXTURE(SemaFixture, "sema different symbol tables do not collide")
{
    build_sema("x:\n"
               "y:\n"
               "VAR_INT x\n"
               "{\nLVAR_INT y\n}\n");
    CHECK(diags.empty());
}

TEST_CASE_FIXTURE(SemaFixture, "sema parsing variable reference")
{
    SUBCASE("empty subscript")
    {
        build_sema("VAR_INT x[] z[10]");
        sema.pass_declarations();
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
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::integer_literal_too_big);
    }

    SUBCASE("floating point subscript")
    {
        build_sema("VAR_INT x[2.0] z[10]");
        sema.pass_declarations();
        CHECK(consume_diag().message == gta3sc::Diag::expected_integer);
    }

    SUBCASE("swap brackets")
    {
        build_sema("VAR_INT x]10[ z[10]");
        sema.pass_declarations();
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == "[");
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == "]");
    }

    SUBCASE("missing opening bracket")
    {
        build_sema("VAR_INT x10] z[10]");
        sema.pass_declarations();
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == "[");
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == "]");
        CHECK(consume_diag().message == gta3sc::Diag::expected_subscript);
    }

    SUBCASE("missing closing bracket")
    {
        build_sema("VAR_INT x[10 z[10]");
        sema.pass_declarations();
        CHECK(peek_diag().message == gta3sc::Diag::expected_word);
        CHECK(consume_diag().args.at(0) == "]");
    }

    SUBCASE("empty var name")
    {
        build_sema("VAR_INT [10] z[10]", true); // parser error
        CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
    }
}


// TODO test VAR_INT (no args)

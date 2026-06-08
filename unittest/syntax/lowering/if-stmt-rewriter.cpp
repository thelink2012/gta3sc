#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/lowering/if-stmt-rewriter.hpp>
#include <gta3sc/util/name-generator.hpp>
#include <iterator>

using gta3sc::LinkedIR;
using gta3sc::SemaIR;

namespace
{
class IfStmtRewriterFixture : public gta3sc::test::CommandTableFixture
{
public:
    IfStmtRewriterFixture() :
        symtable(&arena),
        namegen("TEST_LABEL_"),
        wait_cmd(find_command("WAIT")),
        if_cmd(find_command("IF")),
        ifnot_cmd(find_command("IFNOT")),
        else_cmd(find_command("ELSE")),
        endif_cmd(find_command("ENDIF")),
        andor_cmd(find_command("ANDOR")),
        goto_cmd(find_command("GOTO")),
        goto_if_false_cmd(find_command("GOTO_IF_FALSE")),
        goto_if_true_cmd(find_command("GOTO_IF_TRUE"))
    {}

protected:
    auto make_rewriter() -> gta3sc::syntax::IfStmtRewriter
    {
        return gta3sc::syntax::IfStmtRewriter(cmdman, symtable, namegen,
                                              &arena);
    }

    auto check_andor_result(const LinkedIR<SemaIR>& result,
                            int expected_andor_count) -> void
    {
        const auto& line = *result.begin();
        CHECK(&line.command().def() == &andor_cmd);
        CHECK(*line.command().arg(0).as_int() == expected_andor_count);
    }

    auto
    insert_label(std::string_view name) -> const gta3sc::SymbolTable::Label&
    {
        const auto [label, inserted] = symtable.insert_label(
                name, gta3sc::SymbolTable::global_scope, gta3sc::no_file_range);
        REQUIRE(inserted);
        return *label;
    }

    auto find_label(std::string_view name) -> const gta3sc::SymbolTable::Label&
    {
        const auto label = symtable.lookup_label(name);
        REQUIRE(label != nullptr);
        return *label;
    }

protected:
    gta3sc::ArenaMemoryResource arena;                         // NOLINT
    gta3sc::SymbolTable symtable;                              // NOLINT
    gta3sc::util::NameGenerator namegen;                       // NOLINT
    const gta3sc::CommandTable::CommandDef& wait_cmd;          // NOLINT
    const gta3sc::CommandTable::CommandDef& if_cmd;            // NOLINT
    const gta3sc::CommandTable::CommandDef& ifnot_cmd;         // NOLINT
    const gta3sc::CommandTable::CommandDef& else_cmd;          // NOLINT
    const gta3sc::CommandTable::CommandDef& endif_cmd;         // NOLINT
    const gta3sc::CommandTable::CommandDef& andor_cmd;         // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_cmd;          // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_if_false_cmd; // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_if_true_cmd;  // NOLINT
};
} // namespace

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "no statement to rewrite")
{
    auto rewriter = make_rewriter();

    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build());

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "IF single condition no ELSE")
{
    auto rewriter = make_rewriter();

    const auto if_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build());
    REQUIRE(if_result);
    check_andor_result(*if_result, 0);

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(endif_result);
    REQUIRE(*endif_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .command(goto_if_false_cmd)
                                         .arg_label(find_label("TEST_LABEL_0"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_0"))
                                         .build()}));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "IF single condition with ELSE")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto else_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(else_cmd).build());
    REQUIRE(else_result);
    REQUIRE(*else_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .command(goto_if_false_cmd)
                                         .arg_label(find_label("TEST_LABEL_1"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .command(goto_cmd)
                                         .arg_label(find_label("TEST_LABEL_0"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_1"))
                                         .build()}));

    const auto endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(endif_result);
    REQUIRE(*endif_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_0"))
                                         .build()}));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "IFNOT single condition")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(ifnot_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(endif_result);
    REQUIRE(*endif_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .command(goto_if_true_cmd)
                                         .arg_label(find_label("TEST_LABEL_0"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_0"))
                                         .build()}));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "andor_count passes through")
{
    // if_arg to IF/ANDOR: 2 = three AND cond lines, 22 = three OR.
    const int if_arg = GENERATE(2, 22);
    CAPTURE(if_arg);

    auto rewriter = make_rewriter();

    const auto if_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(if_arg).build());
    REQUIRE(if_result);
    check_andor_result(*if_result, if_arg);

    for(int i = 0; i < 3; ++i)
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(i).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "label on IF preserved on ANDOR")
{
    auto rewriter = make_rewriter();
    const auto& foo = insert_label("FOO");

    const auto if_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                   .label(&foo)
                                                   .command(if_cmd)
                                                   .arg_int(0)
                                                   .build());
    REQUIRE(if_result);

    const auto& andor_line = *if_result->begin();
    REQUIRE(andor_line.has_label());
    CHECK(andor_line.label().name() == "FOO");
    CHECK(&andor_line.command().def() == &andor_cmd);
    CHECK(*andor_line.command().arg(0).as_int() == 0);
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "label on ELSE preserved on GOTO")
{
    auto rewriter = make_rewriter();
    const auto& foo = insert_label("FOO");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto else_result = rewriter.visit(
            *SemaIR::Builder(&arena).label(&foo).command(else_cmd).build());
    REQUIRE(else_result);

    auto it = std::next(else_result->begin());
    REQUIRE(it->has_label());
    CHECK(it->label().name() == "FOO");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "user label on ENDIF is ignored")
{
    auto rewriter = make_rewriter();
    const auto& user_label = insert_label("USER_LABEL");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto& merge_label = find_label("TEST_LABEL_0");
    const auto endif_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                      .label(&user_label)
                                                      .command(endif_cmd)
                                                      .build());
    REQUIRE(endif_result);
    REQUIRE(std::distance(endif_result->begin(), endif_result->end()) == 2);

    auto it = endif_result->begin();
    CHECK(&it->command().def() == &goto_if_false_cmd);
    CHECK(it->command().arg(0).as_label() == &merge_label);
    ++it;
    REQUIRE(it->has_label());
    CHECK(it->label().name() == "TEST_LABEL_0");
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture,
                  "goto injection during first then statement")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(10).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(11).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(12).build()));

    const auto first_then_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(20).build());
    REQUIRE(first_then_result);
    REQUIRE(std::distance(first_then_result->begin(), first_then_result->end())
            == 2);

    const auto& else_label = find_label("TEST_LABEL_1");

    auto it = first_then_result->begin();
    CHECK(&it->command().def() == &goto_if_false_cmd);
    CHECK(it->command().arg(0).as_label() == &else_label);
    ++it;
    CHECK(&it->command().def() == &wait_cmd);
    CHECK(*it->command().arg(0).as_int() == 20);
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "empty then with ELSE")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto else_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(else_cmd).build());
    REQUIRE(else_result);

    const auto& else_label = find_label("TEST_LABEL_1");
    const auto& endif_label = find_label("TEST_LABEL_0");
    auto it = else_result->begin();

    CHECK(&it->command().def() == &goto_if_false_cmd);
    CHECK(it->command().arg(0).as_label() == &else_label);
    ++it;
    CHECK(&it->command().def() == &goto_cmd);
    CHECK(it->command().arg(0).as_label() == &endif_label);
    ++it;
    CHECK(it->has_label());
    CHECK(it->label().name() == "TEST_LABEL_1");

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "three AND conditions")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(3).build()));

    const auto endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(endif_result);
    REQUIRE(std::distance(endif_result->begin(), endif_result->end()) == 2);
    CHECK(&endif_result->begin()->command().def() == &goto_if_false_cmd);
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "maximum conditions per list")
{
    SUBCASE("8 AND conditions")
    {
        auto rewriter = make_rewriter();
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(7).build()));
        for(int i = 0; i < 8; ++i)
            REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                             .command(wait_cmd)
                                             .arg_int(i)
                                             .build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build()));
    }

    SUBCASE("8 OR conditions")
    {
        auto rewriter = make_rewriter();
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(27).build()));
        for(int i = 0; i < 8; ++i)
            REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                             .command(wait_cmd)
                                             .arg_int(i)
                                             .build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build()));
    }
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "nested IF")
{
    SUBCASE("nested IF in then-block")
    {
        auto rewriter = make_rewriter();

        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

        const auto inner_if_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build());
        REQUIRE(inner_if_result);
        REQUIRE(std::distance(inner_if_result->begin(), inner_if_result->end())
                == 2);
        auto inner_if_it = inner_if_result->begin();
        CHECK(&inner_if_it->command().def() == &goto_if_false_cmd);
        ++inner_if_it;
        CHECK(&inner_if_it->command().def() == &andor_cmd);
        CHECK(*inner_if_it->command().arg(0).as_int() == 0);

        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));

        const auto inner_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(inner_endif_result);
        REQUIRE(std::distance(inner_endif_result->begin(),
                              inner_endif_result->end())
                == 2);
        auto inner_it = inner_endif_result->begin();
        CHECK(&inner_it->command().def() == &goto_if_false_cmd);
        const auto* const inner_tgt = inner_it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(inner_tgt->name()) == inner_tgt);
        ++inner_it;
        CHECK(&inner_it->label() == inner_tgt);

        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(2).build()));

        const auto outer_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(outer_endif_result);
        REQUIRE(std::distance(outer_endif_result->begin(),
                              outer_endif_result->end())
                == 2);
        auto outer_it = outer_endif_result->begin();
        CHECK(outer_it->has_label());
        ++outer_it;
        CHECK(outer_it->has_label());
        CHECK(outer_it->label().name() == "TEST_LABEL_0");
    }

    SUBCASE("nested IF directly after condition")
    {
        auto rewriter = make_rewriter();

        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

        const auto inner_if_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build());
        REQUIRE(inner_if_result);
        REQUIRE(std::distance(inner_if_result->begin(), inner_if_result->end())
                == 2);
        auto it = inner_if_result->begin();
        CHECK(&it->command().def() == &goto_if_false_cmd);
        const auto& outer_else_label = find_label("TEST_LABEL_1");
        CHECK(it->command().arg(0).as_label() == &outer_else_label);
        ++it;
        CHECK(&it->command().def() == &andor_cmd);
        CHECK(*it->command().arg(0).as_int() == 0);

        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));

        const auto inner_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(inner_endif_result);
        REQUIRE(std::distance(inner_endif_result->begin(),
                              inner_endif_result->end())
                == 2);

        const auto outer_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(outer_endif_result);
        REQUIRE(std::distance(outer_endif_result->begin(),
                              outer_endif_result->end())
                == 2);
    }

    SUBCASE("nested IF in else-block")
    {
        auto rewriter = make_rewriter();

        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(else_cmd).build()));

        const auto inner_if_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build());
        REQUIRE(inner_if_result);
        check_andor_result(*inner_if_result, 0);

        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));

        const auto inner_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(inner_endif_result);
        REQUIRE(std::distance(inner_endif_result->begin(),
                              inner_endif_result->end())
                == 2);
        auto inner_it = inner_endif_result->begin();
        CHECK(&inner_it->command().def() == &goto_if_false_cmd);
        const auto* const inner_tgt = inner_it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(inner_tgt->name()) == inner_tgt);
        ++inner_it;
        CHECK(&inner_it->label() == inner_tgt);

        const auto outer_endif_result = rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build());
        REQUIRE(outer_endif_result);
        REQUIRE(std::distance(outer_endif_result->begin(),
                              outer_endif_result->end())
                == 1);
        CHECK(outer_endif_result->begin()->has_label());
        CHECK(outer_endif_result->begin()->label().name() == "TEST_LABEL_0");
    }
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "three levels of nesting")
{
    auto rewriter = make_rewriter();

    for(int depth = 0; depth < 3; ++depth)
    {
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
        REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                         .command(wait_cmd)
                                         .arg_int(depth)
                                         .build()));
    }

    const auto e0_endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(e0_endif_result);
    REQUIRE(std::distance(e0_endif_result->begin(), e0_endif_result->end())
            == 2);
    {
        auto it = e0_endif_result->begin();
        CHECK(&it->command().def() == &goto_if_false_cmd);
        const auto* const t = it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(t->name()) == t);
        CHECK(&(++it)->label() == t);
    }

    const auto e1_endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(e1_endif_result);
    REQUIRE(std::distance(e1_endif_result->begin(), e1_endif_result->end())
            == 2);
    {
        auto it = e1_endif_result->begin();
        CHECK(it->has_label());
        ++it;
        CHECK(it->has_label());
    }

    const auto e2_endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(e2_endif_result);
    REQUIRE(std::distance(e2_endif_result->begin(), e2_endif_result->end())
            == 2);
    {
        auto it = e2_endif_result->begin();
        CHECK(it->has_label());
        ++it;
        CHECK(it->has_label());
        CHECK(it->label().name() == "TEST_LABEL_0");
    }
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "ELSE or ENDIF without preceding IF")
{
    SUBCASE("ELSE without IF")
    {
        auto rewriter = make_rewriter();
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(else_cmd).build()));
    }

    SUBCASE("ENDIF without IF")
    {
        auto rewriter = make_rewriter();
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build()));
    }

    SUBCASE("second ENDIF without matching IF")
    {
        auto rewriter = make_rewriter();

        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(endif_cmd).build()));
    }
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "successive IF blocks unique labels")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));

    rewriter = make_rewriter();
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto second_endif_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build());
    REQUIRE(second_endif_result);

    const auto& merge_label_1 = find_label("TEST_LABEL_1");
    auto it = second_endif_result->begin();
    REQUIRE(it->has_command());
    CHECK(it->command().arg(0).as_label() == &merge_label_1);
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture, "IF NOT flag is not copied to ANDOR")
{
    auto rewriter = make_rewriter();

    const auto if_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                   .command(if_cmd)
                                                   .not_flag(true)
                                                   .arg_int(0)
                                                   .build());
    REQUIRE(if_result);

    const auto& andor_node = *if_result->begin();
    CHECK(&andor_node.command().def() == &andor_cmd);
    CHECK(*andor_node.command().arg(0).as_int() == 0);
    CHECK(!andor_node.command().not_flag());

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));
}

TEST_CASE_FIXTURE(IfStmtRewriterFixture,
                  "label on first then statement preserved in rewrite")
{
    auto rewriter = make_rewriter();
    const auto& then_label = insert_label("MY_THEN_LABEL");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(if_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto then_stmt_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                          .label(&then_label)
                                                          .command(wait_cmd)
                                                          .arg_int(1)
                                                          .build());
    REQUIRE(then_stmt_result);

    auto it = then_stmt_result->begin();
    ++it;
    REQUIRE(it->has_label());
    CHECK(it->label().name() == "MY_THEN_LABEL");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endif_cmd).build()));
}

// TODO this unit test is a bit messy maybe it can be improved in the future
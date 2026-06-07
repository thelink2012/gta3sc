#include "../../command-manager-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/syntax/lowering/while-stmt-rewriter.hpp>
#include <gta3sc/util/name-generator.hpp>
#include <iterator>

using gta3sc::LinkedIR;
using gta3sc::SemaIR;

namespace
{
class WhileStmtRewriterFixture : public gta3sc::test::CommandTableFixture
{
public:
    WhileStmtRewriterFixture() :
        symtable(&arena),
        namegen("TEST_LABEL_"),
        wait_cmd(find_command("WAIT")),
        while_cmd(find_command("WHILE")),
        whilenot_cmd(find_command("WHILENOT")),
        endwhile_cmd(find_command("ENDWHILE")),
        andor_cmd(find_command("ANDOR")),
        goto_cmd(find_command("GOTO")),
        goto_if_false_cmd(find_command("GOTO_IF_FALSE")),
        goto_if_true_cmd(find_command("GOTO_IF_TRUE"))
    {}

protected:
    auto make_rewriter() -> gta3sc::syntax::WhileStmtRewriter
    {
        return gta3sc::syntax::WhileStmtRewriter(cmdman, symtable, namegen,
                                                 &arena);
    }

    auto check_andor_result(const LinkedIR<SemaIR>& result,
                            int expected_andor_count) -> void
    {
        auto it = result.begin();
        REQUIRE(it->has_label());
        CHECK(!it->has_command());
        ++it;
        CHECK(&it->command().def() == &andor_cmd);
        CHECK(*it->command().arg(0).as_int() == expected_andor_count);
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
    const gta3sc::CommandTable::CommandDef& while_cmd;         // NOLINT
    const gta3sc::CommandTable::CommandDef& whilenot_cmd;      // NOLINT
    const gta3sc::CommandTable::CommandDef& endwhile_cmd;      // NOLINT
    const gta3sc::CommandTable::CommandDef& andor_cmd;         // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_cmd;          // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_if_false_cmd; // NOLINT
    const gta3sc::CommandTable::CommandDef& goto_if_true_cmd;  // NOLINT
};
} // namespace

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "no statement to rewrite")
{
    auto rewriter = make_rewriter();

    const auto result = rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build());

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "WHILE single condition")
{
    auto rewriter = make_rewriter();

    const auto while_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build());
    REQUIRE(while_result);
    check_andor_result(*while_result, 0);

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto body_line
            = SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build();
    const auto body_result = rewriter.visit(*body_line);
    REQUIRE(body_result);
    REQUIRE(*body_result
            == LinkedIR<SemaIR>(
                    {SemaIR::Builder(&arena)
                             .command(goto_if_false_cmd)
                             .arg_label(find_label("TEST_LABEL_1"))
                             .build(),
                     SemaIR::create(nullptr, body_line->command_or_null(),
                                    &arena)}));

    const auto endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(endwhile_result);
    REQUIRE(*endwhile_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .command(goto_cmd)
                                         .arg_label(find_label("TEST_LABEL_0"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_1"))
                                         .build()}));
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "WHILENOT single condition")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(whilenot_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(endwhile_result);
    REQUIRE(*endwhile_result
            == LinkedIR<SemaIR>({SemaIR::Builder(&arena)
                                         .command(goto_if_true_cmd)
                                         .arg_label(find_label("TEST_LABEL_1"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .command(goto_cmd)
                                         .arg_label(find_label("TEST_LABEL_0"))
                                         .build(),
                                 SemaIR::Builder(&arena)
                                         .label(&find_label("TEST_LABEL_1"))
                                         .build()}));
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "andor_count passes through")
{
    const int while_arg = GENERATE(2, 22);
    CAPTURE(while_arg);

    auto rewriter = make_rewriter();

    const auto while_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                      .command(while_cmd)
                                                      .arg_int(while_arg)
                                                      .build());
    REQUIRE(while_result);
    check_andor_result(*while_result, while_arg);

    for(int i = 0; i < 3; ++i)
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(i).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(9).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "label on WHILE preserved on ANDOR")
{
    auto rewriter = make_rewriter();
    const auto& foo = insert_label("FOO");

    const auto while_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                      .label(&foo)
                                                      .command(while_cmd)
                                                      .arg_int(0)
                                                      .build());
    REQUIRE(while_result);

    const auto& andor_line = *std::next(while_result->begin());
    REQUIRE(andor_line.has_label());
    CHECK(andor_line.label().name() == "FOO");
    CHECK(&andor_line.command().def() == &andor_cmd);
    CHECK(*andor_line.command().arg(0).as_int() == 0);
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "label on ENDWHILE preserved on GOTO")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));

    const auto& foo = insert_label("FOO");
    const auto endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).label(&foo).command(endwhile_cmd).build());
    REQUIRE(endwhile_result);

    auto it = endwhile_result->begin();
    REQUIRE(it->has_label());
    CHECK(it->label().name() == "FOO");
    CHECK(&it->command().def() == &goto_cmd);
    CHECK(it->command().arg(0).as_label() == &find_label("TEST_LABEL_0"));
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "goto injection during first body statement")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(10).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(11).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(12).build()));

    const auto first_body_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(20).build());
    REQUIRE(first_body_result);
    REQUIRE(std::distance(first_body_result->begin(), first_body_result->end())
            == 2);

    const auto& endwhile_label = find_label("TEST_LABEL_1");

    auto it = first_body_result->begin();
    CHECK(&it->command().def() == &goto_if_false_cmd);
    CHECK(it->command().arg(0).as_label() == &endwhile_label);
    ++it;
    CHECK(&it->command().def() == &wait_cmd);
    CHECK(*it->command().arg(0).as_int() == 20);
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "empty body")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(endwhile_result);

    const auto& while_label = find_label("TEST_LABEL_0");
    const auto& endwhile_label = find_label("TEST_LABEL_1");
    auto it = endwhile_result->begin();

    CHECK(&it->command().def() == &goto_if_false_cmd);
    CHECK(it->command().arg(0).as_label() == &endwhile_label);
    ++it;
    CHECK(&it->command().def() == &goto_cmd);
    CHECK(it->command().arg(0).as_label() == &while_label);
    ++it;
    CHECK(it->has_label());
    CHECK(it->label().name() == "TEST_LABEL_1");
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "WHILENOT empty body")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(whilenot_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(endwhile_result);

    const auto& while_label = find_label("TEST_LABEL_0");
    const auto& endwhile_label = find_label("TEST_LABEL_1");
    auto it = endwhile_result->begin();

    CHECK(&it->command().def() == &goto_if_true_cmd);
    CHECK(it->command().arg(0).as_label() == &endwhile_label);
    ++it;
    CHECK(&it->command().def() == &goto_cmd);
    CHECK(it->command().arg(0).as_label() == &while_label);
    ++it;
    CHECK(it->has_label());
    CHECK(it->label().name() == "TEST_LABEL_1");
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "three AND conditions")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(2).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(3).build()));

    const auto body_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(9).build());
    REQUIRE(body_result);
    REQUIRE(std::distance(body_result->begin(), body_result->end()) == 2);
    CHECK(&body_result->begin()->command().def() == &goto_if_false_cmd);
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "maximum conditions per list")
{
    SUBCASE("8 AND conditions")
    {
        auto rewriter = make_rewriter();
        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(while_cmd)
                                        .arg_int(7)
                                        .build()));
        for(int i = 0; i < 8; ++i)
            REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                             .command(wait_cmd)
                                             .arg_int(i)
                                             .build()));
        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(wait_cmd)
                                        .arg_int(100)
                                        .build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
    }

    SUBCASE("8 OR conditions")
    {
        auto rewriter = make_rewriter();
        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(while_cmd)
                                        .arg_int(27)
                                        .build()));
        for(int i = 0; i < 8; ++i)
            REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                             .command(wait_cmd)
                                             .arg_int(i)
                                             .build()));
        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(wait_cmd)
                                        .arg_int(100)
                                        .build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
    }
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "nested WHILE")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto inner_while_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build());
    REQUIRE(inner_while_result);
    check_andor_result(*inner_while_result, 0);

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(2).build()));

    const auto inner_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(inner_endwhile_result);
    REQUIRE(std::distance(inner_endwhile_result->begin(),
                          inner_endwhile_result->end())
            == 2);
    auto inner_it = inner_endwhile_result->begin();
    CHECK(&inner_it->command().def() == &goto_cmd);
    const auto* const inner_back_tgt = inner_it->command().arg(0).as_label();
    CHECK(symtable.lookup_label(inner_back_tgt->name()) == inner_back_tgt);
    CHECK(inner_back_tgt == &find_label("TEST_LABEL_2"));
    ++inner_it;
    CHECK(&inner_it->label() == &find_label("TEST_LABEL_3"));

    const auto outer_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(outer_endwhile_result);
    REQUIRE(std::distance(outer_endwhile_result->begin(),
                          outer_endwhile_result->end())
            == 3);

    const auto& outer_while_label = find_label("TEST_LABEL_0");
    const auto& outer_endwhile_label = find_label("TEST_LABEL_1");
    auto outer_it = outer_endwhile_result->begin();
    CHECK(&outer_it->command().def() == &goto_if_false_cmd);
    CHECK(outer_it->command().arg(0).as_label() == &outer_endwhile_label);
    ++outer_it;
    CHECK(&outer_it->command().def() == &goto_cmd);
    CHECK(outer_it->command().arg(0).as_label() == &outer_while_label);
    ++outer_it;
    CHECK(outer_it->has_label());
    CHECK(outer_it->label().name() == "TEST_LABEL_1");
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "three levels of nesting")
{
    auto rewriter = make_rewriter();

    for(int depth = 0; depth < 3; ++depth)
    {
        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(while_cmd)
                                        .arg_int(0)
                                        .build()));
        REQUIRE(!rewriter.visit(*SemaIR::Builder(&arena)
                                         .command(wait_cmd)
                                         .arg_int(depth)
                                         .build()));
    }

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(100).build()));

    const auto e0_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(e0_endwhile_result);
    REQUIRE(std::distance(e0_endwhile_result->begin(),
                          e0_endwhile_result->end())
            == 2);
    {
        auto it = e0_endwhile_result->begin();
        CHECK(&it->command().def() == &goto_cmd);
        const auto* const t = it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(t->name()) == t);
        CHECK(&(++it)->label() == &find_label("TEST_LABEL_5"));
    }

    const auto e1_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(e1_endwhile_result);
    REQUIRE(std::distance(e1_endwhile_result->begin(),
                          e1_endwhile_result->end())
            == 3);
    {
        auto it = e1_endwhile_result->begin();
        CHECK(&it->command().def() == &goto_if_false_cmd);
        const auto* const t = it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(t->name()) == t);
        ++it;
        CHECK(&it->command().def() == &goto_cmd);
        CHECK(it->command().arg(0).as_label() == &find_label("TEST_LABEL_2"));
        ++it;
        CHECK(it->has_label());
        CHECK(it->label().name() == t->name());
    }

    const auto e2_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(e2_endwhile_result);
    REQUIRE(std::distance(e2_endwhile_result->begin(),
                          e2_endwhile_result->end())
            == 3);
    {
        auto it = e2_endwhile_result->begin();
        CHECK(&it->command().def() == &goto_if_false_cmd);
        const auto* const t = it->command().arg(0).as_label();
        CHECK(symtable.lookup_label(t->name()) == t);
        CHECK(t->name() == "TEST_LABEL_1");
        ++it;
        CHECK(&it->command().def() == &goto_cmd);
        CHECK(it->command().arg(0).as_label() == &find_label("TEST_LABEL_0"));
        ++it;
        CHECK(it->has_label());
        CHECK(it->label().name() == "TEST_LABEL_1");
    }
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture, "ENDWHILE without preceding WHILE")
{
    SUBCASE("ENDWHILE without WHILE")
    {
        auto rewriter = make_rewriter();
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
    }

    SUBCASE("second ENDWHILE without matching WHILE")
    {
        auto rewriter = make_rewriter();

        REQUIRE(rewriter.visit(*SemaIR::Builder(&arena)
                                        .command(while_cmd)
                                        .arg_int(0)
                                        .build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
        REQUIRE(rewriter.visit(
                *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
        REQUIRE(!rewriter.visit(
                *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
    }
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "successive WHILE blocks unique labels")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build()));

    rewriter = make_rewriter();
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(1).build()));

    const auto second_endwhile_result = rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build());
    REQUIRE(second_endwhile_result);

    const auto& while_label = find_label("TEST_LABEL_2");
    const auto& endwhile_label = find_label("TEST_LABEL_3");
    auto it = second_endwhile_result->begin();
    REQUIRE(it->has_command());
    CHECK(&it->command().def() == &goto_cmd);
    CHECK(it->command().arg(0).as_label()->name() == while_label.name());
    ++it;
    CHECK(it->has_label());
    CHECK(it->label().name() == endwhile_label.name());
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "WHILE NOT flag is not copied to ANDOR")
{
    auto rewriter = make_rewriter();

    const auto while_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                      .command(while_cmd)
                                                      .not_flag(true)
                                                      .arg_int(0)
                                                      .build());
    REQUIRE(while_result);

    const auto& andor_node = *std::next(while_result->begin());
    CHECK(&andor_node.command().def() == &andor_cmd);
    CHECK(*andor_node.command().arg(0).as_int() == 0);
    CHECK(!andor_node.command().not_flag());

    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));
    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "not_flag on condition command passes through")
{
    auto rewriter = make_rewriter();

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));

    const auto cond_line = SemaIR::Builder(&arena)
                                   .command(wait_cmd)
                                   .not_flag(true)
                                   .arg_int(0)
                                   .build();
    REQUIRE(!rewriter.visit(*cond_line));
    CHECK(cond_line->command().not_flag());
}

TEST_CASE_FIXTURE(WhileStmtRewriterFixture,
                  "label on first body statement preserved in rewrite")
{
    auto rewriter = make_rewriter();
    const auto& body_label = insert_label("MY_BODY_LABEL");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(while_cmd).arg_int(0).build()));
    REQUIRE(!rewriter.visit(
            *SemaIR::Builder(&arena).command(wait_cmd).arg_int(0).build()));

    const auto body_result = rewriter.visit(*SemaIR::Builder(&arena)
                                                     .label(&body_label)
                                                     .command(wait_cmd)
                                                     .arg_int(1)
                                                     .build());
    REQUIRE(body_result);

    auto it = body_result->begin();
    ++it;
    REQUIRE(it->has_label());
    CHECK(it->label().name() == "MY_BODY_LABEL");

    REQUIRE(rewriter.visit(
            *SemaIR::Builder(&arena).command(endwhile_cmd).build()));
}

#include "syntax-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/syntax/parser.hpp>
#include <string>
using namespace gta3sc::test::syntax; // NOLINT
using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;
using gta3sc::test::syntax::d;

// TODO test parse_main_extension_file
// TODO test parse_subscript_file
// TODO test parse_mission_script_file

namespace gta3sc::test::syntax
{
class ParserFixture : public SyntaxFixture
{
public:
    ParserFixture() : parser(make_parser(make_source(""), diagman, arena)) {}

protected:
    void build_parser(std::string_view src)
    {
        this->parser = make_parser(make_source(src), diagman, arena);
    }

private:
    static auto make_parser(gta3sc::SourceFile source,
                            gta3sc::DiagnosticHandler& diagman,
                            gta3sc::ArenaMemoryResource& arena)
            -> gta3sc::syntax::Parser
    {
        auto pp = gta3sc::syntax::Preprocessor(std::move(source), diagman);
        auto scanner = gta3sc::syntax::Scanner(std::move(pp));
        return gta3sc::syntax::Parser(std::move(scanner), arena);
    }

    gta3sc::ArenaMemoryResource arena;

protected:
    gta3sc::syntax::Parser parser; // NOLINT
};
} // namespace gta3sc::test::syntax

namespace
{
auto size(const gta3sc::LinkedIR<gta3sc::ParserIR>& ir)
{
    return std::distance(ir.begin(), ir.end());
}
}

TEST_CASE_FIXTURE(ParserFixture, "parsing an empty main script file")
{
    build_parser("");
    auto ir = parser.parse_main_script_file();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->empty());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a label definition")
{
    build_parser("laBEL:\n"
                 "laBEL: WAIT 0\n"
                 "label:\n"
                 "WAIT 0\n"
                 "la:bel:\n"
                 "1abel:\n"
                 "lab\"el\":\n"
                 "\"label\":\n"
                 "lab\"el:\n"
                 ":\n"
                 "::\n"
                 "label:");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "LABEL");
    REQUIRE(ir->front().command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "LA:BEL");

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1abel:
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_identifier);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el":
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_token);
    CHECK(consume_diag().args.at(0) == d(gta3sc::syntax::Category::Whitespace));

    ir = parser.parse_statement();
    parser.skip_current_line(); // "label":
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_command);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el:
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::unterminated_string_literal);

    ir = parser.parse_statement();
    parser.skip_current_line(); // :
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_identifier);

    ir = parser.parse_statement();
    parser.skip_current_line(); // ::
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_identifier);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "LABEL");
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a empty line")
{
    build_parser("\n"
                 "WAIT 0\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->empty());

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid scope block")
{
    build_parser("{\n"
                 "WAIT 0\n"
                 "WAIT 1\n"
                 "}\n"
                 "WAIT 2\n"
                 "{\n"
                 "}\n"
                 "WAIT 3\n");

    auto linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);

    auto ir = linked->begin();
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->args.size() == 0);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->args.size() == 0);
    REQUIRE(++ir == linked->end());

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();
    ;
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->args.size() == 0);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->args.size() == 0);
    REQUIRE(++ir == linked->end());

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a nested scope block")
{
    build_parser("{\n"
                 "{\n"
                 "}\n"
                 "}\n");

    auto linked = parser.parse_statement();
    REQUIRE(linked == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::cannot_nest_scopes);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a } outside a scope block")
{
    build_parser("}\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("}"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a unclosed scope block")
{
    build_parser("{\n"
                 "\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_word);
    CHECK(consume_diag().args.at(0) == d("}"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a command")
{
    build_parser("waIT 10 20 30\n"
                 "C\n"
                 "c\n"
                 "l: c:\n"
                 "a.sc\n"
                 "\"a\"\n"
                 "%\n"
                 "$\n"
                 "1\n"
                 ".1\n"
                 "-1\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 3);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "C");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "C");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().label->name == "L");
    REQUIRE(ir->front().command->name == "C:");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "A.SC");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a"
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_command);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "%");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "$");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "1");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == ".1");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "-1");
    REQUIRE(ir->front().command->args.size() == 0);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing integer argument")
{
    build_parser("WAIT 123 010 -39\n"
                 "WAIT 2147483647 -2147483648\n"
                 "WAIT -432-10\n"
                 "WAIT 123a\n"
                 "WAIT 0x10\n"
                 "WAIT +39\n"
                 "WAIT 432+10\n"
                 "WAIT x -\n"
                 "WAIT x --\n"
                 "WAIT 9");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 3);
    REQUIRE(*ir->front().command->args[0]->as_integer() == 123);
    REQUIRE(*ir->front().command->args[1]->as_integer() == 10);
    REQUIRE(*ir->front().command->args[2]->as_integer() == -39);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 2);
    REQUIRE(*ir->front().command->args[0]->as_integer()
            == std::numeric_limits<int32_t>::max());
    REQUIRE(*ir->front().command->args[1]->as_integer()
            == std::numeric_limits<int32_t>::min());

    ir = parser.parse_statement();
    parser.skip_current_line(); // -432-10
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_token);
    CHECK(consume_diag().args.at(0) == d(gta3sc::syntax::Category::Whitespace));

    ir = parser.parse_statement();
    parser.skip_current_line(); // 123a
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 0x10
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // +39
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 432+10
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_token);
    CHECK(consume_diag().args.at(0) == d(gta3sc::syntax::Category::Whitespace));

    ir = parser.parse_statement();
    parser.skip_current_line(); // -
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // --
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE_FIXTURE(ParserFixture, "parsing float argument")
{
    build_parser("WAIT .1 -.1 .1f .1F .15 .1.9 -.1.\n"
                 "WAIT 1F -1f 1. 1.1 1.f 1.. -1..\n"
                 "WAIT .1a\n"
                 "WAIT .1fa\n"
                 "WAIT .1.a\n"
                 "WAIT 1..a\n"
                 "WAIT .1-.1\n"
                 "WAIT 9");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 7);
    REQUIRE(*ir->front().command->args[0]->as_float() == 0.1F);
    REQUIRE(*ir->front().command->args[1]->as_float() == -0.1F);
    REQUIRE(*ir->front().command->args[2]->as_float() == 0.1F);
    REQUIRE(*ir->front().command->args[3]->as_float() == 0.1F);
    REQUIRE(*ir->front().command->args[4]->as_float() == 0.15F);
    REQUIRE(*ir->front().command->args[5]->as_float() == 0.1F);
    REQUIRE(*ir->front().command->args[6]->as_float() == -0.1F);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 7);
    REQUIRE(*ir->front().command->args[0]->as_float() == 1.0F);
    REQUIRE(*ir->front().command->args[1]->as_float() == -1.0F);
    REQUIRE(*ir->front().command->args[2]->as_float() == 1.0F);
    REQUIRE(*ir->front().command->args[3]->as_float() == 1.1F);
    REQUIRE(*ir->front().command->args[4]->as_float() == 1.0F);
    REQUIRE(*ir->front().command->args[5]->as_float() == 1.0F);
    REQUIRE(*ir->front().command->args[6]->as_float() == -1.0F);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1a
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1fa
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); //.1.a
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1..a
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1-.1
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_token);
    CHECK(consume_diag().args.at(0) == d(gta3sc::syntax::Category::Whitespace));

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE_FIXTURE(ParserFixture, "parsing identifier argument")
{
    build_parser("WAIT $abc abc AbC a@_1$\n"
                 "WAIT _abc\n"
                 "WAIT @abc\n"
                 "WAIT 1abc\n"
                 "WAIT abc: def\n"
                 "WAIT 9");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 4);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "$ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[2]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[3]->as_identifier(), "A@_1$"sv);

    ir = parser.parse_statement();
    parser.skip_current_line(); // _abc
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // @abc
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1abc
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    parser.skip_current_line(); // abc: def
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE_FIXTURE(ParserFixture, "parsing string literal argument")
{
    build_parser("WAIT \"this\tI$ /* a // \\n (%1teral),\"\n"
                 "WAIT \"\"\n"
                 "WAIT \"\n"
                 "WAIT \"string\"abc\n"
                 "WAIT 9");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_string(),
               "this\tI$ /* a // \\n (%1teral),"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_string(), ""sv);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::unterminated_string_literal);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "string"abc
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_token);
    CHECK(consume_diag().args.at(0) == d(gta3sc::syntax::Category::Whitespace));

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE_FIXTURE(ParserFixture, "parsing filename argument")
{
    build_parser("LAUNCH_MISSION .sc\n"
                 "LAUNCH_MISSION a.SC\n"
                 "WAIT a.SC\n"
                 "WAIT 1.SC\n"
                 "LAUNCH_MISSION @.sc\n"
                 "LAUNCH_MISSION 1.sc\n"
                 "LAUNCH_MISSION 1.0sc\n"
                 "LAUNCH_MISSION SC\n"
                 "LAUNCH_MISSION C\n"
                 "LAUNCH_MISSION \"a\".sc\n"
                 "LOAD_AND_LAUNCH_MISSION file-name.sc\n"
                 "GOSUB_FILE label file-name.sc\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), ".SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "A.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // WAIT a.SC

    ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt); // WAIT 1.sc
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "@.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "1.SC"sv);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1.0sc
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);

    ir = parser.parse_statement();
    parser.skip_current_line(); // SC
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);

    ir = parser.parse_statement();
    parser.skip_current_line(); // C
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a".sc
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // LOAD_AND_LAUNCH_MISSION
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "FILE-NAME.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // GOSUB_FILE
    REQUIRE(ir->front().command->args.size() == 2);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "LABEL"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_filename(), "FILE-NAME.SC"sv);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing permutations of absolute expressions")
{
    build_parser("x = aBs y\n"
                 "x = ABS x\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "SET");
    REQUIRE(it->command->args.size() == 2);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
    REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Y"sv);
    ++it;
    REQUIRE(it->command->name == "ABS");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
    REQUIRE(++it == ir->end());

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "ABS");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
    REQUIRE(size(*ir) == 1);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing permutations of unary expressions")
{
    build_parser("++x\n"
                 "x++\n"
                 "--x\n"
                 "x--\n");

    constexpr std::string_view expects_table[] = {
            "ADD_THING_TO_THING",
            "ADD_THING_TO_THING",
            "SUB_THING_FROM_THING",
            "SUB_THING_FROM_THING",
    };

    for(const auto& command_name : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_integer(), 1);
        REQUIRE(size(*ir) == 1);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "parsing permutations of binary expressions")
{
    build_parser("x = y\n"
                 "x = x\n"
                 "x =# y\n"
                 "x =# x\n"
                 "x += y\n"
                 "x += x\n"
                 "x -= y\n"
                 "x -= x\n"
                 "x *= y\n"
                 "x *= x\n"
                 "x /= y\n"
                 "x /= x\n"
                 "x +=@ y\n"
                 "x +=@ x\n"
                 "x -=@ y\n"
                 "x -=@ x\n");

    constexpr std::string_view expects_table[] = {
            "SET",
            "CSET",
            "ADD_THING_TO_THING",
            "SUB_THING_FROM_THING",
            "MULT_THING_BY_THING",
            "DIV_THING_BY_THING",
            "ADD_THING_TO_THING_TIMED",
            "SUB_THING_FROM_THING_TIMED",
    };

    for(const auto& command_name : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
        REQUIRE(size(*ir) == 1);

        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "X"sv);
        REQUIRE(size(*ir) == 1);
    }
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing permutations of conditional expressions")
{
    build_parser("IF x = y GOTO elsewhere\n"
                 "IFNOT x = x GOTO elsewhere\n"
                 "x < y\n"
                 "x < x\n"
                 "x <= y\n"
                 "x > y\n"
                 "x >= y\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 3);
    auto it = ir->begin();
    REQUIRE(std::next(it)->command->name == "IS_THING_EQUAL_TO_THING");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 3);
    it = ir->begin();
    REQUIRE(std::next(it)->command->name == "IS_THING_EQUAL_TO_THING");

    constexpr std::array<std::string_view, 3> expects_table[] = {
            {"IS_THING_GREATER_THAN_THING", "Y", "X"},
            {"IS_THING_GREATER_THAN_THING", "X", "X"},
            {"IS_THING_GREATER_OR_EQUAL_TO_THING", "Y", "X"},
            {"IS_THING_GREATER_THAN_THING", "X", "Y"},
            {"IS_THING_GREATER_OR_EQUAL_TO_THING", "X", "Y"},
    };

    for(const auto& [command_name, a, b] : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), a);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), b);
        REQUIRE(size(*ir) == 1);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "parsing permutations of ternary expressions")
{
    build_parser("x = x + x\n"
                 "x = x + y\n"
                 "x = y + x\n"
                 "x = y + z\n"
                 "x = x - x\n"
                 "x = x - y\n"
                 "x = y - x\n" // invalid
                 "x = y - z\n"
                 "x = x * x\n"
                 "x = x * y\n"
                 "x = y * x\n"
                 "x = y * z\n"
                 "x = x / x\n"
                 "x = x / y\n"
                 "x = y / x\n" // invalid
                 "x = y / z\n"
                 "x = x +@ x\n"
                 "x = x +@ y\n"
                 "x = y +@ x\n" // invalid
                 "x = y +@ z\n"
                 "x = x -@ x\n"
                 "x = x -@ y\n"
                 "x = y -@ x\n" // invalid
                 "x = y -@ z\n");

    constexpr std::string_view expects_table[] = {
            "ADD_THING_TO_THING",       "SUB_THING_FROM_THING",
            "MULT_THING_BY_THING",      "DIV_THING_BY_THING",
            "ADD_THING_TO_THING_TIMED", "SUB_THING_FROM_THING_TIMED",
    };

    for(const auto& command_name : expects_table)
    {
        // x = x + x
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "X"sv);
        REQUIRE(size(*ir) == 1);

        // x = x + y
        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
        REQUIRE(size(*ir) == 1);

        // x = y + x
        if(command_name == "ADD_THING_TO_THING"
           || command_name == "MULT_THING_BY_THING")
        {
            ir = parser.parse_statement();
            REQUIRE(ir != std::nullopt);
            REQUIRE(ir->front().command->name == command_name);
            REQUIRE(ir->front().command->args.size() == 2);
            REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
            REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
            REQUIRE(size(*ir) == 1);
        }
        else
        {
            ir = parser.parse_statement();
            parser.skip_current_line();
            REQUIRE(ir == std::nullopt);
            CHECK(peek_diag().message
                  == gta3sc::Diag::invalid_expression_unassociative);
            if(command_name == "SUB_THING_FROM_THING")
                CHECK(consume_diag().args.at(0)
                      == d(gta3sc::syntax::Category::Minus));
            else if(command_name == "DIV_THING_BY_THING")
                CHECK(consume_diag().args.at(0)
                      == d(gta3sc::syntax::Category::Slash));
            else if(command_name == "ADD_THING_TO_THING_TIMED")
                CHECK(consume_diag().args.at(0)
                      == d(gta3sc::syntax::Category::PlusAt));
            else if(command_name == "SUB_THING_FROM_THING_TIMED")
                CHECK(consume_diag().args.at(0)
                      == d(gta3sc::syntax::Category::MinusAt));
            else
                CHECK(false);
        }

        // x = y + z
        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        auto it = ir->begin();
        REQUIRE(it->command->name == "SET");
        REQUIRE(it->command->args.size() == 2);
        REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Y"sv);
        REQUIRE((++it)->command->name == command_name);
        REQUIRE(it->command->args.size() == 2);
        REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Z"sv);
        REQUIRE(++it == ir->end());
    }
}

TEST_CASE_FIXTURE(ParserFixture, "parsing the ternary minus one ambiguity")
{
    build_parser("x = 1-1\n"
                 "x = 1 -1\n"
                 "x = 1 - 1\n"
                 "x = 1--1\n"
                 "x = 1- -1\n");

    auto ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_expression);

    ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::invalid_expression);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_ternary_operator);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing operators not in expression")
{
    build_parser("+= 1\n"
                 "x / 2\n");

    auto ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_command);

    ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing invalid expressions")
{
    build_parser("--x c\n"
                 "x++ c\n"
                 "x = ABS y z\n"
                 "x = y +\n"
                 "x = + y\n"
                 "x = y + z + w\n"
                 "x = y z\n"
                 "x += y + z\n"
                 "x =#\n"
                 "x < y + z\n"
                 "x <\n"
                 "x + y\n"
                 "x = y += z\n");

    for(size_t count = 0;; ++count)
    {
        auto ir = parser.parse_statement();
        if(ir != std::nullopt && ir->empty())
        {
            REQUIRE(count == 13);
            break;
        }
        parser.skip_current_line();
        REQUIRE(ir == std::nullopt);
        if(count == 11)
            CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
        else if(count == 12)
            CHECK(consume_diag().message
                  == gta3sc::Diag::expected_ternary_operator);
        else
            CHECK(consume_diag().message == gta3sc::Diag::invalid_expression);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "parsing expressions with no whitespaces")
{
    build_parser("-- x\n"
                 "x ++\n"
                 "x=ABS y\n"
                 "x=y+z\n"
                 "x+=y\n"
                 "x<y\n"
                 "x<=y\n");

    for(size_t count = 0;; ++count)
    {
        auto ir = parser.parse_statement();
        if(ir != std::nullopt && ir->empty())
        {
            REQUIRE(count == 7);
            break;
        }
        REQUIRE(ir != std::nullopt);
    }
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing commands with operators in the middle")
{
    build_parser("COMMAND x - y\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing special words in expressions")
{
    build_parser( // The following is invalid:
            "GOSUB_FILE++\n"
            "GOSUB_FILE ++\n"
            "LAUNCH_MISSION ++\n"
            "GOSUB_FILE = OTHER\n"
            "LOAD_AND_LAUNCH_MISSION = OTHER\n"
            "MISSION_START = OTHER\n"
            "MISSION_END = OTHER\n"
            "MISSION_START ++\n"
            "MISSION_END ++\n"
            // The following is valid:
            "++GOSUB_FILE\n"
            "++ GOSUB_FILE\n"
            "++ MISSION_START\n"
            "++MISSION_START\n"
            "++MISSION_END\n"
            "OTHER = GOSUB_FILE\n"
            "VAR_INT = LVAR_INT\n"
            "WHILE = ENDWHILE\n"
            "ENDIF = IF\n"
            "ELSE = ENDIF\n"
            "IFNOT = IFNOT\n"
            "REPEAT = ENDREPEAT\n"
            "ABS = ABS ABS\n");

    for(int invalid = 9; invalid > 0; --invalid)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        parser.skip_current_line();

        switch(invalid)
        {
            case 9: // GOSUB_FILE++
                CHECK(consume_diag().message == gta3sc::Diag::expected_token);
                break;
            case 8: // GOSUB_FILE ++
                CHECK(consume_diag().message
                      == gta3sc::Diag::expected_argument);
                break;
            case 7: // LAUNCH_MISSION ++
                CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);
                break;
            case 6: // GOSUB_FILE = OTHER
                CHECK(consume_diag().message
                      == gta3sc::Diag::expected_argument);
                break;
            case 5: // LOAD_AND_LAUNCH_MISSION = OTHER
                CHECK(consume_diag().message == gta3sc::Diag::invalid_filename);
                break;
            case 4: // MISSION_START = OTHER
            case 3: // MISSION_END = OTHER
            case 2: // MISSION_START ++
            case 1: // MISSION_END ++
                CHECK(consume_diag().message
                      == gta3sc::Diag::unexpected_special_name);
                break;
            default:
                assert(false);
                break;
        }
    }

    for(int valid = 13; valid > 0; --valid)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->empty());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid IF...GOTO statement")
{
    build_parser("IF SOMETHING GOTO elsewhere\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "GOTO_IF_TRUE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "ELSEWHERE"sv);
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid IFNOT...GOTO statement")
{
    build_parser("IFNOT SOMETHING GOTO elsewhere\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "GOTO_IF_FALSE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "ELSEWHERE"sv);
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing a valid conditional element with equal operator")
{
    build_parser("IF x = y GOTO elsewhere\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE((++it)->command->name == "GOTO_IF_TRUE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE(*it->command->args[0]->as_identifier() == "ELSEWHERE"sv);
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing a ternary expression with a GOTO following it")
{
    build_parser("IF x = y + z GOTO elsewhere\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message
          == gta3sc::Diag::expected_conditional_expression);
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing a conditional element with assignment expression")
{
    build_parser("IF x += y GOTO elsewhere\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message
          == gta3sc::Diag::expected_conditional_operator);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid IF...ENDIF block")
{
    build_parser("IF SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid IF...ELSE...ENDIF block")
{
    build_parser("IF SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ELSE\n"
                 "    DO_3\n"
                 "    DO_4\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ELSE");
    REQUIRE((++it)->command->name == "DO_3");
    REQUIRE((++it)->command->name == "DO_4");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid IFNOT...ENDIF block")
{
    build_parser("IFNOT SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IFNOT");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid NOT")
{
    build_parser("IF NOT SOMETHING\n"
                 "OR NOT OTHER_THING\n"
                 "OR ANOTHER_THING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 22);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a IF without ENDIF")
{
    build_parser("IF SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_words);
    CHECK(consume_diag().args.at(0) == d(std::vector{"ELSE"s, "ENDIF"s}));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a IF...ELSE without ENDIF")
{
    build_parser("IF SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ELSE\n"
                 "    DO_3\n"
                 "    DO_4\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_word);
    CHECK(consume_diag().args.at(0) == d("ENDIF"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a ELSE/ENDIF with no IF")
{
    build_parser("ENDIF\n"
                 "ELSE\n");

    auto ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("ENDIF"));

    ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("ELSE"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a conditionless IF")
{
    build_parser("IF \n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_command);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid AND list")
{
    build_parser("IF SOMETHING\n"
                 "AND OTHER_THING\n"
                 "AND ANOTHER_THING\n"
                 "AND THING_4\n"
                 "AND THING_5\n"
                 "AND THING_6\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 5);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE((++it)->command->name == "THING_4");
    REQUIRE((++it)->command->name == "THING_5");
    REQUIRE((++it)->command->name == "THING_6");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid OR list")
{
    build_parser("IF SOMETHING\n"
                 "OR OTHER_THING\n"
                 "OR ANOTHER_THING\n"
                 "OR THING_4\n"
                 "OR THING_5\n"
                 "OR THING_6\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 25);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE((++it)->command->name == "THING_4");
    REQUIRE((++it)->command->name == "THING_5");
    REQUIRE((++it)->command->name == "THING_6");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing AND/OR/NOT outside of condition")
{
    build_parser("AND SOMETHING\n"
                 "OR OTHER_THING\n"
                 "NOT AAAA\n");

    auto ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("AND"));

    ir = parser.parse_statement();
    parser.skip_current_line();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("OR"));

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("NOT"));
}
TEST_CASE_FIXTURE(ParserFixture, "parsing mixed AND/OR")
{
    build_parser("IF SOMETHING\n"
                 "OR OTHER_THING\n"
                 "AND ANOTHER_THING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::cannot_mix_andor);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing too many AND/OR")
{
    build_parser("IF SOMETHING\n"
                 "OR OTHER_THING\n"
                 "OR ANOTHER_THING\n"
                 "OR THING_4\n"
                 "OR THING_5\n"
                 "OR THING_6\n"
                 "OR THING_7\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::too_many_conditions);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a conditionless AND/OR")
{
    build_parser("IF SOMETHING\n"
                 "OR \n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_command);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid WHILE...ENDWHILE")
{
    build_parser("WHILE SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a valid WHILENOT...ENDWHILE")
{
    build_parser("WHILENOT SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILENOT");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing a valid WHILE...ENDWHILE with AND/OR/NOT")
{
    build_parser("WHILE SOMETHING\n"
                 "AND OTHER_THING\n"
                 "AND NOT ANOTHER_THING\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(it->command->not_flag == false);
    REQUIRE(*it->command->args[0]->as_integer() == 2);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(it->command->not_flag == false);
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a WHILE without ENDWHILE")
{
    build_parser("WHILE SOMETHING\n"
                 "    DO_1\n"
                 "    DO_2\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_word);
    CHECK(consume_diag().args.at(0) == d("ENDWHILE"));
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing nested blocks with empty statement list")
{
    build_parser("WHILE THING_1\n"
                 "    WHILE THING_2\n"
                 "    ENDWHILE\n"
                 "ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE((++it)->command->name == "THING_1");
    REQUIRE((++it)->command->name == "WHILE");
    REQUIRE((++it)->command->name == "THING_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing valid REPEAT...ENDREPEAT")
{
    build_parser("REPEAT 5 var\n"
                 "    DO_1\n"
                 "    DO_2\n"
                 "ENDREPEAT\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "REPEAT");
    REQUIRE(it->command->args.size() == 2);
    REQUIRE(*it->command->args[0]->as_integer() == 5);
    REQUIRE_EQ(*it->command->args[1]->as_identifier(), "VAR"sv);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDREPEAT");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing a REPEAT without ENDREPEAT")
{
    build_parser("REPEAT 5 var\n"
                 "    DO_1\n"
                 "    DO_2\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::expected_word);
    CHECK(consume_diag().args.at(0) == d("ENDREPEAT"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing valid var declaration commands")
{
    build_parser("VAR_INT x y z\n"
                 "LVAR_INT x y z\n"
                 "VAR_FLOAT x y z\n"
                 "LVAR_FLOAT x y z\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 1);
    REQUIRE(ir->front().command->name == "LVAR_FLOAT");
    REQUIRE(ir->front().command->args.size() == 3);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
    REQUIRE_EQ(*ir->front().command->args[2]->as_identifier(), "Z"sv);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing invalid use of special names")
{
    build_parser("MISSION_END\n"
                 "MISSION_START\n"
                 "}\n"
                 "NOT\n"
                 "AND\n"
                 "OR\n"
                 "ELSE\n"
                 "ENDIF\n"
                 "ENDWHILE\n"
                 "ENDREPEAT\n"
                 "IF {\n"
                 "IF NOT NOT\n"
                 "IF AND\n"
                 "IF IF 0\n"
                 "IF IFNOT 0\n"
                 "IF WHILE 0\n"
                 "IF REPEAT 4 x\n"
                 "IF GOSUB_FILE a b.sc\n"
                 "IF LAUNCH_MISSION b.sc\n"
                 "IF LOAD_AND_LAUNCH_MISSION b.sc\n"
                 "IF MISSION_START\n"
                 "IF MISSION_END\n"
                 "WAIT 0\n"); // valid sync point

    for(auto invalid = 0; invalid < 22; ++invalid)
    {
        auto ir = parser.parse_statement();
        parser.skip_current_line();
        REQUIRE(ir == std::nullopt);
        CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
        CHECK(consume_diag().args.size() == 1);
    }

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
}

TEST_CASE_FIXTURE(ParserFixture,
                  "parsing var decl while trying to match a special name")
{
    build_parser("WHILE x = 0\n"
                 "VAR_INT y\n"
                 "ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing weird closing blocks")
{
    build_parser("WHILE x = 0\n"
                 "    IF y = 0\n"
                 "        WAIT 0\n"
                 "ENDWHILE\n"
                 "    ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(peek_diag().message == gta3sc::Diag::unexpected_special_name);
    CHECK(consume_diag().args.at(0) == d("ENDWHILE"));
}

TEST_CASE_FIXTURE(ParserFixture, "parsing labels in AND/OR")
{
    build_parser("IF x = 0\n"
                 "label: AND y = 0\n"
                 "    WAIT 0\n"
                 "ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    CHECK(consume_diag().message == gta3sc::Diag::expected_argument);
}

TEST_CASE_FIXTURE(ParserFixture, "parsing labels in }")
{
    build_parser("{\n"
                 "WAIT 0\n"
                 "label: }\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 3);

    auto it = ir->begin();
    REQUIRE(it->command->name == "{");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "}");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing labels in ELSE/ENDIF")
{
    build_parser("IF x = 0\n"
                 "    WAIT 0\n"
                 "lab1: ELSE\n"
                 "    WAIT 1\n"
                 "lab2: ENDIF\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 6);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ELSE");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LAB1");
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LAB2");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing labels in ENDWHILE")
{
    build_parser("WHILE x = 0\n"
                 "    WAIT 0\n"
                 "label: ENDWHILE\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 4);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "parsing labels in ENDREPEAT")
{
    build_parser("REPEAT 2 x\n"
                 "    WAIT 0\n"
                 "label: ENDREPEAT\n");

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(size(*ir) == 3);

    auto it = ir->begin();
    REQUIRE(it->command->name == "REPEAT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDREPEAT");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of ENDWHILE")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("WHILE SOMETHING\nENDWHILE\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);
        REQUIRE(ir->back().command != nullptr);
        REQUIRE(ir->back().command->args.size() == 0);
    }

    SUBCASE("too many arguments")
    {
        build_parser("WHILE SOMETHING\nENDWHILE 1\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of REPEAT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("REPEAT 10 a\nENDREPEAT\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 2);
    }

    SUBCASE("too few arguments")
    {
        build_parser("REPEAT 10\nENDREPEAT\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }

    SUBCASE("too many arguments")
    {
        build_parser("REPEAT 10 a b\nENDREPEAT\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of ENDREPEAT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("REPEAT 10 a\nENDREPEAT\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);
        REQUIRE(ir->back().command != nullptr);
        REQUIRE(ir->back().command->args.size() == 0);
    }

    SUBCASE("too many arguments")
    {
        build_parser("REPEAT 10 a\nENDREPEAT 10\n");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of ELSE")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("IF SOMETHING\nELSE\nENDIF");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 4);
        REQUIRE(std::next(ir->begin(), 2)->command != nullptr);
        REQUIRE(std::next(ir->begin(), 2)->command->args.size() == 0);
    }

    SUBCASE("too many arguments")
    {
        build_parser("IF SOMETHING\nELSE xyz\nENDIF");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of ENDIF")
{
    SUBCASE("correct number of arguments (without ELSE)")
    {
        build_parser("IF SOMETHING\nENDIF");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 3);
        REQUIRE(ir->back().command != nullptr);
        REQUIRE(ir->back().command->args.size() == 0);
    }

    SUBCASE("too many arguments (without ELSE)")
    {
        build_parser("IF SOMETHING\nENDIF xyz");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }

    SUBCASE("correct number of arguments (with ELSE)")
    {
        build_parser("IF SOMETHING\nELSE\nENDIF");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 4);
        REQUIRE(ir->back().command != nullptr);
        REQUIRE(ir->back().command->args.size() == 0);
    }

    SUBCASE("too many arguments (with ELSE)")
    {
        build_parser("IF SOMETHING\nELSE\nENDIF xyz");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of MISSION_START")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("MISSION_START\nMISSION_END");
        auto ir = parser.parse_subscript_file();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 0);
    }

    SUBCASE("too many arguments")
    {
        build_parser("MISSION_START a\nMISSION_END");
        auto ir = parser.parse_subscript_file();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of MISSION_END")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("MISSION_START\nMISSION_END");
        auto ir = parser.parse_subscript_file();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 2);
        REQUIRE(ir->back().command != nullptr);
        REQUIRE(ir->back().command->args.size() == 0);
    }

    SUBCASE("too many arguments")
    {
        build_parser("MISSION_START a\nMISSION_END a");
        auto ir = parser.parse_subscript_file();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_many_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of GOSUB_FILE")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("GOSUB_FILE a a.sc");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 2);
    }

    SUBCASE("too few arguments")
    {
        build_parser("GOSUB_FILE a ");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_identifier);
    }

    SUBCASE("too many arguments")
    {
        build_parser("GOSUB_FILE a a.sc b");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_token);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of LAUNCH_MISSION")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("LAUNCH_MISSION a.sc");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 1);
    }

    SUBCASE("too few arguments")
    {
        build_parser("LAUNCH_MISSION");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_identifier);
    }

    SUBCASE("too many arguments")
    {
        build_parser("LAUNCH_MISSION a.sc b");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_token);
    }
}

TEST_CASE_FIXTURE(ParserFixture,
                  "number of arguments of LOAD_AND_LAUNCH_MISSION")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("LOAD_AND_LAUNCH_MISSION a.sc");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(size(*ir) == 1);
        REQUIRE(ir->front().command != nullptr);
        REQUIRE(ir->front().command->args.size() == 1);
    }

    SUBCASE("too few arguments")
    {
        build_parser("LOAD_AND_LAUNCH_MISSION");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_identifier);
    }

    SUBCASE("too many arguments")
    {
        build_parser("LOAD_AND_LAUNCH_MISSION a.sc b");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::expected_token);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of VAR_INT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("VAR_INT a\nVAR_INT b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("VAR_INT");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of LVAR_INT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("LVAR_INT a\nLVAR_INT b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("LVAR_INT");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of VAR_FLOAT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("VAR_FLOAT a\nVAR_FLOAT b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("VAR_FLOAT");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of LVAR_FLOAT")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("LVAR_FLOAT a\nLVAR_FLOAT b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("LVAR_FLOAT");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of VAR_TEXT_LABEL")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("VAR_TEXT_LABEL a\nVAR_TEXT_LABEL b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("VAR_TEXT_LABEL");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

TEST_CASE_FIXTURE(ParserFixture, "number of arguments of LVAR_TEXT_LABEL")
{
    SUBCASE("correct number of arguments")
    {
        build_parser("LVAR_TEXT_LABEL a\nLVAR_TEXT_LABEL b c");
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    SUBCASE("too few arguments")
    {
        build_parser("LVAR_TEXT_LABEL");
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        REQUIRE(consume_diag().message == gta3sc::Diag::too_few_arguments);
    }
}

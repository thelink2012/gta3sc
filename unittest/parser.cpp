#include <doctest.h>
#include <gta3sc/parser.hpp>
#include <cstring>
using namespace std::literals::string_view_literals;

namespace
{
template<std::size_t N>
auto make_source(const char (&data)[N]) -> gta3sc::SourceFile
{
    auto ptr = std::make_unique<char[]>(N);
    std::memcpy(ptr.get(), data, N);
    return gta3sc::SourceFile(std::move(ptr), N-1);
}

auto make_parser(const gta3sc::SourceFile& source, gta3sc::ArenaMemoryResource& arena)
    -> gta3sc::Parser
{
    auto pp = gta3sc::Preprocessor(source);
    auto scanner = gta3sc::Scanner(std::move(pp));
    return gta3sc::Parser(std::move(scanner), arena);
}
}

TEST_CASE("parsing a label definition")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("laBEL:\n"
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
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "LABEL");
    REQUIRE(ir->front()->command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "LA:BEL");

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1abel:
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el":
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "label":
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el:
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // :
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // ::
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "LABEL");
}

TEST_CASE("parsing a valid scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "WAIT 0\n"
                              "WAIT 1\n"
                              "}\n"
                              "WAIT 2\n"
                              "{\n"
                              "}\n"
                              "WAIT 3\n");
    auto parser = make_parser(source, arena);

    auto linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);

    auto ir = linked->front();
    REQUIRE(ir != nullptr);
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->num_args == 0);

    ir = ir->next;
    REQUIRE(ir != nullptr);
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->num_args == 1);

    ir = ir->next;
    REQUIRE(ir != nullptr);
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->num_args == 1);

    ir = ir->next;
    REQUIRE(ir != nullptr);
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->num_args == 0);
    REQUIRE(ir->next == nullptr);

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->num_args == 1);

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->num_args == 0);

    ir = ir->next;
    REQUIRE(ir != nullptr);
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->num_args == 0);
    REQUIRE(ir->next == nullptr);

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->num_args == 1);
}

TEST_CASE("parsing a nested scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "{\n"
                              "}\n"
                              "}\n");
    auto parser = make_parser(source, arena);

    auto linked = parser.parse_statement();
    REQUIRE(linked == std::nullopt);
}

TEST_CASE("parsing a scope block with bad argument count")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{ 1 2\n"
                              "}\n"
                              "{\n"
                              "}\n"
                              "{\n"
                              "} 1 2\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "{");
    REQUIRE(ir->back()->command->name == "}");

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a unclosed scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a command")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("waIT 10 20 30\n"
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
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 3);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "C");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "C");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(ir->front()->label->name == "L");
    REQUIRE(ir->front()->command->name == "C:");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "A.SC");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a"
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "%");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "$");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "1");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == ".1");
    REQUIRE(ir->front()->command->num_args == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "-1");
    REQUIRE(ir->front()->command->num_args == 0);
}

TEST_CASE("parsing integer argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT 123 010 -39\n"
                              "WAIT 2147483647 -2147483648\n"
                              "WAIT -432-10\n"
                              "WAIT 123a\n"
                              "WAIT 0x10\n"
                              "WAIT +39\n"
                              "WAIT 432+10\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 3);
    REQUIRE(std::get<int32_t>(ir->front()->command->args[0]->value) == 123);
    REQUIRE(std::get<int32_t>(ir->front()->command->args[1]->value) == 10);
    REQUIRE(std::get<int32_t>(ir->front()->command->args[2]->value) == -39);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 2);
    REQUIRE(std::get<int32_t>(ir->front()->command->args[0]->value)
            == std::numeric_limits<int32_t>::max());
    REQUIRE(std::get<int32_t>(ir->front()->command->args[1]->value)
            == std::numeric_limits<int32_t>::min());

    ir = parser.parse_statement();
    parser.skip_current_line(); // -432-10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 123a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //0x10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // +39
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 432+10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing float argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT .1 -.1 .1f .1F .15 .1.9 -.1.\n"
                              "WAIT 1F -1f 1. 1.1 1.f 1.. -1..\n"
                              "WAIT .1a\n"
                              "WAIT .1fa\n"
                              "WAIT .1.a\n"
                              "WAIT 1..a\n"
                              "WAIT .1-.1\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 7);
    REQUIRE(*ir->front()->command->args[0]->as_float() == 0.1f);
    REQUIRE(*ir->front()->command->args[1]->as_float() == -0.1f);
    REQUIRE(std::get<float>(ir->front()->command->args[2]->value) == 0.1f);
    REQUIRE(std::get<float>(ir->front()->command->args[3]->value) == 0.1f);
    REQUIRE(std::get<float>(ir->front()->command->args[4]->value) == 0.15f);
    REQUIRE(std::get<float>(ir->front()->command->args[5]->value) == 0.1f);
    REQUIRE(std::get<float>(ir->front()->command->args[6]->value) == -0.1f);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 7);
    REQUIRE(std::get<float>(ir->front()->command->args[0]->value) == 1.0f);
    REQUIRE(std::get<float>(ir->front()->command->args[1]->value) == -1.0f);
    REQUIRE(std::get<float>(ir->front()->command->args[2]->value) == 1.0f);
    REQUIRE(std::get<float>(ir->front()->command->args[3]->value) == 1.1f);
    REQUIRE(std::get<float>(ir->front()->command->args[4]->value) == 1.0f);
    REQUIRE(std::get<float>(ir->front()->command->args[5]->value) == 1.0f);
    REQUIRE(std::get<float>(ir->front()->command->args[6]->value) == -1.0f);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // .1a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1fa
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //.1.a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1..a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1-.1
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing identifier argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT $abc abc AbC a@_1$\n"
                              "WAIT _abc\n"
                              "WAIT @abc\n"
                              "WAIT 1abc\n"
                              "WAIT abc: def\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 4);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_identifier(), "$ABC"sv);
    REQUIRE_EQ(*ir->front()->command->args[1]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front()->command->args[2]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front()->command->args[3]->as_identifier(), "A@_1$"sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // _abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // @abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //1abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // abc: def
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing string literal argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT \"this\tI$ /* a // \\n (%1teral),\"\n"
                              "WAIT \"\"\n"
                              "WAIT \"\n"
                              "WAIT \"string\"abc\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_string(),
               "this\tI$ /* a // \\n (%1teral),"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "WAIT");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_string(), ""sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // "
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "string"abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing filename argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("LAUNCH_MISSION .sc\n"
                              "LAUNCH_MISSION a.SC\n"
                              "WAIT a.SC\n"
                              "WAIT 1.SC\n"
                              "LAUNCH_MISSION @.sc\n"
                              "LAUNCH_MISSION 1.sc\n"
                              "LAUNCH_MISSION 1.0sc\n"                                                                      "LAUNCH_MISSION SC\n"
                              "LAUNCH_MISSION C\n"
                              "LAUNCH_MISSION \"a\".sc\n"
                              "LOAD_AND_LAUNCH_MISSION filename.sc\n"
                              "GOSUB_FILE label filename.sc\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_filename(), ".SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_filename(), "A.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // WAIT a.SC

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt); // WAIT 1.sc
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_filename(), "@.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front()->command->num_args == 1);
    REQUIRE_EQ(*ir->front()->command->args[0]->as_filename(), "1.SC"sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // 1.0sc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // SC
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // C
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a".sc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // LOAD_AND_LAUNCH_MISSION

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // GOSUB_FILE
}

// TODO label
// TODO expressions
// TODO expr or operator

#include <doctest.h>
#include <gta3sc/parser.hpp>
#include <cstring>

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
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next != nullptr);
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "LABEL");
    REQUIRE(std::get<gta3sc::ParserIR::Command>(ir->front()->next->op).name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(std::get<gta3sc::ParserIR::Command>(ir->front()->op).name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next == nullptr);
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "LA:BEL");

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
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "LABEL");
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
    {
    ir = ir;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "{");
    REQUIRE(command.num_arguments == 0);
    }

    REQUIRE(ir->next != nullptr);
    {
    ir = ir->next;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    }

    REQUIRE(ir->next != nullptr);
    {
    ir = ir->next;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    }

    REQUIRE(ir->next != nullptr);
    {
    ir = ir->next;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "}");
    REQUIRE(command.num_arguments == 0);
    REQUIRE(ir->next == nullptr);
    }

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    {
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    }

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    {
    ir = ir;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "{");
    REQUIRE(command.num_arguments == 0);
    }

    REQUIRE(ir->next != nullptr);
    {
    ir = ir->next;
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "}");
    REQUIRE(command.num_arguments == 0);
    REQUIRE(ir->next == nullptr);
    }

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->front();
    {
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    }
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

    auto linked = parser.parse_statement();
    REQUIRE(linked == std::nullopt);
    parser.skip_current_line();
    parser.skip_current_line();

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    REQUIRE(std::get<gta3sc::ParserIR::Command>(linked->front()->op).name == "{");
    REQUIRE(std::get<gta3sc::ParserIR::Command>(linked->back()->op).name == "}");

    linked = parser.parse_statement();
    REQUIRE(linked == std::nullopt);
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
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 3);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "C");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "C");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front()->next != nullptr);
    REQUIRE(std::get<gta3sc::ParserIR::LabelDef>(ir->front()->op).name == "L");
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->next->op);
    REQUIRE(command.name == "C:");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "A.SC");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a"
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "%");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "$");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "1");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == ".1");
    REQUIRE(command.num_arguments == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "-1");
    REQUIRE(command.num_arguments == 0);
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
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 3);
    REQUIRE(std::get<int32_t>(command.arguments[0]->value) == 123);
    REQUIRE(std::get<int32_t>(command.arguments[1]->value) == 10);
    REQUIRE(std::get<int32_t>(command.arguments[2]->value) == -39);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 2);
    REQUIRE(std::get<int32_t>(command.arguments[0]->value)
            == std::numeric_limits<int32_t>::max());
    REQUIRE(std::get<int32_t>(command.arguments[1]->value)
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
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 7);
    REQUIRE(std::get<float>(command.arguments[0]->value) == 0.1f);
    REQUIRE(std::get<float>(command.arguments[1]->value) == -0.1f);
    REQUIRE(std::get<float>(command.arguments[2]->value) == 0.1f);
    REQUIRE(std::get<float>(command.arguments[3]->value) == 0.1f);
    REQUIRE(std::get<float>(command.arguments[4]->value) == 0.15f);
    REQUIRE(std::get<float>(command.arguments[5]->value) == 0.1f);
    REQUIRE(std::get<float>(command.arguments[6]->value) == -0.1f);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 7);
    REQUIRE(std::get<float>(command.arguments[0]->value) == 1.0f);
    REQUIRE(std::get<float>(command.arguments[1]->value) == -1.0f);
    REQUIRE(std::get<float>(command.arguments[2]->value) == 1.0f);
    REQUIRE(std::get<float>(command.arguments[3]->value) == 1.1f);
    REQUIRE(std::get<float>(command.arguments[4]->value) == 1.0f);
    REQUIRE(std::get<float>(command.arguments[5]->value) == 1.0f);
    REQUIRE(std::get<float>(command.arguments[6]->value) == -1.0f);
    
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
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 4);
    REQUIRE(std::get<gta3sc::ParserIR::Identifier>(command.arguments[0]->value).name 
            == "$ABC");
    REQUIRE(std::get<gta3sc::ParserIR::Identifier>(command.arguments[1]->value).name
            == "ABC");
    REQUIRE(std::get<gta3sc::ParserIR::Identifier>(command.arguments[2]->value).name
            == "ABC");
    REQUIRE(std::get<gta3sc::ParserIR::Identifier>(command.arguments[3]->value).name
            == "A@_1$");
    
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
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::String>(command.arguments[0]->value).string
            == "this\tI$ /* a // \\n (%1teral),");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::String>(command.arguments[0]->value).string
            == "");
    
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
    // TODO
    return;

    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("LAUNCH_MISSION .sc\n"
                              "LAUNCH_MISSION a.SC\n"
                              "LAUNCH_MISSION @.sc\n"
                              "LAUNCH_MISSION 1.sc\n"
                              "LAUNCH_MISSION 1.0sc\n"                                                                      "LAUNCH_MISSION SC\n"
                              "LAUNCH_MISSION C\n"
                              "LAUNCH_MISSION \"a\".sc\n"
                              "LAUNCH_MISSION filename.sc\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    auto& command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "LAUNCH_MISSION");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::Filename>(command.arguments[0]->value).filename
            == ".SC");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "LAUNCH_MISSION");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::Filename>(command.arguments[0]->value).filename
            == "A.SC");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "LAUNCH_MISSION");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::Filename>(command.arguments[0]->value).filename
            == "@.SC");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    command = std::get<gta3sc::ParserIR::Command>(ir->front()->op);
    REQUIRE(command.name == "LAUNCH_MISSION");
    REQUIRE(command.num_arguments == 1);
    REQUIRE(std::get<gta3sc::ParserIR::Filename>(command.arguments[0]->value).filename
            == "1.SC");
    
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
    REQUIRE(ir != std::nullopt); // filename.sc
}

// TODO label
// TODO expressions
// TODO expr or operator

#include "doctest.h"
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

TEST_CASE("parsing a command")
{
    auto arena = gta3sc::ArenaMemoryResource();
    auto source = make_source("WAIT 10 20 30");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_command_statement();
    REQUIRE(ir != std::nullopt);

    REQUIRE(std::holds_alternative<gta3sc::ParserIR::Command>((*ir)->op));
    auto& command = std::get<gta3sc::ParserIR::Command>((*ir)->op);

    REQUIRE(command.name == "WAIT");
    REQUIRE(command.num_arguments == 3);

    gta3sc::ParserIR::Argument* arg;

    arg = command.arguments[0];
    REQUIRE(std::holds_alternative<int32_t>(arg->value));
    REQUIRE(std::get<int32_t>(arg->value) == 10);

    arg = command.arguments[1];
    REQUIRE(std::holds_alternative<int32_t>(arg->value));
    REQUIRE(std::get<int32_t>(arg->value) == 20);

    arg = command.arguments[2];
    REQUIRE(std::holds_alternative<int32_t>(arg->value));
    REQUIRE(std::get<int32_t>(arg->value) == 30);

    REQUIRE(std::move(parser).to_scanner().eof());
}

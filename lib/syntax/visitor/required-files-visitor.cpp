#include <gta3sc/syntax/visitor/required-files-visitor.hpp>
#include <string_view>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto command_gosub_file = "GOSUB_FILE"sv;
constexpr auto command_launch_mission = "LAUNCH_MISSION"sv;
constexpr auto command_load_and_launch_mission = "LOAD_AND_LAUNCH_MISSION"sv;
} // namespace

namespace gta3sc::syntax
{
void RequiredFilesVisitor::visit(const ParserIR& instruction)
{
    if(!instruction.has_command())
        return;

    const auto& command = instruction.command();
    if(command.name() == command_gosub_file)
    {
        visit_require(command, 1, SymbolTable::FileType::main_extension);
    }
    else if(command.name() == command_launch_mission)
    {
        visit_require(command, 0, SymbolTable::FileType::subscript);
    }
    else if(command.name() == command_load_and_launch_mission)
    {
        visit_require(command, 0, SymbolTable::FileType::mission);
    }
}

void RequiredFilesVisitor::visit_require(const ParserIR::Command& command,
                                         uint32_t arg_index,
                                         SymbolTable::FileType file_type)
{
    if(command.num_args() <= arg_index)
        return;

    const auto& arg = command.arg(arg_index);
    if(arg.type() != ParserIR::Argument::Type::FILENAME)
        return;

    visit_require(command, arg, file_type);
}

CallbackRequiredFilesVisitor::CallbackRequiredFilesVisitor(
        CallbackFunction callback) noexcept :
    callback(std::move(callback))
{}

void CallbackRequiredFilesVisitor::visit_require(
        const ParserIR::Command& command, const ParserIR::Argument& arg,
        SymbolTable::FileType file_type)
{
    callback(command, arg, file_type);
}
} // namespace gta3sc::syntax
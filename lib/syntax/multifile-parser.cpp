#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/preprocessor.hpp>
#include <gta3sc/syntax/scanner.hpp>
#include <queue>
#include <string_view>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto file_label_prefix = "@@"sv;

constexpr auto main_file_identifier = "MAIN"sv;

constexpr auto command_gosub_file = "GOSUB_FILE"sv;
constexpr auto command_launch_mission = "LAUNCH_MISSION"sv;
constexpr auto command_load_and_launch_mission = "LOAD_AND_LAUNCH_MISSION"sv;
} // namespace

namespace gta3sc::syntax
{

bool MultifileParser::ParseQueueItem::operator<(
        const ParseQueueItem& other) const
{
    if(file->type() == other.file->type())
        return file->type_id() < other.file->type_id();
    return file->type() < other.file->type();
}

MultifileParser::MultifileParser(std::filesystem::path main_script_path,
                                 SymbolTable& symbol_table,
                                 SourceManager& source_manager,
                                 DiagnosticHandler& diag,
                                 ArenaAllocator<> allocator) :
    main_script_path(std::move(main_script_path)),
    symbol_table(&symbol_table),
    source_manager(&source_manager),
    diag(&diag),
    ir_allocator(allocator)
{
    auto [main_file, _] = symbol_table.insert_file(
            main_file_identifier, SymbolTable::FileType::main,
            gta3sc::SourceManager::no_source_range);
    parse_queue.emplace(main_file); // insert root of parse queue
}

auto MultifileParser::parse() -> std::optional<LinkedIR<ParserIR>>
{
    LinkedIR<ParserIR> result_ir;

    std::string file_label_buffer;
    file_label_buffer.reserve(100);

    while(!parse_queue.empty())
    {
        const auto& file = *parse_queue.top().file;
        parse_queue.pop();

        if(static_cast<int>(file.type()) < static_cast<int>(parsing_file_type))
        {
            // TODO diag but keep going

            // Can keep going as this is a fatal error only at the codegen
            // stage.
        }

        parsing_file_type = file.type();

        auto source_file = load_file(file);
        if(!source_file)
            return std::nullopt;

        auto ir = parse(std::move(*source_file), file.type());
        if(!ir)
            return std::nullopt;

        analyze_required_files(*ir);

        file_label_buffer.assign(file_label_prefix);
        file_label_buffer.append(file.name());
        ir->push_front(*ParserIR::Builder(ir_allocator)
                                .label(file_label_buffer, file.source())
                                .build());

        result_ir.splice_back(std::move(*ir));
    }

    return result_ir;
}

auto MultifileParser::load_file(const SymbolTable::File& file)
        -> std::optional<SourceFile>
{
    if(file.type() == SymbolTable::FileType::main)
    {
        auto source_file = source_manager->load_file(main_script_path);
        if(!source_file)
        {
            diag->report(gta3sc::SourceManager::no_source_loc,
                         gta3sc::Diag::
                                 config_xml_could_not_open_file) // TODO diag
                                                                 // not a xml
                                                                 // error
                    .args(main_script_path.generic_string());
            return std::nullopt;
        }
        return source_file;
    }

    auto source_file = source_manager->load_file(file.name());
    if(!source_file)
    {
        // TODO diag
        return std::nullopt;
    }
    return source_file;
}

auto MultifileParser::parse(SourceFile source_file, SymbolTable::FileType type)
        -> std::optional<LinkedIR<ParserIR>>
{
    Preprocessor pp(std::move(source_file), *diag);
    Scanner scanner(std::move(pp));
    Parser parser(std::move(scanner), ir_allocator);

    switch(type)
    {
        case SymbolTable::FileType::main:
            return parser.parse_main_script_file();
        case SymbolTable::FileType::main_extension:
            return parser.parse_main_extension_file();
        case SymbolTable::FileType::subscript:
            return parser.parse_subscript_file();
        case SymbolTable::FileType::mission:
            return parser.parse_mission_script_file();
    }

    assert(false);
}

void MultifileParser::analyze_required_files(const LinkedIR<ParserIR>& ir)
{
    for(const auto& line : ir)
    {
        if(line.has_command())
        {
            const auto& command = line.command();
            if(command.name() == command_gosub_file)
            {
                analyze_required_file(command, 1,
                                      SymbolTable::FileType::main_extension);
            }
            else if(command.name() == command_launch_mission)
            {
                analyze_required_file(command, 0,
                                      SymbolTable::FileType::subscript);
            }
            else if(command.name() == command_load_and_launch_mission)
            {
                analyze_required_file(command, 0,
                                      SymbolTable::FileType::mission);
            }
        }
    }
}

void MultifileParser::analyze_required_file(const ParserIR::Command& command,
                                            uint32_t arg_index,
                                            SymbolTable::FileType type)
{
    if(command.num_args() <= arg_index)
        return;

    const auto& arg = command.arg(arg_index);
    const auto filename = arg.as_filename();
    if(!filename)
        return;

    // TODO I could put this in an analyzer/visitor and the code below would be
    // the custom part

    auto [file, newly_inserted] = symbol_table->insert_file(*filename, type,
                                                            arg.source());
    if(newly_inserted)
    {
        parse_queue.emplace(file);
        return;
    }

    if(file->type() != type)
    {
        // TODO diag
        return;
    }
}
} // namespace gta3sc::syntax
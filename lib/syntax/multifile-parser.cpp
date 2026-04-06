#include <gta3sc/syntax/multifile-parser.hpp>
#include <gta3sc/syntax/parser.hpp>
#include <gta3sc/syntax/preprocessor.hpp>
#include <gta3sc/syntax/scanner.hpp>
#include <gta3sc/syntax/visitor/required-files-visitor.hpp>
#include <string_view>
using namespace std::literals::string_view_literals;

namespace
{
constexpr auto file_label_prefix = "@@"sv;
constexpr auto main_file_identifier = "MAIN"sv;
} // namespace

namespace gta3sc::syntax::diag
{
const DiagnosticDescriptor unsupported_main_extension_import_order(
        DiagnosticSeverity::error, "Unsupported main extension import order",
        "TODO");
const DiagnosticDescriptor
        unsupported_subscript_import_order(DiagnosticSeverity::error,
                                           "Unsupported subscript import order",
                                           "TODO");
const DiagnosticDescriptor unsupported_mission_import_order(
        DiagnosticSeverity::error, "Unsupported mission import order", "TODO");

const DiagnosticDescriptor file_already_imported_as_different_type(
        DiagnosticSeverity::error, "File already imported as different type",
        "TODO");
} // namespace gta3sc::syntax::diag

namespace gta3sc::syntax
{
bool MultifileParser::ParseQueueItem::operator<(
        const ParseQueueItem& other) const
{
    if(file->type() == other.file->type())
        return file->type_id() > other.file->type_id();
    return file->type() > other.file->type();
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

auto MultifileParser::has_next_file() -> bool
{
    return !parse_queue.empty();
}

auto MultifileParser::parse_next_file() -> std::optional<LinkedIR<ParserIR>>
{
    assert(has_next_file());

    bool has_error = false;

    const auto& file = *parse_queue.top().file;
    parse_queue.pop();

    if(file.type() < parsing_file_type)
    {
        report_unsupported_import_order(file);
        has_error = true;
    }
    else
    {
        // Keep the highest file type seen so far
        parsing_file_type = file.type();
    }

    auto source_file = load_file(file);
    if(!source_file)
        return std::nullopt;

    auto ir = parse(std::move(*source_file), file.type());
    if(!ir)
        return std::nullopt;

    if(has_error)
        return std::nullopt;

    push_required_files(*ir);

    std::string file_label_buffer;
    file_label_buffer.reserve(file_label_prefix.size() + file.name().size());
    file_label_buffer.assign(file_label_prefix);
    file_label_buffer.append(file.name());

    ir->push_front(*ParserIR::Builder(ir_allocator)
                            .label(file_label_buffer, file.source())
                            .build());
    return ir;
}

auto MultifileParser::parse() -> std::optional<LinkedIR<ParserIR>>
{
    bool has_error = false;
    LinkedIR<ParserIR> result_ir;

    while(has_next_file())
    {
        auto file_ir = parse_next_file();
        if(!file_ir)
        {
            has_error = true;
            continue;
        }
        result_ir.splice_back(std::move(*file_ir));
    }

    if(has_error)
        return std::nullopt;

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
                         gta3sc::diag::could_not_open_file)
                    .args(main_script_path.generic_string());
            return std::nullopt;
        }
        return source_file;
    }

    auto source_file = source_manager->load_file(file.name());
    if(!source_file)
    {
        diag->report(gta3sc::SourceManager::no_source_loc,
                     gta3sc::diag::could_not_load_file)
                .args(file.name());
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

void MultifileParser::report_unsupported_import_order(
        const SymbolTable::File& imported_file)
{
    switch(imported_file.type())
    {
        case SymbolTable::FileType::main:
            // main cannot be imported from another file so this is unreachable
            break;
        case SymbolTable::FileType::main_extension:
            diag->report(imported_file.source(),
                         diag::unsupported_main_extension_import_order);
            return;
        case SymbolTable::FileType::subscript:
            diag->report(imported_file.source(),
                         diag::unsupported_subscript_import_order);
            return;
        case SymbolTable::FileType::mission:
            diag->report(imported_file.source(),
                         diag::unsupported_mission_import_order);
            return;
    }

    assert(false);
}

void MultifileParser::push_required_files(const LinkedIR<ParserIR>& ir)
{
    CallbackRequiredFilesVisitor([this](const ParserIR::Command&,
                                        const ParserIR::Argument& arg,
                                        SymbolTable::FileType type) {
        const auto filename = arg.as_filename();
        assert(filename != std::nullopt);

        auto [file, newly_inserted] = symbol_table->insert_file(*filename, type,
                                                                arg.source());
        if(newly_inserted)
        {
            parse_queue.emplace(file);
            return;
        }

        if(file->type() != type)
        {
            // FIXME find a better way to include file types in the
            // diagnostic
            diag->report(arg.source().begin,
                         diag::file_already_imported_as_different_type)
                    .range(arg.source())
                    .range(file->source())
                    .args(static_cast<int>(file->type()),
                          static_cast<int>(type));
            return;
        }
    }).visit_each(ir);
}
} // namespace gta3sc::syntax
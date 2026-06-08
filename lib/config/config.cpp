#include <charconv>
#include <cstring>
#include <gta3sc/config/config.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/filesystem/file-pool.hpp>
#include <gta3sc/source-manager.hpp>
#include <gta3sc/util/arena.hpp>
#include <gta3sc/util/ctype.hpp>
#include <gta3sc/util/memory.hpp>
#include <memory>
#include <pugixml.hpp>
using namespace std::string_view_literals;

namespace gta3sc::config::diag
{
// Config XML diagnostics
const DiagnosticDescriptor
        xml_missing_required_attr(DiagnosticSeverity::error,
                                  "XML missing required attribute", "TODO");
const DiagnosticDescriptor xml_empty_attr(DiagnosticSeverity::error,
                                          "XML empty attribute", "TODO");
const DiagnosticDescriptor xml_unknown_node(DiagnosticSeverity::error,
                                            "XML unknown node", "TODO");
const DiagnosticDescriptor xml_parse_failed(DiagnosticSeverity::error,
                                            "XML parse failed", "TODO");
const DiagnosticDescriptor xml_invalid_root_element(DiagnosticSeverity::error,
                                                    "XML invalid root element",
                                                    "TODO");
const DiagnosticDescriptor xml_import_too_deep(DiagnosticSeverity::error,
                                               "XML import too deep", "TODO");
const DiagnosticDescriptor xml_invalid_param_type(DiagnosticSeverity::error,
                                                  "XML invalid parameter type",
                                                  "TODO");
const DiagnosticDescriptor
        xml_opt_must_be_last_param(DiagnosticSeverity::error,
                                   "XML optional parameter must be last",
                                   "TODO");
const DiagnosticDescriptor xml_security_import_filesystem_traversal(
        DiagnosticSeverity::error, "XML security: import filesystem traversal",
        "TODO");
const DiagnosticDescriptor xml_import_failed_to_determine_game_config(
        DiagnosticSeverity::error, "XML import failed to determine game config",
        "TODO");
const DiagnosticDescriptor
        xml_invalid_constant_value(DiagnosticSeverity::error,
                                   "XML invalid constant value", "TODO");
const DiagnosticDescriptor xml_invalid_handled_without_id(
        DiagnosticSeverity::error, "XML invalid handled without ID", "TODO");
const DiagnosticDescriptor xml_invalid_command_id(DiagnosticSeverity::error,
                                                  "XML invalid command ID",
                                                  "TODO");
const DiagnosticDescriptor xml_expected_boolean(DiagnosticSeverity::error,
                                                "XML expected boolean", "TODO");
} // namespace gta3sc::config::diag

// Configuration files are in XML.
// Schema can be found at
// https://github.com/GTAmodding/gta3script-config/blob/gta3sc-rewrite/schema.rnc.
// Example configuration files can be found at
// https://github.com/GTAmodding/gta3script-config/tree/gta3sc-rewrite/config.

// TODO: V1 support for backward compatibility with older configs.
//   - We can have a ConfigConverter that can convert V1 root node to V2 root
//   node
//   - We can probably also expose it in a CLI for users to convert their
//   configs!

namespace
{
using namespace gta3sc;

class ConfigLoader
{
public:
    /// See \ref gta3sc::config::load_config for more information.
    ///
    /// \param root_path The root path of the game configs, or empty if imports
    /// are not allowed. \param file_pool The file pool to use to load files or
    /// nullptr if imports are not allowed. \param diagman The diagnostic
    /// handler to use to report errors. \param builder The builder to use to
    /// build the command table.
    ConfigLoader(const std::filesystem::path& root_path, FilePool* file_pool,
                 DiagnosticHandler& diagman, CommandTable::Builder& builder) :
        builder(builder),
        root_path(root_path),
        file_pool(file_pool),
        diagman(diagman)
    {}

    ConfigLoader(const ConfigLoader&) = delete;
    auto operator=(const ConfigLoader&) -> ConfigLoader& = delete;

    ConfigLoader(ConfigLoader&&) = delete;
    auto operator=(ConfigLoader&&) -> ConfigLoader& = delete;

    /// Loads the config file into the builder.
    void load_config(const FileEntryRef& config_file);

private:
    // Processing methods

    void process_import(const FileEntryRef& config_file,
                        const pugi::xml_node& node);

    void process_commands_section(const FileEntryRef& config_file,
                                  const pugi::xml_node& section);
    void process_command(const FileEntryRef& config_file,
                         const pugi::xml_node& node);
    void process_command_id(const FileEntryRef& config_file,
                            const pugi::xml_node& node);
    auto process_param_node(const FileEntryRef& config_file,
                            const pugi::xml_node& param)
            -> std::optional<CommandTable::ParamDef>;

    void process_alternators_section(const FileEntryRef& config_file,
                                     const pugi::xml_node& section);
    void process_alternator_node(const FileEntryRef& config_file,
                                 const pugi::xml_node& node);
    void process_alternative_node(const FileEntryRef& config_file,
                                  const pugi::xml_node& node,
                                  CommandTable::AlternatorDef& alternator);

    void process_constants_section(const FileEntryRef& config_file,
                                   const pugi::xml_node& section);
    void process_enum_node(const FileEntryRef& config_file,
                           const pugi::xml_node& enum_node);
    void process_constant_node(const FileEntryRef& config_file,
                               const pugi::xml_node& constant_node,
                               CommandTable::EnumId enum_id,
                               int32_t& next_value);
    void process_constant_node(const FileEntryRef& config_file,
                               const pugi::xml_node& constant_node,
                               CommandTable::EnumId enum_id,
                               int32_t& next_value, bool mirror_global);
    auto parse_constant_value(const FileEntryRef& config_file,
                              const pugi::xml_node& constant_node,
                              const char* value_str) -> int32_t;

    // Diagnostic report methods
    void report_missing_required_attr(const FileEntryRef& config_file,
                                      const pugi::xml_node& node,
                                      std::string_view attr_name);
    void report_empty_attr(const FileEntryRef& config_file,
                           const pugi::xml_node& node,
                           std::string_view attr_name);
    void report_unknown_node(const FileEntryRef& config_file,
                             const pugi::xml_node& node);
    void report_parse_error(const FileEntryRef& config_file, ptrdiff_t offset,
                            const char* description);
    void report_invalid_root_element(const FileEntryRef& config_file,
                                     const pugi::xml_node& root);
    void report_import_too_deep(const FileEntryRef& config_file,
                                const pugi::xml_node& node);
    void report_invalid_param_type(const FileEntryRef& config_file,
                                   const pugi::xml_node& node,
                                   const char* type_value);
    void report_opt_param_not_last(const FileEntryRef& config_file,
                                   const pugi::xml_node& node);
    void report_import_security_error(const FileEntryRef& config_file,
                                      const pugi::xml_node& node,
                                      const char* from_value);
    void report_import_game_config_error(const FileEntryRef& config_file,
                                         const pugi::xml_node& node);
    void report_could_not_open_file(const FileEntryRef& config_file,
                                    const pugi::xml_node& node,
                                    std::string_view path);
    void report_invalid_constant_value(const FileEntryRef& config_file,
                                       const pugi::xml_node& node,
                                       const char* value);
    void report_invalid_handled_without_id(const FileEntryRef& config_file,
                                           const pugi::xml_node& node);
    void report_invalid_command_id(const FileEntryRef& config_file,
                                   const pugi::xml_node& node,
                                   const char* id_value);
    void report_expected_boolean(const FileEntryRef& config_file,
                                 const pugi::xml_node& node, const char* value);

    // Other utilities
    auto
    determine_import_path(const char* from, const FileEntryRef& config_file,
                          const pugi::xml_node& node) -> std::filesystem::path;

    /// Converts a string to uppercase.
    ///
    /// This function uses an allocation storage with the same lifetime as the
    /// config loader. Please don't keep references to the uppercase view
    /// around.
    auto toupper(std::string_view str) -> std::string_view
    {
        return util::new_string(str, ArenaAllocator<>(&scratchpad),
                                util::toupper);
    }

private:
    CommandTable::Builder& builder;
    const std::filesystem::path& root_path;
    FilePool* file_pool;
    DiagnosticHandler& diagman;
    ArenaMemoryResource scratchpad;
    uint8_t import_depth{0};

    // TODO instead of an arena leaking all over the place, use member variables
    // for example vec scratchpad_params, str scratchpad_upper, etc.
};
} // namespace

namespace gta3sc::config
{
auto load_config(const std::filesystem::path& root_path,
                 const std::filesystem::path& config_path, FilePool& file_pool,
                 DiagnosticHandler& diagman,
                 ArenaAllocator<> allocator) -> CommandTable
{
    CommandTable::Builder builder(allocator);
    return load_config(root_path, config_path, file_pool, diagman,
                       std::move(builder))
            .build();
}

auto load_config(const std::filesystem::path& root_path,
                 const std::filesystem::path& config_path, FilePool& file_pool,
                 DiagnosticHandler& diagman,
                 CommandTable::Builder&& builder) -> CommandTable::Builder&&
{
    // Load the config file into the file pool
    auto config_file = file_pool.load_file(config_path);
    if(!config_file)
    {
        diagman.report(no_file_loc, gta3sc::diag::could_not_open_file)
                .args(config_path.generic_string());
        return std::move(builder);
    }

    // TODO page out the config file when done

    ConfigLoader loader(root_path, &file_pool, diagman, builder);
    loader.load_config(*config_file);

    // Return the same rvalue reference as given as input.
    return std::move(builder);
}

auto load_config(const FileEntryRef& config_file, DiagnosticHandler& diagman,
                 CommandTable::Builder&& builder) -> CommandTable::Builder&&
{
    std::filesystem::path null_path;

    ConfigLoader loader(null_path, nullptr, diagman, builder);
    loader.load_config(config_file);

    // Return the same rvalue reference as given as input.
    return std::move(builder);
}
} // namespace gta3sc::config

namespace
{
auto xml_location(const FileEntryRef& config_file, ptrdiff_t offset) -> FileLoc
{
    return offset > 0 ? config_file.location_of(config_file.data() + offset)
                      : no_file_loc;
}

auto xml_location(const FileEntryRef& config_file,
                  const pugi::xml_node& node) -> FileLoc
{
    return xml_location(config_file, node.offset_debug());
}

auto parse_boolean(const char* str) -> std::optional<bool>
{
    if(std::strcmp(str, "true") == 0)
        return true;
    if(std::strcmp(str, "false") == 0)
        return false;
    return std::nullopt;
}

void ConfigLoader::report_missing_required_attr(const FileEntryRef& config_file,
                                                const pugi::xml_node& node,
                                                std::string_view attr_name)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_missing_required_attr)
            .args(attr_name);
}

void ConfigLoader::report_empty_attr(const FileEntryRef& config_file,
                                     const pugi::xml_node& node,
                                     std::string_view attr_name)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_empty_attr)
            .args(attr_name);
}

void ConfigLoader::report_unknown_node(const FileEntryRef& config_file,
                                       const pugi::xml_node& node)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_unknown_node)
            .args(node.name());
}

void ConfigLoader::report_parse_error(const FileEntryRef& config_file,
                                      ptrdiff_t offset, const char* description)
{
    diagman.report(xml_location(config_file, offset),
                   config::diag::xml_parse_failed)
            .args(description);
}

void ConfigLoader::report_invalid_root_element(const FileEntryRef& config_file,
                                               const pugi::xml_node& root)
{
    diagman.report(xml_location(config_file, root),
                   config::diag::xml_invalid_root_element)
            .args(root ? root.name() : "");
}

void ConfigLoader::report_import_too_deep(const FileEntryRef& config_file,
                                          const pugi::xml_node& node)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_import_too_deep);
}

void ConfigLoader::report_invalid_param_type(const FileEntryRef& config_file,
                                             const pugi::xml_node& node,
                                             const char* type_value)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_invalid_param_type)
            .args(type_value);
}

void ConfigLoader::report_opt_param_not_last(const FileEntryRef& config_file,
                                             const pugi::xml_node& node)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_opt_must_be_last_param);
}

void ConfigLoader::report_import_security_error(const FileEntryRef& config_file,
                                                const pugi::xml_node& node,
                                                const char* from_value)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_security_import_filesystem_traversal)
            .args(from_value);
}

void ConfigLoader::report_import_game_config_error(
        const FileEntryRef& config_file, const pugi::xml_node& node)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_import_failed_to_determine_game_config);
}

void ConfigLoader::report_could_not_open_file(const FileEntryRef& config_file,
                                              const pugi::xml_node& node,
                                              std::string_view path)
{
    diagman.report(xml_location(config_file, node),
                   gta3sc::diag::could_not_open_file)
            .args(path);
}

void ConfigLoader::report_invalid_constant_value(
        const FileEntryRef& config_file, const pugi::xml_node& node,
        const char* value)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_invalid_constant_value)
            .args(value);
}

void ConfigLoader::report_invalid_handled_without_id(
        const FileEntryRef& config_file, const pugi::xml_node& node)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_invalid_handled_without_id);
}

void ConfigLoader::report_invalid_command_id(const FileEntryRef& config_file,
                                             const pugi::xml_node& node,
                                             const char* id_value)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_invalid_command_id)
            .args(id_value);
}

void ConfigLoader::report_expected_boolean(const FileEntryRef& config_file,
                                           const pugi::xml_node& node,
                                           const char* value)
{
    diagman.report(xml_location(config_file, node),
                   config::diag::xml_expected_boolean)
            .args(value);
}

void ConfigLoader::load_config(const FileEntryRef& config_file)
{
    pugi::xml_document doc;

    // Copies (and parses) the config file into a internal pugixml buffer.
    // This is necessary because pugixml modifies the buffer as it parses,
    // however we need to keep the original content for pretty diagnostics.
    if(auto result = doc.load_buffer(config_file.data(), config_file.size());
       !result)
    {
        report_parse_error(config_file, result.offset, result.description());
        return;
    }

    // Validate root element
    const auto root = doc.document_element();
    if(!root || std::strcmp(root.name(), "GTA3Script") != 0)
    {
        report_invalid_root_element(config_file, root);
        return;
    }

    // Validate version
    if(const auto version = root.attribute("Version");
       !version || std::strcmp(version.value(), "2.0") != 0)
    {
        report_missing_required_attr(config_file, root, "Version"sv);
        return;
    }

    // Process each child node in order
    for(auto node = root.first_child(); node; node = node.next_sibling())
    {
        if(std::strcmp(node.name(), "Import") == 0)
        {
            if(this->import_depth >= 1)
            {
                report_import_too_deep(config_file, node);
                continue;
            }

            process_import(config_file, node);
        }
        else if(std::strcmp(node.name(), "Commands") == 0)
        {
            process_commands_section(config_file, node);
        }
        else if(std::strcmp(node.name(), "Alternators") == 0)
        {
            process_alternators_section(config_file, node);
        }
        else if(std::strcmp(node.name(), "Constants") == 0)
        {
            process_constants_section(config_file, node);
        }
        else
        {
            report_unknown_node(config_file, node);
        }
    }
}

void ConfigLoader::process_import(const FileEntryRef& config_file,
                                  const pugi::xml_node& node)
{
    assert(file_pool != nullptr);
    assert(!root_path.empty());

    // Get the import name (required)
    auto name = node.attribute("Name");
    if(!name || name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, node, "Name"sv);
        return;
    }

    // Get the From attribute (optional)
    auto from = node.attribute("From");
    if(from && from.value()[0] == '\0')
    {
        report_empty_attr(config_file, node, "From"sv);
        return;
    }

    // Figure out the base path of the import based on the From attribute.
    auto import_path = this->determine_import_path(from.value(), config_file,
                                                   node);
    if(import_path.empty()) // in case of error
        return;

    import_path /= name.value();

    auto import_file = file_pool->load_file(import_path);
    if(!import_file)
    {
        report_could_not_open_file(config_file, node,
                                   import_path.generic_string());
        return;
    }

    ++this->import_depth;
    this->load_config(*import_file);
    --this->import_depth;

    // TODO page out the imported file when done
}

auto ConfigLoader::determine_import_path(
        const char* from, const FileEntryRef& config_file,
        const pugi::xml_node& node) -> std::filesystem::path
{
    // TODO could we perform path resolution using a PathResolver?
    //      maybe we could pass a RelativePathResolver as argument to
    //      load_config, rather than root_path?

    assert(from != nullptr);

    if(from[0] == '\0')
    {
        // "If not specified, imports from the config directory of the game
        // being targeted"
        std::error_code ec;
        auto rel_path = std::filesystem::relative(config_file.path(), root_path,
                                                  ec);

        if(ec || rel_path.empty() || rel_path.begin() == rel_path.end()
           || *rel_path.begin() == "..")
        {
            // This should never happen - internal error.
            // Config file is not inside root path.
            report_import_game_config_error(config_file, node);
            return std::filesystem::path();
        }

        // Use the first component of the relative path as game config
        return root_path / *rel_path.begin();
    }
    else if(std::strcmp(from, ".") == 0)
    {
        // "If equal '.' imports from the same directory as the
        // configuration file being read"
        auto result = config_file.path();
        result.remove_filename();
        return result;
    }
    else
    {
        // "Imports the configurations from the specified game config"

        // Validate From is a plain text string (no slashes or dots)
        // TODO dots as part of the filename are not necessarily a problem (e.g.
        // game1.jp)
        for(const char* p = from; *p; ++p)
        {
            if(*p == '/' || *p == '\\' || *p == '.')
            {
                report_import_security_error(config_file, node, from);
                return std::filesystem::path();
            }
        }

        return root_path / from;
    }
}

auto parse_command_id(std::string_view str) -> std::optional<int16_t>
{
    int base = 10;
    if(str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        base = 16;
        str.remove_prefix(2);
    }

    uint16_t value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value,
                                     base);

    // Must be a valid number and consume entire string
    if(ec != std::errc() || ptr != str.data() + str.size())
        return std::nullopt;

    return static_cast<int16_t>(value);
}

void ConfigLoader::process_command_id(const FileEntryRef& config_file,
                                      const pugi::xml_node& node)
{
    // Get the Name (required)
    auto name = node.attribute("Name");
    if(!name || name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, node, "Name"sv);
        return;
    }

    // Get the ID (optional)
    std::optional<int16_t> target_id;
    auto id_attr = node.attribute("ID");
    if(id_attr)
    {
        if(target_id = parse_command_id(id_attr.value()); !target_id)
            report_invalid_command_id(config_file, node, id_attr.value());
    }

    // Get the Handled (optional)
    bool handled = target_id.has_value();
    if(auto handled_attr = node.attribute("Handled"))
    {
        if(auto opt_handled = parse_boolean(handled_attr.value()))
            handled = *opt_handled;
        else
            report_expected_boolean(config_file, node, handled_attr.value());
    }

    // "If this id is not specified, the `Handled` attribute must be false."
    if(!target_id && handled)
        report_invalid_handled_without_id(config_file, node);

    // Command ids must be positive
    if(target_id && *target_id < 0)
    {
        report_invalid_command_id(config_file, node, id_attr.value());
        target_id = std::nullopt;
    }

    auto [command, _] = builder.insert_command(toupper(name.value()));
    assert(command != nullptr);

    builder.set_command_id(*command, target_id, handled);
}

void ConfigLoader::process_commands_section(const FileEntryRef& config_file,
                                            const pugi::xml_node& section)
{
    for(auto node = section.first_child(); node; node = node.next_sibling())
    {
        if(std::strcmp(node.name(), "CommandId") == 0)
        {
            process_command_id(config_file, node);
        }
        else if(std::strcmp(node.name(), "Command") == 0)
        {
            process_command(config_file, node);
        }
        else
        {
            report_unknown_node(config_file, node);
        }
    }
}

auto parse_param_type(std::string_view type_str)
        -> std::optional<CommandTable::ParamType>
{
    // Map string to ParamType enum
    static const std::pair<std::string_view, CommandTable::ParamType> type_map[]
            = {
                    {"INT"sv, CommandTable::ParamType::INT},
                    {"FLOAT"sv, CommandTable::ParamType::FLOAT},
                    {"VAR_INT"sv, CommandTable::ParamType::VAR_INT},
                    {"LVAR_INT"sv, CommandTable::ParamType::LVAR_INT},
                    {"VAR_FLOAT"sv, CommandTable::ParamType::VAR_FLOAT},
                    {"LVAR_FLOAT"sv, CommandTable::ParamType::LVAR_FLOAT},
                    {"VAR_TEXT_LABEL"sv,
                     CommandTable::ParamType::VAR_TEXT_LABEL},
                    {"LVAR_TEXT_LABEL"sv,
                     CommandTable::ParamType::LVAR_TEXT_LABEL},
                    {"INPUT_INT"sv, CommandTable::ParamType::INPUT_INT},
                    {"INPUT_FLOAT"sv, CommandTable::ParamType::INPUT_FLOAT},
                    {"OUTPUT_INT"sv, CommandTable::ParamType::OUTPUT_INT},
                    {"OUTPUT_FLOAT"sv, CommandTable::ParamType::OUTPUT_FLOAT},
                    {"LABEL"sv, CommandTable::ParamType::LABEL},
                    {"TEXT_LABEL"sv, CommandTable::ParamType::TEXT_LABEL},
                    {"STRING"sv, CommandTable::ParamType::STRING},
                    {"VAR_INT_OPT"sv, CommandTable::ParamType::VAR_INT_OPT},
                    {"LVAR_INT_OPT"sv, CommandTable::ParamType::LVAR_INT_OPT},
                    {"VAR_FLOAT_OPT"sv, CommandTable::ParamType::VAR_FLOAT_OPT},
                    {"LVAR_FLOAT_OPT"sv,
                     CommandTable::ParamType::LVAR_FLOAT_OPT},
                    {"VAR_TEXT_LABEL_OPT"sv,
                     CommandTable::ParamType::VAR_TEXT_LABEL_OPT},
                    {"LVAR_TEXT_LABEL_OPT"sv,
                     CommandTable::ParamType::LVAR_TEXT_LABEL_OPT},
                    {"INPUT_OPT"sv, CommandTable::ParamType::INPUT_OPT},
            };

    for(const auto& [str, type] : type_map)
    {
        if(type_str == str)
            return type;
    }
    return std::nullopt;
}

auto ConfigLoader::process_param_node(const FileEntryRef& config_file,
                                      const pugi::xml_node& param)
        -> std::optional<CommandTable::ParamDef>
{
    // Get the type (required)
    auto type_attr = param.attribute("Type");
    if(!type_attr)
    {
        report_missing_required_attr(config_file, param, "Type"sv);
        return std::nullopt;
    }

    auto type = parse_param_type(type_attr.value());
    if(!type)
    {
        report_invalid_param_type(config_file, param, type_attr.value());
        return std::nullopt;
    }

    // Get enum reference (optional)
    auto enum_attr = param.attribute("Enum");
    std::optional<CommandTable::EnumId> enum_id;
    if(enum_attr)
    {
        auto enum_attr_value = enum_attr.value();
        if(enum_attr_value[0] == '\0')
        {
            report_empty_attr(config_file, param, "Enum"sv);
            return std::nullopt;
        }
        auto [enum_id_new,
              _] = builder.insert_enumeration(toupper(enum_attr_value));
        enum_id = enum_id_new;
    }

    // Get entity reference (optional)
    auto entity_attr = param.attribute("Entity");
    std::optional<CommandTable::EntityId> entity_id;
    if(entity_attr)
    {
        auto entity_attr_value = entity_attr.value();
        if(entity_attr_value[0] == '\0')
        {
            report_empty_attr(config_file, param, "Entity"sv);
            return std::nullopt;
        }
        auto [entity_id_new,
              _] = builder.insert_entity_type(toupper(entity_attr_value));
        entity_id = entity_id_new;
    }

    return CommandTable::ParamDef(
            *type, entity_id.value_or(CommandTable::no_entity_type),
            enum_id.value_or(CommandTable::global_enum));
}

void ConfigLoader::process_command(const FileEntryRef& config_file,
                                   const pugi::xml_node& node)
{
    // Get the command name (required)
    auto name = node.attribute("Name");
    if(!name || name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, node, "Name"sv);
        return;
    }

    // Find or create the command definition
    auto [command, _] = builder.insert_command(toupper(name.value()));
    assert(command != nullptr);

    std::span<CommandTable::ParamDef> param_defs;

    // Process parameters if present
    if(auto params = node.child("Params"))
    {
        // Ensure this is the only child
        if(params != node.first_child() || params.next_sibling())
        {
            report_unknown_node(config_file, params.next_sibling()
                                                     ? params.next_sibling()
                                                     : node.first_child());
            return;
        }

        size_t num_params = std::distance(params.begin(), params.end());

        ArenaAllocator<> scratchpad_allocator(&scratchpad);
        size_t param_defs_capacity = 0;

        for(auto param = params.first_child(); param;
            param = param.next_sibling())
        {
            if(std::strcmp(param.name(), "Param") == 0)
            {
                if(auto param_def = process_param_node(config_file, param))
                {
                    if(!param_defs.empty() && param_defs.back().is_optional())
                    {
                        report_opt_param_not_last(config_file, param);
                    }
                    else
                    {
                        std::tie(param_defs, param_defs_capacity)
                                = util::new_array_element(
                                        *param_def, param_defs,
                                        param_defs_capacity, num_params,
                                        scratchpad_allocator);
                    }
                }
            }
            else
            {
                report_unknown_node(config_file, param);
            }
        }

        // We never actually need to reallocate the buffer.
        assert(param_defs.size() <= num_params);
    }
    else if(node.first_child())
    {
        // No <Params> but has other children - error
        report_unknown_node(config_file, node.first_child());
    }

    // Always set all the parameters to override the command definition (TODO
    // add a unit test for this)
    builder.set_command_params(*command, param_defs.begin(), param_defs.end());
}

void ConfigLoader::process_alternators_section(const FileEntryRef& config_file,
                                               const pugi::xml_node& section)
{
    for(auto node = section.first_child(); node; node = node.next_sibling())
    {
        if(std::strcmp(node.name(), "Alternator") == 0)
        {
            process_alternator_node(config_file, node);
        }
        else
        {
            report_unknown_node(config_file, node);
        }
    }
}

void ConfigLoader::process_alternator_node(const FileEntryRef& config_file,
                                           const pugi::xml_node& node)
{
    // Get the alternator name (required)
    auto name = node.attribute("Name");
    if(!name || name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, node, "Name"sv);
        return;
    }

    // Find or create the alternator definition
    auto [alternator, _] = builder.insert_alternator(toupper(name.value()));
    assert(alternator != nullptr);

    // Process each alternative
    for(auto alt = node.first_child(); alt; alt = alt.next_sibling())
    {
        if(std::strcmp(alt.name(), "Alternative") == 0)
        {
            process_alternative_node(config_file, alt, *alternator);
        }
        else
        {
            report_unknown_node(config_file, alt);
        }
    }
}

void ConfigLoader::process_alternative_node(
        const FileEntryRef& config_file, const pugi::xml_node& node,
        CommandTable::AlternatorDef& alternator)
{
    // Get the command name (required)
    auto cmd_name = node.attribute("Name");
    if(!cmd_name || cmd_name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, node, "Name"sv);
        return;
    }

    // An alternator definition may appear before its associated
    // commands appear in the config file.
    //
    // If this is the first time the command is seen, it'll be
    // a clean unhandled command. Later it may be marked as
    // handled by a <CommandId> node.
    auto [command, _] = builder.insert_command(toupper(cmd_name.value()));
    assert(command != nullptr);

    // Add the alternative
    builder.insert_alternative(alternator, *command);
}

void ConfigLoader::process_constants_section(const FileEntryRef& config_file,
                                             const pugi::xml_node& section)
{
    for(auto node = section.first_child(); node; node = node.next_sibling())
    {
        if(std::strcmp(node.name(), "Enum") == 0)
        {
            process_enum_node(config_file, node);
        }
        else
        {
            report_unknown_node(config_file, node);
        }
    }
}

auto ConfigLoader::parse_constant_value(const FileEntryRef& config_file,
                                        const pugi::xml_node& constant_node,
                                        const char* value_cstr) -> int32_t
{
    std::string_view str = value_cstr;

    // Default base
    int base = 10;
    bool is_positive_hex = false;

    // Skip leading plus sign if present and check for invalid minus after
    if(!str.empty() && str[0] == '+')
    {
        str.remove_prefix(1);
        if(!str.empty() && str[0] == '-')
        {
            report_invalid_constant_value(config_file, constant_node,
                                          value_cstr);
            return 0;
        }
    }

    // Check for hex prefix and handle negative hex specially
    if(str.size() > 2)
    {
        if(str[0] == '-' && str.size() > 3 && str[1] == '0'
           && (str[2] == 'x' || str[2] == 'X'))
        {
            // Negative hex case (-0x...)
            base = 16;

            // Keep minus but remove 0x
            size_t new_str_size = str.size() - 2;
            char* new_str = ArenaAllocator<>(&scratchpad)
                                    .allocate_object<char>(str.size() - 2);
            std::uninitialized_default_construct(new_str,
                                                 new_str + new_str_size);

            new_str[0] = '-';
            std::copy(str.data() + 3, str.data() + str.size(), new_str + 1);

            str = std::string_view(new_str, new_str_size);
        }
        else if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        {
            // Positive hex case (0x...)
            base = 16;
            is_positive_hex = true;
            str = str.substr(2);
        }
    }

    if(is_positive_hex)
    {
        // For hex values, parse as uint32_t first to handle full range
        uint32_t unsigned_value;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(),
                                         unsigned_value, base);

        // Must be a valid number and consume entire string
        if(ec != std::errc() || ptr != str.data() + str.size())
        {
            report_invalid_constant_value(config_file, constant_node,
                                          value_cstr);
            return 0;
        }

        // For positive hex, just reinterpret as signed
        return static_cast<int32_t>(unsigned_value);
    }
    else
    {
        // For decimal values and negative hex, parse directly as int32_t
        int32_t value;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(),
                                         value, base);

        // Must be a valid number and consume entire string
        if(ec != std::errc() || ptr != str.data() + str.size())
        {
            report_invalid_constant_value(config_file, constant_node,
                                          value_cstr);
            return 0;
        }

        return value;
    }
}

void ConfigLoader::process_constant_node(const FileEntryRef& config_file,
                                         const pugi::xml_node& constant_node,
                                         CommandTable::EnumId enum_id,
                                         int32_t& next_value)
{
    // Get the constant name (required)
    auto const_name = constant_node.attribute("Name");
    if(!const_name || const_name.value()[0] == '\0')
    {
        report_missing_required_attr(config_file, constant_node, "Name"sv);
        return;
    }

    // Get the value (optional, defaults to successor of previous)
    if(auto value_attr = constant_node.attribute("Value"); value_attr)
    {
        next_value = parse_constant_value(config_file, constant_node,
                                          value_attr.value());
    }

    const int32_t value = next_value;

    // Add the constant
    builder.insert_or_assign_constant(enum_id, toupper(const_name.value()),
                                      value);
    ++next_value;
}

void ConfigLoader::process_constant_node(const FileEntryRef& config_file,
                                         const pugi::xml_node& constant_node,
                                         CommandTable::EnumId enum_id,
                                         int32_t& next_value,
                                         bool mirror_global)
{
    const int32_t value_before = next_value;
    process_constant_node(config_file, constant_node, enum_id, next_value);
    if(!mirror_global || enum_id == CommandTable::global_enum)
        return;

    auto const_name = constant_node.attribute("Name");
    if(!const_name || const_name.value()[0] == '\0')
        return;

    builder.insert_or_assign_constant(CommandTable::global_enum,
                                      toupper(const_name.value()),
                                      value_before);
}

void ConfigLoader::process_enum_node(const FileEntryRef& config_file,
                                     const pugi::xml_node& enum_node)
{
    // Get the enum name (optional, defaults to global enum)
    auto name = enum_node.attribute("Name");
    if(name && name.value()[0] == '\0')
    {
        report_empty_attr(config_file, enum_node, "Name"sv);
        return;
    }

    const bool is_global = [&enum_node]() {
        if(const auto global = enum_node.attribute("Global"))
        {
            return std::strcmp(global.value(), "true") == 0
                   || std::strcmp(global.value(), "1") == 0;
        }
        return false;
    }();

    auto enum_id
            = name ? builder.insert_enumeration(toupper(name.value())).first
                   : CommandTable::global_enum;

    // Process each constant
    int32_t next_value = 0;
    for(auto constant = enum_node.first_child(); constant;
        constant = constant.next_sibling())
    {
        if(std::strcmp(constant.name(), "Constant") == 0)
        {
            process_constant_node(config_file, constant, enum_id, next_value,
                                  is_global);
        }
        else
        {
            report_unknown_node(config_file, constant);
        }
    }
}
} // namespace
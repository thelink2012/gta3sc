#pragma once
#include <filesystem>
#include <gta3sc/command-table.hpp>

namespace gta3sc
{
class SourceManager;
class SourceFile;
class DiagnosticDescriptor;
class DiagnosticHandler;
} // namespace gta3sc

namespace gta3sc::config
{
/// Loads a `config.xml` file and populates a command table builder with the
/// commands, alternators, enumerations, etc defined in the file.
///
/// The `config.xml` may import other files. These imports will be relative to
/// the same directory as the `config.xml` file. If any given import is relative
/// to a different game config, the file will be imported from
/// `root_path/<game>/`.
///
/// See https://github.com/GTAmodding/gta3script-config for the config schema.
///
/// Errors will be reported to the given diagnostic handler.
///
/// \param root_path The root path of the game configs i.e. configs are
/// usually located at `root_path/<game>/config.xml`.
/// \param config_path The path to the `config.xml` file.
/// \param fileman file manager used to load files with diagnostic support.
/// \param diagman diagnostic handler to report errors to.
/// \param builder results will be added to this builder.
/// \return the builder with the configurations added in.
auto load_config(const std::filesystem::path& root_path,
                 const std::filesystem::path& config_path,
                 SourceManager& fileman, DiagnosticHandler& diagman,
                 CommandTable::Builder&& builder) -> CommandTable::Builder&&;

/// Same as `load_config` but returns a `CommandTable` instead of
/// manipulating a builder.
///
/// See the other overload for more information.
auto load_config(const std::filesystem::path& root_path,
                 const std::filesystem::path& config_path,
                 SourceManager& fileman, DiagnosticHandler& diagman,
                 ArenaAllocator<> allocator) -> CommandTable;

/// Same as `load_config` but uses a loaded `SourceFile` instead of a path.
///
/// Importing other configs from within a config are not supported in this
/// overload.
///
/// See the other overload for more information.
auto load_config(const SourceFile& config_file, DiagnosticHandler& diagman,
                 CommandTable::Builder&& builder) -> CommandTable::Builder&&;
} // namespace gta3sc::config

namespace gta3sc::config::diag
{
extern const DiagnosticDescriptor
        xml_missing_required_attr; // %0 => string (attribute name)
extern const DiagnosticDescriptor
        xml_empty_attr; // %0 => string (attribute name)
extern const DiagnosticDescriptor xml_unknown_node; // %0 => string (got)
extern const DiagnosticDescriptor
        xml_parse_failed; // %0 => string (error description)
extern const DiagnosticDescriptor
        xml_invalid_root_element; // %0 => string (got)
extern const DiagnosticDescriptor xml_import_too_deep;
extern const DiagnosticDescriptor xml_invalid_param_type; // %0 => string (got)
extern const DiagnosticDescriptor xml_opt_must_be_last_param;
extern const DiagnosticDescriptor
        xml_security_import_filesystem_traversal; // %0 => string (got)
extern const DiagnosticDescriptor xml_import_failed_to_determine_game_config;
extern const DiagnosticDescriptor
        xml_invalid_constant_value; // %0 => string (value)
extern const DiagnosticDescriptor xml_invalid_handled_without_id;
extern const DiagnosticDescriptor
        xml_invalid_command_id;                         // %0 => string (value)
extern const DiagnosticDescriptor xml_expected_boolean; // %0 => string (got)
} // namespace gta3sc::config::diag
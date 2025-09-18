#pragma once

#include "gta3sc/command-table.hpp"
#include "gta3sc/diagnostics.hpp"
#include "gta3sc/sourceman.hpp"
namespace gta3sc
{
/// Loads a `config.xml` file and populates a command table builder with the
/// commands, alternators, enumerations, etc defined in the file.
///
/// The `config.xml` may import other files. These imports will be relative to
/// the same directory as the `config.xml` file. If any given import is relative
/// to a different game config, the file will be imported from `root_path/<game>/`.
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
auto load_config(
    const std::filesystem::path& root_path,
    const std::filesystem::path& config_path,
    SourceManager& fileman,
    DiagnosticHandler& diagman,
    CommandTable::Builder&& builder
) -> CommandTable::Builder&&;


/// Same as `load_config` but returns a `CommandTable` instead of
/// manipulating a builder.
///
/// See the other overload for more information.
auto load_config(
    const std::filesystem::path& root_path,
    const std::filesystem::path& config_path,
    SourceManager& fileman,
    DiagnosticHandler& diagman,
    ArenaAllocator<> allocator
) -> CommandTable;

/// Same as `load_config` but uses a loaded `SourceFile` instead of a path.
///
/// Importing other configs from within a config are not supported in this overload.
///
/// See the other overload for more information.
auto load_config(
    const SourceFile& config_file,
    DiagnosticHandler& diagman,
    CommandTable::Builder&& builder
) -> CommandTable::Builder&&;
} // namespace gta3sc
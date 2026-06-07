#pragma once
#include <filesystem>
#include <gta3sc/fwd.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/util/arena.hpp>

namespace gta3sc::config
{
/// Loads models from a level file (i.e. `gta.dat` / `gta3.dat` /
/// `gta_vc.dat`).
///
/// Opens a given level file and parses all IDE files specified in it. Each
/// `IDE` line has the format `IDE path/to/file.ide`. Paths are resolved to
/// full paths by using \p path_resolver (typically a 
/// `gta3sc::filesystem::RelativeInsensitivePathResolver(game_root)`).
///
/// Errors will be reported to the given diagnostic handler.
///
/// \param path_resolver resolves each relative IDE path in the DAT to a full path.
/// \param level_path the path to the level file (e.g. `gta3.dat`).
/// \param objs_only whether to only consider `objs`, `tobj` and `anim`
/// sections in IDEs.
/// \param file_pool file pool used to load files with diagnostic support.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_level(
        const gta3sc::filesystem::PathResolver& path_resolver,
        const std::filesystem::path& level_path, bool objs_only,
        FilePool& file_pool, DiagnosticHandler& diagman,
        ModelTable::Builder&& builder) -> ModelTable::Builder&&;

/// Same as `load_models_from_level` but returns a `ModelTable` instead of
/// manipulating a builder.
///
/// See the other overload for more information.
auto load_models_from_level(
        const gta3sc::filesystem::PathResolver& path_resolver,
        const std::filesystem::path& level_path, bool objs_only,
        FilePool& file_pool, DiagnosticHandler& diagman,
        ArenaAllocator<> allocator) -> ModelTable;

/// Loads models from an IDE file.
///
/// \param ide_file the IDE file to load.
/// \param objs_only whether to only consider `objs`, `tobj` and `anim`
/// sections.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_ide(const FileEntryRef& ide_file, bool objs_only,
                          DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&;

/// Same as `load_models_from_ide` but takes a path instead of a source file.
///
/// \param ide_path the path to the IDE file.
/// \param objs_only whether to only consider `objs`, `tobj` and `anim`
/// sections.
/// \param file_pool file pool used to load files with diagnostic support.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_ide(const std::filesystem::path& ide_path, bool objs_only,
                          FilePool& file_pool, DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&;
} // namespace gta3sc::config

namespace gta3sc::config::diag
{
extern const DiagnosticDescriptor models_invalid_ide_line;
} // namespace gta3sc::config::diag

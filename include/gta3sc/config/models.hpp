#pragma once
#include <filesystem>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/sourceman.hpp>
#include <gta3sc/util/arena.hpp>

namespace gta3sc::config
{
/// Loads models from a level file (i.e. `gta.dat` / `gta3.dat` /
/// `gta_vc.dat`).
///
/// Opens a given level file and parses all IDE files specified in it. An IDE
/// line has the format `IDE path/to/file.ide`. IDE files are found relative to
/// the given `root_path`.
///
/// Errors will be reported to the given diagnostic handler.
///
/// \param root_path the root path where the IDE files are located (usually the
/// game root path).
/// \param level_path the path to the level file (e.g.
/// `gta3.dat`). This path is used as-is, not relative to `root_path`.
/// \param objs_only whether to only consider `objs`, `tobj` and `anim` sections in IDEs.
/// \param fileman file manager used to load files with diagnostic support.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_level(const std::filesystem::path& root_path,
                            const std::filesystem::path& level_path,
                            bool objs_only,
                            SourceManager& fileman, DiagnosticHandler& diagman,
                            ModelTable::Builder&& builder)
        -> ModelTable::Builder&&;


/// Same as `load_models_from_level` but returns a `ModelTable` instead of
/// manipulating a builder.
///
/// See the other overload for more information.
auto load_models_from_level(const std::filesystem::path& root_path,
                            const std::filesystem::path& level_path,
                            bool objs_only, SourceManager& fileman,
                            DiagnosticHandler& diagman,
                            ArenaAllocator<> allocator) -> ModelTable;

/// Loads models from an IDE file.
///
/// \param ide_file the IDE file to load.
/// \param objs_only whether to only consider `objs`, `tobj` and `anim` sections.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_ide(
        const SourceFile& ide_file,
        bool objs_only,
        DiagnosticHandler& diagman,
        ModelTable::Builder&& builder) -> ModelTable::Builder&&;

/// Same as `load_models_from_ide` but takes a path instead of a source file.
///
/// \param ide_path the path to the IDE file.
/// \param objs_only whether to only consider `objs`, `tobj` and `anim` sections.
/// \param fileman file manager used to load files with diagnostic support.
/// \param diagman diagnostic handler to report errors to.
/// \param builder resulting models will be added to this builder.
/// \return the builder with the models added.
auto load_models_from_ide(const std::filesystem::path& ide_path,
                          bool objs_only,
                          SourceManager& fileman, DiagnosticHandler& diagman,
                          ModelTable::Builder&& builder)
        -> ModelTable::Builder&&;
} // namespace gta3sc::config
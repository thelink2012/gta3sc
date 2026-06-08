add_library(gta3sc
  command-table.cpp
  model-table.cpp
  diagnostics.cpp
  source-manager.cpp
  ir/parser-ir.cpp
  ir/sema-ir.cpp
  ir/symbol-table.cpp
  util/arena.cpp
  util/name-generator.cpp
  filesystem/file-pool.cpp
  filesystem/resolver/script-path-resolver.cpp
  filesystem/resolver/relative-path-resolver.cpp
  filesystem/resolver/relative-insensitive-path-resolver.cpp
  config/models.cpp
  syntax/diagnostics.cpp
  syntax/preprocessor.cpp
  syntax/scanner.cpp
  syntax/parser.cpp
  syntax/multifile-parser.cpp
  syntax/sema.cpp
  syntax/lowering/instruction-remover.cpp
  syntax/lowering/load-and-launch-mission-rewriter.cpp
  syntax/lowering/mission-stmt-rewriter.cpp
  syntax/lowering/repeat-stmt-rewriter.cpp
  syntax/lowering/if-stmt-rewriter.cpp
  syntax/lowering/while-stmt-rewriter.cpp
  syntax/lowering/scope-remover.cpp
  syntax/lowering/stats-rewriter.cpp
  syntax/lowering/var-decl-remover.cpp
  syntax/visitor/required-files-visitor.cpp
  codegen/storage-table.cpp
  codegen/relocation-table.cpp
  codegen/relocator.cpp
  codegen/trilogy/emitter.cpp
  codegen/trilogy/codegen.cpp
  codegen/trilogy/multifile-codegen.cpp
  driver/compilation.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_26)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)
target_compile_options(gta3sc PRIVATE ${GTA3SC_PEDANTIC_COMPILE_OPTIONS})

add_library(gta3sc-config
  config/config.cpp
)
target_compile_features(gta3sc-config PUBLIC cxx_std_26)
target_link_libraries(gta3sc-config gta3sc pugixml-static)
target_compile_options(gta3sc-config PRIVATE ${GTA3SC_PEDANTIC_COMPILE_OPTIONS})
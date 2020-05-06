add_library(gta3sc 
  command-manager.cpp
  model-manager.cpp
  diagnostics.cpp
  sourceman.cpp
  ir/parser-ir.cpp
  ir/sema-ir.cpp
  ir/symbol-table.cpp
  util/arena.cpp
  util/name-generator.cpp
  syntax/preprocessor.cpp
  syntax/scanner.cpp
  syntax/parser.cpp
  syntax/sema.cpp
  syntax/lowering/repeat-stmt-rewriter.cpp
  codegen/trilogy/emitter.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_20)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(gta3sc span-lite)

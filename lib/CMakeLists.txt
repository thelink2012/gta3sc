add_library(gta3sc 
  command-table.cpp
  model-table.cpp
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
  codegen/storage-table.cpp
  codegen/relocation-table.cpp
  codegen/trilogy/emitter.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_20)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(gta3sc span-lite)
target_compile_options(gta3sc PRIVATE ${GTA3SC_PEDANTIC_COMPILE_OPTIONS})

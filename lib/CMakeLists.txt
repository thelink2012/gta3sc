add_library(gta3sc 
  command-manager.cpp
  diagnostics.cpp
  sourceman.cpp
  ir/symbol-table.cpp
  syntax/preprocessor.cpp
  syntax/scanner.cpp
  syntax/parser.cpp
  syntax/sema.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_17)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)

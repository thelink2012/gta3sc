add_library(gta3sc 
  diagnostics.cpp
  sourceman.cpp
  symbol-table.cpp
  preprocessor.cpp
  scanner.cpp
  parser.cpp
  sema.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_17)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)

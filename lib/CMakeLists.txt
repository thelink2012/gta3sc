add_library(gta3sc 
  sourceman.cpp
  scanner.cpp
)
target_compile_features(gta3sc PUBLIC cxx_std_17)
target_include_directories(gta3sc
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>)

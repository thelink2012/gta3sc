cmake_minimum_required(VERSION 3.5)
project(gta3sc)

option(GTA3SC_COVERAGE "Enable code coverage measurements with gcov/lcov." OFF)

set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake/modules
  ${CMAKE_MODULE_PATH})

include(CTest)

if(GTA3SC_COVERAGE)
  find_program( GCOV_PATH gcov-8 )
  include(CodeCoverage)
  append_coverage_compiler_flags()
  set(COVERAGE_LCOV_EXCLUDES '/usr/*' '${CMAKE_CURRENT_SOURCE_DIR}/unittest/*')
endif()

add_subdirectory(lib)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(unittest)
endif()

cmake_minimum_required(VERSION 3.12)

project(gta3sc CXX)

option(GTA3SC_COVERAGE "Enable code coverage measurements with gcov/lcov." OFF)
option(GTA3SC_ANALYSIS "Enable static analysis and linting with clang-tidy." OFF)

set(CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake/modules
  ${CMAKE_MODULE_PATH})

if(GTA3SC_COVERAGE)
  message("GTA3SC_COVERAGE enabled. Switching to Debug build.")
  set(CMAKE_BUILD_TYPE Debug)
  include(CodeCoverage)
  append_coverage_compiler_flags()
  set(COVERAGE_LCOV_EXCLUDES '/usr/*' '${CMAKE_CURRENT_SOURCE_DIR}/unittest/*' 
                                      '${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/*')
endif()

if(GTA3SC_ANALYSIS)
  message("GTA3SC_ANALYSIS enabled. Switching to Debug build and verbose Makefile.")
  set(CMAKE_BUILD_TYPE Debug)
  set(CMAKE_VERBOSE_MAKEFILE ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  find_program(CLANG_TIDY NAMES clang-tidy-12 clang-tidy-11 clang-tidy-10 clang-tidy)
  if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
  else()
    message(SEND_ERROR "GTA3SC_ANALYSIS enabled but no clang-tidy binary found")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(GTA3SC_PEDANTIC_COMPILE_OPTIONS -pedantic-errors -pedantic -Wall -Wextra -Werror
    -Wcast-align -Wdouble-promotion) # -Wconversion -Wno-sign-conversion
elseif(MSVC)
  # TODO
endif()

include(CTest)

add_subdirectory(thirdparty)
add_subdirectory(lib)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(unittest)
endif()

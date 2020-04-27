add_executable(gta3sc_unittest
    main.cpp
    syntax/preprocessor.cpp
    syntax/scanner.cpp
    syntax/parser.cpp
    syntax/sema.cpp
    syntax/lowering/repeat-stmt-rewriter.cpp
    codegen/trilogy/emitter.cpp
)
target_compile_features(gta3sc_unittest PUBLIC cxx_std_17)
target_link_libraries(gta3sc_unittest PRIVATE gta3sc)
target_include_directories(gta3sc_unittest 
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest")

add_test(NAME unittest COMMAND gta3sc_unittest)

if(GTA3SC_COVERAGE)
  setup_target_for_coverage_lcov(
    NAME coverage
    EXECUTABLE ctest --extra-verbose --output-on-failure
    DEPENDENCIES gta3sc_unittest)
else()
  add_custom_target(coverage
    COMMAND ctest --extra-verbose --output-on-failure
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS gta3sc_unittest
    COMMENT "Project generated with no coverage suport. Skipping...")
endif()

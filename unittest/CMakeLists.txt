add_executable(gta3sc_unittest
    syntax/preprocessor.cpp
    syntax/scanner.cpp
    syntax/parser.cpp
    syntax/sema.cpp
    syntax/lowering/repeat-stmt-rewriter.cpp
    codegen/storage-table.cpp
    codegen/relocation-table.cpp
    codegen/trilogy/emitter.cpp
    codegen/trilogy/codegen.cpp
    config/config-fixture.hpp
    config/config-fixture.cpp
    config/config-alternator.cpp
    config/config-commands.cpp
    config/config-command.cpp
    config/config-command-id.cpp
    config/config-constant.cpp
    config/config-import.cpp
    config/config-load.cpp
    config/config-root.cpp
)
target_compile_features(gta3sc_unittest PUBLIC cxx_std_20)
target_link_libraries(gta3sc_unittest PRIVATE gta3sc gta3sc-config doctest_with_main)

doctest_discover_tests(gta3sc_unittest)

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

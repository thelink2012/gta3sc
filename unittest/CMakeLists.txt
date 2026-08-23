add_executable(gta3sc_unittest
    with-temp-dir-fixture.cpp
    driver/compilation.cpp
    syntax/preprocessor.cpp
    syntax/scanner.cpp
    syntax/parser.cpp
    syntax/sema.cpp
    syntax/multifile-parser.cpp
    syntax/visitor/required-files-visitor.cpp
    syntax/lowering/instruction-remover.cpp
    syntax/lowering/load-and-launch-mission-rewriter.cpp
    syntax/lowering/mission-stmt-rewriter.cpp
    syntax/lowering/repeat-stmt-rewriter.cpp
    syntax/lowering/if-stmt-rewriter.cpp
    syntax/lowering/while-stmt-rewriter.cpp
    syntax/lowering/scope-remover.cpp
    syntax/lowering/stats-rewriter.cpp
    syntax/lowering/var-decl-remover.cpp
    codegen/storage-table.cpp
    codegen/relocation-table.cpp
    codegen/relocator.cpp
    codegen/trilogy/emitter.cpp
    codegen/trilogy/codegen.cpp
    codegen/trilogy/multifile-codegen.cpp
    config/config-path.cpp
    config/config-fixture.hpp
    config/config-alternator.cpp
    config/config-commands.cpp
    config/config-command.cpp
    config/config-command-id.cpp
    config/config-constant.cpp
    config/config-import.cpp
    config/config-load.cpp
    config/config-root.cpp
    config/models-ide.cpp
    config/models-level.cpp
    filesystem/file-pool.cpp
    filesystem/path-resolver/script-path-resolver.cpp
    filesystem/path-resolver/relative-path-resolver.cpp
    filesystem/path-resolver/relative-insensitive-path-resolver.cpp
    ir/instruction-rewriter.cpp
    cli/option-parser.cpp
    cli/run.cpp
    cli/run-compile.cpp
)
target_compile_features(gta3sc_unittest PUBLIC cxx_std_26)
target_link_libraries(gta3sc_unittest PRIVATE gta3sc gta3sc-config gta3sc-cli doctest_with_main)

doctest_discover_tests(gta3sc_unittest PROPERTIES LABELS unit)

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

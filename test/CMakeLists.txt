find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
  message(FATAL_ERROR
    "GTA3SC_INTEGRATION_TESTS=ON but a Python 3 interpreter was not found.")
endif()

find_program(GTA3SC_LIT_EXECUTABLE NAMES lit)
if(NOT GTA3SC_LIT_EXECUTABLE)
  message(FATAL_ERROR
    "GTA3SC_INTEGRATION_TESTS=ON but 'lit' was not found on PATH. "
    "Install the PyPI lit package (e.g. uv tool install lit).")
endif()

find_program(GTA3SC_FILECHECK_EXECUTABLE NAMES filecheck)
if(NOT GTA3SC_FILECHECK_EXECUTABLE)
  message(FATAL_ERROR
    "GTA3SC_INTEGRATION_TESTS=ON but 'filecheck' was not found on PATH. "
    "Install the PyPI filecheck package (e.g. uv tool install filecheck).")
endif()

set(GTA3SC_LIT_SOURCE_CFG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg")
set(GTA3SC_LIT_EXEC_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg"
  @ONLY)

add_test(
  NAME gta3sc_integration
  COMMAND "${GTA3SC_LIT_EXECUTABLE}"
          --param "gta3sc=$<TARGET_FILE:gta3sc-cli-exe>"
          -v "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(gta3sc_integration PROPERTIES LABELS integration)

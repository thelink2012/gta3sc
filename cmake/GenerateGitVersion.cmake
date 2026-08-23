#   gta3sc_target_git_version(<target> <template.in>)
#
# Adds a generated version.cpp to <target>, filled from <template.in>.
# Rebuilds that source when git HEAD or the index change so --version stays
# current without re-running CMake.

include_guard(GLOBAL)

set(_gta3sc_generate_git_version_file "${CMAKE_CURRENT_LIST_FILE}")

# Runs git in src_dir. RESULT_VARIABLE is the exit code so a missing tag or
# absent git is distinct from a successful empty print.
function(_gta3sc_git)
  cmake_parse_arguments(PARSE_ARGV 0 ARG
    ""
    "WORKING_DIRECTORY;OUTPUT_VARIABLE;RESULT_VARIABLE"
    "COMMAND")
  if(NOT GIT_EXECUTABLE)
    set(${ARG_OUTPUT_VARIABLE} "" PARENT_SCOPE)
    set(${ARG_RESULT_VARIABLE} 1 PARENT_SCOPE)
    return()
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" ${ARG_COMMAND}
    WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
    OUTPUT_VARIABLE output
    ERROR_QUIET
    RESULT_VARIABLE result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(${ARG_OUTPUT_VARIABLE} "${output}" PARENT_SCOPE)
  set(${ARG_RESULT_VARIABLE} "${result}" PARENT_SCOPE)
endfunction()

# Resolves a git-dir relative path (HEAD, index) to an absolute file we can
# list as a build dependency.
function(_gta3sc_git_dep src_dir name out_var)
  _gta3sc_git(
    WORKING_DIRECTORY "${src_dir}"
    OUTPUT_VARIABLE path
    RESULT_VARIABLE path_result
    COMMAND rev-parse --git-path ${name})
  if(NOT path_result EQUAL 0 OR path STREQUAL "")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()
  if(NOT IS_ABSOLUTE "${path}")
    get_filename_component(path "${path}" ABSOLUTE BASE_DIR "${src_dir}")
  endif()
  if(EXISTS "${path}")
    set(${out_var} "${path}" PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

# Substitutes the git identity into the template. copy_if_different keeps
# version.cpp's timestamp stable when the identity did not change.
function(_gta3sc_write_git_version src_dir in_file out_file)
  set(GTA3SC_GIT_VERSION "unknown")

  _gta3sc_git(
    WORKING_DIRECTORY "${src_dir}"
    OUTPUT_VARIABLE hash
    RESULT_VARIABLE hash_result
    COMMAND rev-parse --short=12 HEAD)
  if(hash_result EQUAL 0 AND NOT hash STREQUAL "")
    set(GTA3SC_GIT_VERSION "${hash}")

    _gta3sc_git(
      WORKING_DIRECTORY "${src_dir}"
      OUTPUT_VARIABLE tag
      RESULT_VARIABLE tag_result
      COMMAND describe --tags --exact-match HEAD)
    if(tag_result EQUAL 0 AND NOT tag STREQUAL "")
      set(GTA3SC_GIT_VERSION "${tag}-${hash}")
    endif()

    _gta3sc_git(
      WORKING_DIRECTORY "${src_dir}"
      OUTPUT_VARIABLE unused
      RESULT_VARIABLE unused_result
      COMMAND update-index -q --refresh)
    _gta3sc_git(
      WORKING_DIRECTORY "${src_dir}"
      OUTPUT_VARIABLE unused
      RESULT_VARIABLE unused_result
      COMMAND diff-index --quiet HEAD --)
    if(unused_result EQUAL 1)
      set(GTA3SC_GIT_VERSION "${GTA3SC_GIT_VERSION}-dirty")
    endif()
  endif()

  string(REPLACE "\\" "\\\\" GTA3SC_GIT_VERSION "${GTA3SC_GIT_VERSION}")
  string(REPLACE "\"" "\\\"" GTA3SC_GIT_VERSION "${GTA3SC_GIT_VERSION}")

  set(tmp "${out_file}.tmp")
  configure_file("${in_file}" "${tmp}" @ONLY)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${tmp}" "${out_file}")
  file(REMOVE "${tmp}")
endfunction()

# Public entry: generate version.cpp next to the calling listfile's binary dir.
function(gta3sc_target_git_version target template)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR
      "gta3sc_target_git_version: no such target '${target}'")
  endif()
  if(NOT template)
    message(FATAL_ERROR
      "gta3sc_target_git_version: missing <template.in>")
  endif()

  if(NOT IS_ABSOLUTE "${template}")
    set(template "${CMAKE_CURRENT_SOURCE_DIR}/${template}")
  endif()
  get_filename_component(template "${template}" ABSOLUTE)
  if(NOT EXISTS "${template}")
    message(FATAL_ERROR
      "gta3sc_target_git_version: template not found:\n  ${template}")
  endif()

  get_filename_component(out_name "${template}" NAME)
  if(NOT out_name MATCHES "\\.in$")
    message(FATAL_ERROR
      "gta3sc_target_git_version: template must end in .in:\n  ${template}")
  endif()
  string(REGEX REPLACE "\\.in$" "" out_name "${out_name}")
  set(out_file "${CMAKE_CURRENT_BINARY_DIR}/${out_name}")

  # Walk upward from the calling listfile so a superbuild's git is not used.
  set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}")
  set(script "${_gta3sc_generate_git_version_file}")

  find_package(Git QUIET)
  set(args
    -D "GIT_EXECUTABLE=${GIT_EXECUTABLE}"
    -D "SRC_DIR=${src_dir}"
    -D "IN_FILE=${template}"
    -D "OUT_FILE=${out_file}"
    -P "${script}")

  set(depends "${script}" "${template}")
  if(GIT_FOUND)
    foreach(name IN ITEMS HEAD index)
      _gta3sc_git_dep("${src_dir}" ${name} dep)
      if(dep)
        list(APPEND depends "${dep}")
      endif()
    endforeach()
  endif()

  execute_process(COMMAND "${CMAKE_COMMAND}" ${args})
  add_custom_command(
    OUTPUT "${out_file}"
    COMMAND "${CMAKE_COMMAND}" ${args}
    DEPENDS ${depends}
    COMMENT "Generating ${out_file}"
    VERBATIM)
  target_sources(${target} PRIVATE "${out_file}")
endfunction()

# Script invocation from add_custom_command / execute_process above.
if(NOT CMAKE_PARENT_LIST_FILE)
  _gta3sc_write_git_version("${SRC_DIR}" "${IN_FILE}" "${OUT_FILE}")
endif()

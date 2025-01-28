cmake_minimum_required(VERSION 2.8)
project(gta3sc)
include(deps/CMake/PrecompiledHeader.cmake)
include(deps/CMake/GetGitRevisionDescription.cmake)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_COMPILER_IS_GNUXX AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_COMPILER_IS_GNUXX 1)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-placement-new")
endif()

if(CMAKE_COMPILER_IS_CLANGXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-parentheses-equality -Wno-tautological-compare")
endif()

if(MSVC)
    # MSVC ITERATOR_DEBUG_LEVEL=2 breaks expected-lite
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

    # Fix some weird linker error on Debug builds
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Ob1")

    # Static link the CRT on release
    foreach(flag_var CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach(flag_var)
    if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "/MT")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    endif()
    if(NOT CMAKE_CXX_FLAGS_RELWITHDEBINFO MATCHES "/MT")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
    endif()

endif(MSVC)

include_directories(
  src
  deps
  deps/rapidxml
  deps/cppformat
  deps/optional/include
  deps/expected/include
  deps/any/include
  deps/variant/include
)

add_library(cppformat
  deps/cppformat/cppformat/format.cc
)

set(GTA3SC_SRC_MAIN
  src/stdinc.h
  src/stdinc.cpp
  src/cdimage.hpp
  src/binary_fetcher.hpp
  src/binary_writer.hpp
  src/annotation.hpp
  src/codegen.hpp
  src/codegen.cpp
  src/config.cpp
  src/commands.cpp
  src/commands.hpp
  src/compiler.hpp
  src/compiler.cpp
  src/decompiler_ir2.hpp
  src/disassembler.hpp
  src/disassembler.cpp
  src/main.cpp
  src/main_compile.cpp
  src/main_decompile.cpp
  src/parser_lexer.cpp
  src/parser_syntax.cpp
  src/parser.hpp
  src/program.cpp
  src/program.hpp
  src/symtable.cpp
  src/symtable.hpp
  src/script.hpp
  src/script.cpp
  src/system.cpp
  src/system.hpp
)

set(GTA3SC_SRC_MISC
  src/cpp/any.hpp
  src/cpp/argv.hpp
  src/cpp/contracts.hpp
  src/cpp/file.hpp
  src/cpp/filesystem.hpp
  src/cpp/icompare.hpp
  src/cpp/optional.hpp
  src/cpp/scope_guard.hpp
  src/cpp/variant.hpp
  src/cpp/string_view.hpp
  src/cpp/small_vector.hpp
)

set(GTA3SC_SRC_GITSHA1 "${CMAKE_CURRENT_BINARY_DIR}/git-sha1.cpp")

add_executable(gta3sc ${GTA3SC_SRC_GITSHA1} ${GTA3SC_SRC_MISC} ${GTA3SC_SRC_MAIN})
source_group("autogen" FILES ${GTA3SC_SRC_GITSHA1})
source_group("cpp" FILES ${GTA3SC_SRC_MISC})
source_group("" FILES ${GTA3SC_SRC_MAIN})

target_link_libraries(gta3sc cppformat)

if(MSVC) # idk how to setup this in GCC/Clang
	add_precompiled_header(gta3sc stdinc.h SOURCE_CXX src/stdinc.cpp)
endif(MSVC)

add_definitions(-DGTA3SC_USING_GIT_DESCRIBE)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe_long_exact(GIT_DESCRIBE_TAG)
git_branch(GIT_BRANCH)
if(GIT_DESCRIBE_TAG MATCHES "NOTFOUND")
	set(GIT_DESCRIBE_TAG "")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git-sha1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git-sha1.cpp" @ONLY)

add_custom_command(TARGET gta3sc POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/config $<TARGET_FILE_DIR:gta3sc>/config)

#install(TARGETS gta3sc RUNTIME DESTINATION bin)

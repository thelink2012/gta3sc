add_subdirectory(span-lite EXCLUDE_FROM_ALL)

if(BUILD_TESTING)
    set(DOCTEST_WITH_TESTS OFF)
    add_subdirectory(doctest EXCLUDE_FROM_ALL)

    # Hack for fixing https://github.com/onqtam/doctest/issues/210
    add_custom_target(gta3sc_gen_doctest_h DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest/doctest.h)
    add_dependencies(doctest gta3sc_gen_doctest_h)

    # Import doctest_discover_tests(<target>)
    # Plus a hack for preserving _DOCTEST_DISCOVER_TESTS_SCRIPT because we are not
    # including this module at the top-level directory.
    include(${CMAKE_CURRENT_SOURCE_DIR}/doctest/scripts/cmake/doctest.cmake)
    set(_DOCTEST_DISCOVER_TESTS_SCRIPT "${_DOCTEST_DISCOVER_TESTS_SCRIPT}" PARENT_SCOPE)
endif()

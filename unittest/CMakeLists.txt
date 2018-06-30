add_executable(gta3sc_unittest
    main.cpp
    preprocessor.cpp
)
target_compile_features(gta3sc_unittest PUBLIC cxx_std_17)
target_link_libraries(gta3sc_unittest PRIVATE gta3sc)
target_include_directories(gta3sc_unittest 
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest")

add_test(NAME unittest COMMAND gta3sc_unittest)

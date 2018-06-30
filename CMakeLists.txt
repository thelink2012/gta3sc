cmake_minimum_required(VERSION 3.5)
project(gta3sc)

include(CTest)

add_subdirectory(lib)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(unittest)
endif()

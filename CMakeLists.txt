cmake_minimum_required(VERSION 3.5)
project(gta3sc)

add_subdirectory(lib)
if(BUILD_TESTING)
    add_subdirectory(unittest)
endif()

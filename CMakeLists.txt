cmake_minimum_required(VERSION 3.18)
project(minigdbstub)
option(USING_CONAN OFF)

# If Conan was used
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    set(USING_CONAN ON)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

# Build tests
add_subdirectory(tests)

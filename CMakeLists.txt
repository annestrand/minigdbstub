cmake_minimum_required(VERSION 3.18)
project(minigdbstub)

# If Conan was used
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

# Build tests
add_subdirectory(tests)

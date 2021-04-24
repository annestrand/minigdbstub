cmake_minimum_required(VERSION 3.10)
project(minigdbstub)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Build tests
add_subdirectory(tests)
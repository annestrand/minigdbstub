cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)

# Include minigdbstub header
include_directories(${CMAKE_SOURCE_DIR})

# Tests
add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic ${CONAN_LIBS})
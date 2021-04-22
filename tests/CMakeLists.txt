cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)
include_directories(${CMAKE_SOURCE_DIR}/../external/criterion/include/criterion)
include_directories(${CMAKE_SOURCE_DIR}/..)
link_directories(${CMAKE_SOURCE_DIR}/../external/criterion/build)

# Tests
add_executable(test_basic test_basic.c)
target_link_libraries(test_basic criterion)
add_custom_target(run ALL
    COMMAND ${CMAKE_BINARY_DIR}/test_basic --verbose
    POST_BUILD
)
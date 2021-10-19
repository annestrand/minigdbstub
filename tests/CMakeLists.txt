
cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)

# External dependencies
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)

file(GLOB SRCS *.cpp)
add_executable(minigdbstub_tests ${SRCS})
target_include_directories(minigdbstub_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(minigdbstub_tests PRIVATE "--std=c++11")
if (MSVC)
    target_compile_options(minigdbstub_tests PRIVATE "/W4")
else()
    target_compile_options(minigdbstub_tests PRIVATE -Wall)
    target_compile_options(minigdbstub_tests PRIVATE -pedantic)
    target_compile_options(minigdbstub_tests PRIVATE -Wno-unused-function)
endif()
target_link_libraries(minigdbstub_tests
    GTest::gtest
    GTest::gtest_main
)
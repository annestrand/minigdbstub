cmake_minimum_required(VERSION 3.18)

project(minigdbstub_tests)

set(GTEST_BASEDIR "")
find_library(GTEST gtest HINTS ${GTEST_BASEDIR} REQUIRED)
find_library(GTEST_MAIN gtest_main HINTS ${GTEST_BASEDIR} REQUIRED)

find_package(Threads)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests)
add_compile_options(-Wall)
add_compile_options(-pedantic)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(minigdbstub_tests
    test_recv.cpp
    test_send.cpp
    test_regs.cpp
    test_mem.cpp
)
target_link_libraries(minigdbstub_tests ${GTEST} ${GTEST_MAIN} ${CMAKE_THREAD_LIBS_INIT})

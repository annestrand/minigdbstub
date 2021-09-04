cmake_minimum_required(VERSION 3.18)

project(minigdbstub_tests)

if (NOT USING_CONAN)
    set(GTEST_BASEDIR "")
    find_library(GTEST gtest HINTS ${GTEST_BASEDIR} REQUIRED)
    find_library(GTEST_MAIN gtest_main HINTS ${GTEST_BASEDIR} REQUIRED)
endif()

find_package(Threads)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests)

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options(-Wall)
    add_compile_options(-pedantic)
    add_compile_options(-Wno-unused-function)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

file(GLOB SRCS *.cpp)
add_executable(minigdbstub_tests ${SRCS})

if (USING_CONAN)
    target_link_libraries(minigdbstub_tests ${CONAN_LIBS} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries(minigdbstub_tests ${GTEST} ${GTEST_MAIN} ${CMAKE_THREAD_LIBS_INIT})
endif()


cmake_minimum_required(VERSION 3.10)

project(minigdbstub_tests)

# External dependencies
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(minigdbstub_tests)
target_sources(minigdbstub_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_breakpoint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_recv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_regs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_send.cpp
)
target_include_directories(minigdbstub_tests PRIVATE ${CMAKE_SOURCE_DIR})
if (MSVC)
    target_compile_options(minigdbstub_tests PRIVATE "/Wall")
else()
    target_compile_options(minigdbstub_tests PRIVATE "-Wall")
    target_compile_options(minigdbstub_tests PRIVATE "-pedantic")
    target_compile_options(minigdbstub_tests PRIVATE "--std=c++11")
endif()
target_link_libraries(minigdbstub_tests
    GTest::gtest
    GTest::gtest_main
)